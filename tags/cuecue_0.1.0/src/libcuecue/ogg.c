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


#ifdef USE_OGG

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "cuecue.h"
#include "cuecue_internal.h"

int DecodeOGG(char* file_source, char* file_destination, PROGRESS_CALLBACK callback)
{
	OggVorbis_File ov;
	FILE *src;
	FILE *dst;
	int size=1;
	int bitstream;
	char* buffer;
	int pcm_size;
	int pcm_position=0;
	int error=0;

	src = fopen(file_source, "rb");
	if (src==NULL) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s'",file_source);
		return 0;
	}

	dst = fopen(file_destination, "wb");
	if (dst==NULL) {
		fclose(src);
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s' for writing",file_destination);
		return 0;
	}

	if (ov_open(src, &ov, NULL, 0) < 0) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"'%s' is not a valid Ogg Vorbis file",file_source);
		return 0;
  	}

	buffer = malloc(BUFFER_SIZE);
	pcm_size = 2*ov_pcm_total(&ov, 0)*ov_info(&ov, 0)->channels;

	do {
		int written;
		size = ov_read(&ov, buffer, BUFFER_SIZE, 0, 2, 1, &bitstream);

		if (size<0)  {
			snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"'%s' is a corrupted Ogg Vorbis file",file_source);
			error=1;
		}

		written = fwrite(buffer, 1, size, dst);

		if (written!=size) {
			snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Not enough disk space");
			error=1;
		}

		if (callback!=NULL) {
			pcm_position += size;
			callback((float)pcm_position/pcm_size);
		}

	} while (size>0);

	ov_clear(&ov);

	fclose(src);
	fclose(dst);

	free(buffer);

	if (error) {
		/* error while decoding */
		return 0;
	}

	return 1;
}

#endif /* USE_OGG */
