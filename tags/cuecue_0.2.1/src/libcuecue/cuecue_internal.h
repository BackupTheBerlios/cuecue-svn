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

#ifndef __CUECUE_INTERNAL__
#define __CUECUE_INTERNAL__

#include <stdio.h>
#include <stdlib.h>

/*
  we only handle 44100Hz signed 16bits stereo format.
  sizes are always expressed in BYTES
*/

#define BUFFER_SIZE 0xffff

typedef enum DecoderType
{
#ifdef USE_MP3
	DECODER_MP3,
#endif
#ifdef USE_OGG
	DECODER_OGG,
#endif
#ifdef USE_FLAC
	DECODER_FLAC,
#endif
	DECODER_MAX
} DecoderType;



#ifdef USE_MP3
    int DecodeMP3(char* file_source, char* file_destination, PROGRESS_CALLBACK callback);
#endif
#ifdef USE_OGG
    int DecodeOGG(char* file_source, char* file_destination, PROGRESS_CALLBACK callback);
#endif
#ifdef USE_FLAC
    int DecodeFLAC(char* file_source, char* file_destination, PROGRESS_CALLBACK callback);
#endif

typedef int (*FUNC_DECODER)(char* file_source, char* file_destination, PROGRESS_CALLBACK callback);

#define CUECUE_ERROR_LENGTH 1024
char cuecue_error[CUECUE_ERROR_LENGTH];

#endif /* __CUECUE_INTERNAL__ */

