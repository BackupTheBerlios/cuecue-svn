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


#define LINE_MAX 1024
#define FILENAME_MAX 1024

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

char cuecue_error[CUECUE_ERROR_LENGTH]="";

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


static int readline(FILE *f, char *line, int maxlen)
{
	int pos=0;

	*line=0;
	while(1) {
		int c;

		c = fgetc(f);

		if (c==EOF) {
			if (pos) {
				return 1;
			} else {
				return 0;
			}

		}

		if ( (c=='\n') || (c=='\r')) {
			return 1;
		}
		*line++=c;
		*line=0;
		pos++;

		if (pos>=maxlen) {
			return 0;
		}
	}
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

static
int FindFileInCue(char *cue, char *filename)
{
	char line[LINE_MAX];
	FILE *f;
	int end=0;
	int result=0;

	f = fopen(cue,"rb");
	if (f==NULL) {
		return 0;
	}

	do {
		if (!readline(f,line,LINE_MAX)) {
			end=1;
		}
		if (strncmp(line,"FILE",4)==0) {
			/* TODO: fix ugly code */
			char *s_start, *s_end;
			s_start = strchr(line,'\"');
			s_end   = strchr(s_start+1,'\"');

			if (s_end-s_start<1000) {

				strncpy(filename, s_start+1, s_end-s_start-1);
				filename[s_end-s_start-1]=0;

				if (FileExists(filename)) {
					result=1;
				}

				end=1;
			}
		}
	} while(!end);

	fclose(f);
	return result;
}

static
int FindFileInFolder(char *filename, char *cuefile)
{
	char str[FILENAME_MAX];
	char *ext;
	int i;
	int found=0;

	ext = strrchr(filename,'.');

	for(i=0; i<DECODER_MAX; i++) {
		char *ext;

		strcpy(str,filename);
		ext = strrchr(str,'.');
		if (ext!=NULL) {
			strcpy(ext,Extensions[i]);
			if (FileExists(str)) {
				strcpy(cuefile,str);
				found=1;
				return 1;
			}
		}
	}
	return 0;
}


static
int ConvertCueFile(char *original, char *cuecue, char *bin)
{
	char line[LINE_MAX];
	FILE *src;
	FILE *dst;
	int found=0;

	src = fopen(original, "rb");
	if (src==NULL) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s'",original);
		return 0;
	}

	dst = fopen(cuecue, "wb");
	if (dst==NULL) {
		fclose(src);
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"Cannot open '%s' for writing",cuecue);
		return 0;
	}

	while (readline(src,line,LINE_MAX)) {
		if (strncmp(line,"FILE",4)==0) {
			fprintf(dst, "FILE \"%s\" BINARY\r\n", bin);
			found=1;
		} else {
			if (strlen(line)>0) {
				fprintf(dst, "%s\r\n", line);
			}
		}
	}

	fclose(src);
	fclose(dst);

	if (!found) {
		snprintf(cuecue_error,CUECUE_ERROR_LENGTH,"'%s' does not contain a FILE section",cuecue);
		return 0;
	}

	return 1;
}


char * cue_GetError()
{
	return cuecue_error;
}

int cue_ConvertToAudio(char *filename, char *destFolder, PROGRESS_CALLBACK callback)
{
	char *ext;
	char audioFile[FILENAME_MAX];
	char binFile[FILENAME_MAX];
	char cueFile[FILENAME_MAX];
	int result=0;

	cuecue_error[0]=0;

	if (!FileExists(filename)) {
		snprintf(cuecue_error, CUECUE_ERROR_LENGTH,"Cannot open cue file '%s'", filename);
		return 0;
	}

	if (destFolder!=NULL) {
		strcpy(binFile,destFolder);
		strcat(binFile,filename);
	} else {
		strcpy(binFile,filename);
	}
	ext = strrchr(binFile,'.');
	if (ext==NULL) {
		ext=binFile+strlen(binFile);
	}
	strcpy(ext,".audio.bin");

	strcpy(cueFile,filename);
	ext = strrchr(cueFile,'.');
	if (ext==NULL) {
		ext=cueFile+strlen(cueFile);
	}
	strcpy(ext,".audio.cue");

	if ( !FindFileInCue(filename, audioFile)) {
		if ( !FindFileInFolder(filename, audioFile)) {
			snprintf(cuecue_error, CUECUE_ERROR_LENGTH,"Cannot open audio file associed with '%s'", filename);
			return 0;
		}
	}

	if (*audioFile) {
		if ( Decode(audioFile, binFile, callback) ) {
			if ( ConvertCueFile(filename, cueFile, binFile)) {
				result=1;
			}
		}
	}

	return result;
}







