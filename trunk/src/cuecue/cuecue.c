/* cuecue - (c) 2004 Gautier Portet < kassoulet (ä) users.berlios.de >
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
 *
 * $Id: $
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <strings.h>

#include "../libcuecue/cuecue.h"

#define VERSIONSTRING "cuecue 0.1.0\n"

struct option long_options[] = {
    {"quiet", 0,0,'q'},
    {"help",0,0,'h'},
    {"version", 0, 0, 'v'},
    {"output", 1, 0, 'o'},
    {NULL,0,0,0}
};


static int quiet = 0;
static char *outfolder = NULL;

static void usage(void) {
    fprintf(stderr, "Usage: cuecue [flags] file.cue\n"
                    "\n"
                    "Supported flags:\n"
                    " --quiet,   -q    Quiet mode. No console output.\n"
                    " --help,    -h    Produce this help message.\n"
                    " --version, -v    Print out version number.\n"
                    " --output,  -o    Output to given folder\n"
            );

}


static void parse_options(int argc, char **argv)
{
    int option_index = 1;
    int ret;

    while((ret = getopt_long(argc, argv, "qhvo:",
                    long_options, &option_index)) != -1)
    {
        switch(ret)
        {
            case 'q':
                quiet = 1;
                break;
            case 'h':
                usage();
                exit(0);
                break;
            case 'v':
                fprintf(stderr, VERSIONSTRING);
                exit(0);
                break;
            case 'o':
                /*outfolder = strdup(optarg);*/
		outfolder = optarg;
                break;
            default:
                fprintf(stderr, "Internal error: Unrecognised argument\n");
                break;
        }
    }
}


void progress(float fprogress)
{
	int i;
	int progress = (int) (fprogress*100.0f);
	static int old_progress=-1;

	if (old_progress==progress) {
		return;
	}
	old_progress=progress;

	printf("[");
	for(i=0; i<progress/2; i++) {
		fputc('=',stdout);
	}
	for( ; i<100/2; i++) {
		fputc(' ',stdout);
	}
	fputc(']',stdout);
	printf(" %2d%%\r",progress);
	fflush(stdout);
}


int main(int argc, char **argv)
{
	int result;

	if(argc == 1) {
		fprintf(stderr, VERSIONSTRING);
		usage();
		return 1;
	}

	parse_options(argc,argv);

	if(!quiet) {
		fprintf(stderr, VERSIONSTRING);
	}

	if(optind >= argc) {
		fprintf(stderr, VERSIONSTRING);
		fprintf(stderr, "ERROR: No input files specified. Use -h for help\n");
		usage();
		return 1;
	}

	fprintf(stderr, "Converting: '%s'\n",argv[optind]);

	result = cue_ConvertToAudio( argv[optind], outfolder, quiet ? NULL : progress );

	if (!result) {
		fprintf(stderr, "ERROR: %s\n",cue_GetError());
	}

	return 0;
}
