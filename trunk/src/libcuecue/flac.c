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


#ifdef USE_FLAC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FLAC/all.h"

#include "cuecue.h"
#include "cuecue_internal.h"

typedef struct Internal
{
	FILE *dst;
	unsigned char *buffer;
	FLAC__uint64 size;
	FLAC__uint64 position;
	PROGRESS_CALLBACK callback;
	int error;
} Internal;


void error(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	Internal *internal = client_data;
	switch (status) {
		/* is this minor ?
		case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
			break;*/
		case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
			strcpy(cuecue_error,"Bad Header in FLAC file");
			internal->error=1;
			break;
		case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
			strcpy(cuecue_error,"CRC Mismatch: FLAC file corrupted");
			internal->error=1;
			break;
		default:
			{}
	}
}


void metadata(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* get size of the file, for progress callback */
		Internal *internal = client_data;
		internal->size = metadata->data.stream_info.total_samples;
	}
}


FLAC__StreamDecoderWriteStatus write(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	Internal *internal = client_data;
	unsigned int samples = frame->header.blocksize;
	int i,pos;
	int written;

	if (internal->error) {
		/* arg, we found an error, abort decoding */
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	pos=0;

	for(i=0; i < samples; i++) {
		/* TODO: dithering */
		FLAC__int32 value;

		/* left */
		value = (buffer[0][i]) << (16-frame->header.bits_per_sample);
		internal->buffer[pos++]=value>>8;
		internal->buffer[pos++]=value&0xFF;

		/* right */
		value = (buffer[1][i]) << (16-frame->header.bits_per_sample);
		internal->buffer[pos++]=value>>8;
		internal->buffer[pos++]=value&0xFF;

		if (pos>=BUFFER_SIZE-4) {
			/* flush buffer */
			fwrite(internal->buffer,1,pos,internal->dst);
			pos=0;
		}
	}

	written = fwrite(internal->buffer,1,pos,internal->dst);

	if (written!=pos) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Not enough disk space");
		internal->error=1;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}


	internal->position += samples;

	if (internal->callback!=NULL) {
		if (internal->size>0) {
			internal->callback((float)internal->position/internal->size);
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


int DecodeFLAC(char* file_source, char* file_destination, PROGRESS_CALLBACK callback)
{
	FILE *dst;
	Internal internal;
	FLAC__FileDecoder *decoder;

	dst = fopen(file_destination, "wb");

	if (dst==NULL) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s' for writing",file_destination);
		return 0;
	}

	internal.callback=callback;
	internal.position=0;
	internal.size=0;
	internal.error=0;
	internal.dst=dst;

	decoder = FLAC__file_decoder_new();

	if (decoder==NULL) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot create FLAC decoder! memory error ?");
		return 0;
	}

	internal.buffer = malloc(BUFFER_SIZE);

	FLAC__file_decoder_set_md5_checking(decoder, true);
	FLAC__file_decoder_set_filename(decoder, file_source);

	FLAC__file_decoder_set_write_callback(decoder, write);
	FLAC__file_decoder_set_error_callback(decoder, error);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata);
	FLAC__file_decoder_set_client_data(decoder, &internal);

	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK) {
		snprintf(cuecue_error,1024,"Cannot open '%s'",file_source);
		internal.error=1;
	}

	if(!FLAC__file_decoder_process_until_end_of_file(decoder)) {
		if (!internal.error) {
			/* early error, not even the time to decode metadata :) */
			snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s'",file_source);
		}
		internal.error=1;
	}

	FLAC__file_decoder_finish(decoder);
	FLAC__file_decoder_delete(decoder);

	fclose(dst);
	free(internal.buffer);

	if (internal.error) {
		return 0;
	}

	return 1;
}

#endif /* USE_FLAC */
