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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cuecue.h"
#include "cuecue_internal.h"


FUNC_DECODER decoders[]=
{
#ifdef USE_MP3
	DecodeMP3,
#endif
#ifdef USE_OGG
	DecodeOGG,
#endif
#ifdef USE_FLAC
	DecodeFLAC,
#endif
};

char Extensions[][6]=
{
#ifdef USE_MP3
	".mp3",
#endif
#ifdef USE_OGG
	".ogg",
#endif
#ifdef USE_FLAC
	".flac"
#endif
};

char cuecue_error[1024]="";

static
int Decode(char* file_source, char* file_destination, PROGRESS_CALLBACK callback)
{
	int i;

	for(i=0; i<DECODER_MAX; i++) {
		char *ext;
		ext = strrchr(file_source,'.');
		if (strcmp(ext,Extensions[i])==0) {
			break;
		}
	}

       	return decoders[i](file_source, file_destination, callback);
}

static
int FindFileInCue(char *cue, char *filename)
{

}

static
int FindFileInFolder(char *filename)
{
}

static
int FileExists(char *filename)
{
	FILE *f;
	f = fopen(filename,"rb");
	if (f==NULL) {
		return 0;
	}
	fclose(f);
	return 1;
}

char * cue_GetError()
{
	return cuecue_error;
}

int cue_ConvertToAudio(char *filename, char *destFolder, PROGRESS_CALLBACK callback)
{
	char *ext;
	char *audioFile=NULL;
	char *binFile=NULL;
	char *str;
	int totalsize;
	int currentsize;
	int previousPercent=0;
	int i;

	cuecue_error[0]=0;

	ext = strrchr(filename,'.');
	str = (char*) malloc(strlen(filename)+1);

	if (destFolder!=NULL) {
		binFile = (char*) malloc(strlen(filename)+strlen(destFolder)+10);
		strcpy(binFile,destFolder);
		strcat(binFile,filename);
	} else {
		binFile = (char*) malloc(strlen(filename)+10);
		strcpy(binFile,filename);
	}
		ext = strrchr(binFile,'.');
		strcpy(ext,".bin");

	for(i=0; i<DECODER_MAX; i++) {
		char *ext;

		strcpy(str,filename);
		ext = strrchr(str,'.');
		strcpy(ext,Extensions[i]);
		if (FileExists(str)) {
			audioFile = str;
			break;
		}
	}

	if (audioFile) {
		if ( ! Decode(audioFile,binFile,callback)) {
			return 0;
		}
	}

	free(str);
	free(binFile);

	return 1;
}







