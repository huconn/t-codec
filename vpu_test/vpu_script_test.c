/*******************************************************************************

*   Copyright (c) Telechips Inc.
*   TCC Version 1.0

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not
limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided "AS IS" and nothing contained in this source code
shall constitute any express or implied warranty of any kind, including without
limitation, any warranty of merchantability, fitness for a particular purpose
or non-infringement of any patent, copyright or other third party intellectual
property right.
No warranty is made, express or implied, regarding the information's accuracy,
completeness, or performance.

In no event shall Telechips be liable for any claim, damages or other
liability arising from, out of or in connection with this source code or
the use in the source code.

This source code is provided subject to the terms of a Mutual Non-Disclosure
Agreement between Telechips and Company.
*
*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>

#include "vpu_script.h"
#include "vpu_script_common.h"

static const char* part_name = "TEST";
#define DLOGD(...) 		DLOG(VS_LOG_DEBUG, part_name, __VA_ARGS__)	//debug
#define DLOGI(...) 		DLOG(VS_LOG_INFO, part_name, __VA_ARGS__)	//info
#define DLOGE(...) 		DLOG(VS_LOG_ERROR, part_name, __VA_ARGS__)	//error

char* readTextFile(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("file open failed\n");
        return NULL;
    }

    //check size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        printf("alloc failed\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int vpu_command_parsing_test(int argc, char** argv)
{
	char* buffer = NULL;

	//vpu_proc_help();

	DLOGI("input:%s", argv[1]);
	buffer = readTextFile(argv[1]);

	if(buffer != NULL)
	{
		vpu_script_main(buffer);
	}

	return 0;
}
