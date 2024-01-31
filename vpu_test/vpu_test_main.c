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

//#include <glib.h>
//#include <glib/gprintf.h>

#include "vpu_test_common.h"
#include "vpu_test_encoder.h"
#include "vpu_test_decoder.h"
#include "vpu_script_test.h"

#define DLOGI(...) 		DLOG(__VA_ARGS__)	//info
#define DLOGE(...) 		DLOG(__VA_ARGS__) 	//error

#define MAX_ENCODER_SUPPORT		4
#define MAX_DECODER_SUPPORT		4
#define MAX_ARG_LENGTH			1024

typedef struct tagMainContext
{
    int nb_of_encoder;
    venc_test_h enc_handle[MAX_ENCODER_SUPPORT];
	char* enc_args[MAX_ENCODER_SUPPORT][MAX_ARG_LENGTH];

	int nb_of_decoder;
    venc_test_h dec_handle[MAX_ENCODER_SUPPORT];
	char* dec_args[MAX_ENCODER_SUPPORT][MAX_ARG_LENGTH];

	bool isBackground;
} MainContext_t;


#define BACKTRACE_MAX   10
void segfault_handler(int sig)
{
    void *callstack[128];
    int i, frames;
    char **strs;

    frames = backtrace(callstack, 128);
    strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; i++)
	{
        DLOGI("%s", strs[i]);
	}

    free(strs);
}

static void help(void)
{
    DLOGI("Usage:");
    DLOGI("  vpu_test -e filename width height -v h264|mpeg4|h263|hevc|mjpg -log level -output");
	DLOGI("  vpu_test -d filename width height -v h264|mpeg4|h263|hevc|mjpg -log level -output");
    DLOGI("");
    DLOGI("\tex) vpu_test -e first.yuv 320 240 -v h264 -e second.yuv 720 480 -v mpeg4 -log 2 -output");
	DLOGI("\tex) vpu_test -d first.h264 320 240 -v h264 -d second.hevc 720 480 -v hevc -log 2 -output");
    DLOGI("");
    DLOGI("\tAdd arguments with \"-e\" or \"-d\" if more encoding or decoding is required.");
    DLOGI("\tArguments \"-e\" or \"-d\" with filename, width, height and \"-v\" with codec are required. ");
	DLOGI("\t -e : [mandatory] encoding source file with width, height");
	DLOGI("\t -d : [mandatory] decoding source file with width, height");
	DLOGI("\t -v : [mandatory] codec name [h264|mpeg4|h263|hevc|mjpg]");
	DLOGI("\t -log : [option] set display log level ");
	DLOGI("\t -output : [option] enable output dump");
    DLOGI("");
	DLOGI("VPU Script Usage:");
	DLOGI("  vpu_test test_script.txt");
	DLOGI("set log level");
	DLOGI("  export VS_LOG=4  #display log under debug level");
	DLOGI("  export VS_LOG=1  #display log under debug error");
    DLOGI("");
}

static int parseCommand(MainContext_t* mainCtx, int argc, char **argv)
{
	int ii;
	int arg_idx;
	int encoder_count = 0;
	int decoder_count = 0;
	int length = 0;

    int is_encoding = 0; // Flag to indicate if we are processing encoder arguments
    int is_decoding = 0; // Flag to indicate if we are processing decoder arguments

    for (ii = 1; ii < argc; ii++)
	{
        if (strcmp(argv[ii], "-e") == 0)
		{
			is_encoding = 1;
            is_decoding = 0;
			encoder_count++;

			if((strlen(mainCtx->enc_args[encoder_count - 1]) + strlen(argv[ii]) + 1) < MAX_ARG_LENGTH)
			{
				strcat(mainCtx->enc_args[encoder_count - 1], argv[ii]);
				strcat(mainCtx->enc_args[encoder_count - 1], " ");
				//DLOGI("encoder %d, args:%s", encoder_count, mainCtx->enc_args[encoder_count - 1]);
			}
        }
		else if (strcmp(argv[ii], "-d") == 0)
		{
			is_encoding = 0;
            is_decoding = 1;
			decoder_count++;

			if((strlen(mainCtx->dec_args[decoder_count - 1]) + strlen(argv[ii]) + 1) < MAX_ARG_LENGTH)
			{
				strcat(mainCtx->dec_args[decoder_count - 1], argv[ii]);
				strcat(mainCtx->dec_args[decoder_count - 1], " ");
				//DLOGI("decoder %d, args:%s", encoder_count, mainCtx->dec_args[decoder_count - 1]);
			}
        }
		else if (is_encoding == 1)
		{
			if((strlen(mainCtx->enc_args[encoder_count - 1]) + strlen(argv[ii]) + 1) < MAX_ARG_LENGTH)
			{
				strcat(mainCtx->enc_args[encoder_count - 1], argv[ii]);
				strcat(mainCtx->enc_args[encoder_count - 1], " ");
				//DLOGI("encoder %d, args:%s", encoder_count, mainCtx->enc_args[encoder_count - 1]);
			}
        }
		else if (is_decoding == 1)
		{
			if((strlen(mainCtx->dec_args[decoder_count - 1]) + strlen(argv[ii]) + 1) < MAX_ARG_LENGTH)
			{
				strcat(mainCtx->dec_args[decoder_count - 1], argv[ii]);
				strcat(mainCtx->dec_args[decoder_count - 1], " ");
				//DLOGI("decoder %d, args:%s", encoder_count, mainCtx->dec_args[decoder_count - 1]);
			}
        }
    }

	mainCtx->nb_of_encoder = encoder_count;
	mainCtx->nb_of_decoder = decoder_count;

	// Print extracted arguments
    DLOGI("Encoder Args:%d\n", mainCtx->nb_of_encoder);
    for (ii = 0; ii < mainCtx->nb_of_encoder; ii++)
	{
        DLOGI("[%d]:%s\n", ii, mainCtx->enc_args[ii]);
    }

    DLOGI("Decoder Args:%d\n", mainCtx->nb_of_decoder);
    for (ii = 0; ii < mainCtx->nb_of_decoder; ii++)
	{
        DLOGI("[%d]:%s\n", ii, mainCtx->dec_args[ii]);
    }

    return (mainCtx->nb_of_encoder + mainCtx->nb_of_decoder);
}

