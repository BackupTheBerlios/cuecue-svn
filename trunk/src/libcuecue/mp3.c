/* libcuecue - (c) 2004 Gautier Portet < kassoulet (ä) users.berlios.de >
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifdef USE_MP3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mad.h>

#include "cuecue.h"
#include "cuecue_internal.h"

typedef struct Internal
{
	FILE *src;
	FILE *dst;
	unsigned char *buffer;
	unsigned char *pcm;
	int src_size;
	int src_position;
	PROGRESS_CALLBACK callback;
	int error;
} Internal;


/* some code taken from madplay - copyright (C) 2000-2004 Robert Leslie */

struct audio_stats
{
	unsigned long clipped_samples;
	mad_fixed_t peak_clipping;
	mad_fixed_t peak_sample;
};


struct audio_dither
{
	mad_fixed_t error[3];
	mad_fixed_t random;
};


static struct audio_dither left_dither, right_dither;


static
unsigned long prng(unsigned long state)
{
	return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

signed long audio_linear_dither(unsigned int bits, mad_fixed_t sample,
		struct audio_dither *dither,
		struct audio_stats *stats)
{
	unsigned int scalebits;
	mad_fixed_t output, mask, random;

	enum {
		MIN = -MAD_F_ONE,
		MAX =  MAD_F_ONE - 1
	};

	/* noise shape */
	sample += dither->error[0] - dither->error[1] + dither->error[2];

	dither->error[2] = dither->error[1];
	dither->error[1] = dither->error[0] / 2;

	/* bias */
	output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

	scalebits = MAD_F_FRACBITS + 1 - bits;
	mask = (1L << scalebits) - 1;

	/* dither */
	random  = prng(dither->random);
	output += (random & mask) - (dither->random & mask);

	dither->random = random;

	/* clip */
	if (output >= stats->peak_sample) {
		if (output > MAX) {
			++stats->clipped_samples;
			if (output - MAX > stats->peak_clipping)
				stats->peak_clipping = output - MAX;

			output = MAX;

			if (sample > MAX)
				sample = MAX;
		}
		stats->peak_sample = output;
	}
	else if (output < -stats->peak_sample) {
		if (output < MIN) {
			++stats->clipped_samples;
			if (MIN - output > stats->peak_clipping)
				stats->peak_clipping = MIN - output;

			output = MIN;

			if (sample < MIN)
				sample = MIN;
		}
		stats->peak_sample = -output;
	}

	/* quantize */
	output &= ~mask;

	/* error feedback */
	dither->error[0] = sample - output;

	/* scale */
	return output >> scalebits;
}


unsigned int audio_pcm_s16le(unsigned char *data, unsigned int nsamples,
		mad_fixed_t const *left, mad_fixed_t const *right,
		struct audio_stats *stats)
{
	unsigned int len;
	register signed int sample0, sample1;

	len = nsamples;

	while (len--) {
		sample0 = audio_linear_dither(16, *left++,  &left_dither,  stats);
		sample1 = audio_linear_dither(16, *right++, &right_dither, stats);

		data[0] = sample0 &255; /* >>0*/
		data[1] = sample0 >> 8;
		data[2] = sample1 &255;
		data[3] = sample1 >> 8;

		data += 4;
	}

	return nsamples * 2 * 2;
}


static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
	if ( MAD_RECOVERABLE(stream->error) ) {
		return MAD_FLOW_CONTINUE;
	}

	snprintf(cuecue_error, CUECUE_ERROR_LENGTH,"MP3 decoding error (%s)", mad_stream_errorstr(stream));

	return MAD_FLOW_BREAK; /* stop decoding (and propagate an error) */
}


static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
	Internal *internal;
	int size;
	unsigned int unconsumedBytes;
	internal = data;

	size = internal->src_size-internal->src_position;

	if (size>BUFFER_SIZE) {
		size = BUFFER_SIZE;
	}

	if (size<=0) {
		return MAD_FLOW_STOP;
	}

	/* "Each time you refill your buffer, you need to preserve the data in
	*  your existing buffer from stream.next_frame to the end.
	*
	*  This usually amounts to calling memmove() on this unconsumed portion
	*  of the buffer and appending new data after it, before calling
	*  mad_stream_buffer()"
	*           -- Rob Leslie, on the mad-dev mailing list */

	if(stream->next_frame) {
		unconsumedBytes = internal->buffer + BUFFER_SIZE - stream->next_frame;
		memmove(internal->buffer, stream->next_frame, unconsumedBytes);
	} else {
		unconsumedBytes = 0;
	}

	size = fread(internal->buffer+unconsumedBytes, 1, size-unconsumedBytes, internal->src);

	mad_stream_buffer(stream, internal->buffer, size+unconsumedBytes);

	internal->src_position += size;

	if (internal->callback!=NULL) {
		internal->callback((float)internal->src_position/internal->src_size);
	}

	return MAD_FLOW_CONTINUE;
}


static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
	Internal *internal=data;
	struct audio_stats stats;
	int written;

	/* convert to pcm */
	audio_pcm_s16le(internal->pcm, pcm->length, pcm->samples[0], pcm->samples[1], &stats);

	/* and write*/
	written = fwrite(internal->pcm, 1, pcm->length*4, internal->dst);

	if (written!=pcm->length*4) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Not enough disk space");
		internal->error=1;
		return MAD_FLOW_STOP;
	}

	return MAD_FLOW_CONTINUE;
}

int DecodeMP3(char* file_source, char* file_destination, PROGRESS_CALLBACK callback)
{
	struct mad_decoder decoder;
	Internal internal;

	internal.src = fopen(file_source, "rb");
	if (internal.src==NULL) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s'",file_source);
		return 0;
	}

	internal.dst = fopen(file_destination, "wb");
	if (internal.dst==NULL) {
		fclose(internal.src);
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s' for writing",file_destination);
		return 0;
	}

	internal.src_position=0;
	internal.error=0;

	/* source file size */
	fseek(internal.src, 0, SEEK_END);
	internal.src_size=ftell(internal.src);
	fseek(internal.src, 0, SEEK_SET);

	internal.buffer = malloc(BUFFER_SIZE+MAD_BUFFER_GUARD);
	internal.pcm = malloc(BUFFER_SIZE+MAD_BUFFER_GUARD);
	internal.callback = callback;

	mad_decoder_init(&decoder, &internal,
			input, 0 /* header */, 0 /* filter */, output,
			error, 0 /* message */);

	mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);

	fclose(internal.src);
	fclose(internal.dst);

	free(internal.buffer);
	free(internal.pcm);

	if (internal.error) {
		return 0;
	}

	return 1;
}

#endif /* USE_MP3 */