//call encoder test create with each cli parameter as order in cli
//call decoder test create with each cli parameter
int cmd_processing(MainContext_t* mainCtx, int argc, char **argv)
{
	int ii;
	char cmd;

	(void)parseCommand(mainCtx, argc, argv);

    //create encoder
	DLOGI("create number of encoder:%d, decoder:%d", mainCtx->nb_of_encoder, mainCtx->nb_of_decoder);
    for(ii=0; ii<mainCtx->nb_of_encoder; ii++)
    {
		DLOGI("try to create encoder %d with %s\n", ii, mainCtx->enc_args[ii]);
		mainCtx->enc_handle[ii] = venc_test_create(mainCtx->enc_args[ii]);
    }

	//create decoder
	for(ii=0; ii<mainCtx->nb_of_decoder; ii++)
    {
		DLOGI("try to create decoder %d with %s\n", ii, mainCtx->dec_args[ii]);
		mainCtx->dec_handle[ii] = vdec_test_create(mainCtx->dec_args[ii]);
    }

	//create encoder
	DLOGI("start encoding test, nb test:%d", mainCtx->nb_of_encoder);
	for(ii=0; ii<mainCtx->nb_of_encoder; ii++)
	{
		venc_test_start(mainCtx->enc_handle[ii]);
	}

	DLOGI("start decoding test, nb test:%d", mainCtx->nb_of_decoder);
	for(ii=0; ii<mainCtx->nb_of_decoder; ii++)
	{
		vdec_test_start(mainCtx->dec_handle[ii]);
	}

    //command process
    while(1)
    {
        cmd = getchar();
        if(cmd == 'q')
        {
            DLOGI("stop");
            break;
        }
    }

	DLOGI("stop encoder/decoder");

    //stop decoding
    for(ii=0; ii<mainCtx->nb_of_decoder; ii++)
    {
		if(mainCtx->dec_handle[ii] != NULL)
		{
			vdec_test_stop(mainCtx->dec_handle[ii]);
		}
    }

	//stop encoding
	for(ii=0; ii<mainCtx->nb_of_encoder; ii++)
    {
		if(mainCtx->enc_handle[ii] != NULL)
		{
			venc_test_stop(mainCtx->enc_handle[ii]);
		}
    }

	DLOGI("destroy encoder/decoder");

    //destroy
    for(ii=0; ii<mainCtx->nb_of_decoder; ii++)
    {
		if(mainCtx->dec_handle[ii] != NULL)
		{
			vdec_test_destroy(mainCtx->dec_handle[ii]);
		}
    }

	for(ii=0; ii<mainCtx->nb_of_encoder; ii++)
    {
		if(mainCtx->enc_handle[ii] != NULL)
		{
			venc_test_destroy(mainCtx->enc_handle[ii]);
		}
    }

	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;

	printf("[%s:%d] VPU test app, build date:%s, time:%s\n", __FUNCTION__, __LINE__, __DATE__, __TIME__);

	signal(SIGSEGV, segfault_handler);

	if(argc < 5)
    {
		if(argc == 2)
		{
			vpu_command_parsing_test(argc, argv);
		}
		else
		{
			help();
			ret = -1;
		}
    }
	else
	{
		int ii;
		MainContext_t* mainCtx = malloc(sizeof(MainContext_t));
		if(mainCtx != NULL)
		{
			memset(mainCtx, 0x00, sizeof(MainContext_t));

			pid_t fg = tcgetpgrp(STDIN_FILENO);
			if(fg == -1)
			{
				printf("Piped\n");
			}
			else if (fg == getpgrp())
			{
				printf("foreground\n");
				mainCtx->isBackground = false;
			}
			else
			{
				printf("background\n");
				mainCtx->isBackground = true;
			}

			cmd_processing(mainCtx, argc, argv);
		}
	}

    DLOGI("Exit VPU Test");
    return 0;
}
