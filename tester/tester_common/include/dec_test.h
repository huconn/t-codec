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


#ifndef DEC_TEST_H
#define DEC_TEST_H


#include "video_decoder.h"

#include "test_common.h"


#define USE_DEC_PERF_REPORT	(0)
#define USE_DEC_DUMP 		(0)


typedef struct
{
	bool bIsInit;

	int iTesterIdx;
	int iMaxCnt;
	bool bIsRunning;

	VideoDecoder_Error_E eDecError;

	VideoDecoder_Codec_E eCodec;
	int iWidth;
	int iHeight;
	int iFrameRate;

	VideoDecoder_T *ptVideoDecoder;
	pthread_t hDecThread;
	char strDecInputFileName[TEST_COMMON_PATH_STR];

	bool bIsInputDumpOn;
	FILE *hInputFile;
	char strInputFileName[TEST_COMMON_PATH_STR];

	bool bIsOutputDumpOn;
	FILE *hOutputFile;
	char strOutputFileName[TEST_COMMON_PATH_STR];

	VID_Tester_Type_E eTest;

	unsigned int uiCnt;

	long long llMinDec;
	long long llMaxDec;
	long long llAccDecDiff;
	long long llAvrDecDiff;

#if (USE_DEC_PERF_REPORT == 1)
	long long *ptDecStart;
	long long *ptDecEnd;
	long long *ptDecDiff;
#endif
} VideoDecoder_Tester_T;

typedef struct
{
	int iIndex; // test index
	VID_Tester_Type_E eType;

	int iMaxCnt; // test count: -1 means infinite
	char strInputFileName[TEST_COMMON_PATH_STR];
} VideoDecoder_Tester_Init_T;


bool vdt_run_test(VideoDecoder_Tester_Init_T *ptInit, VideoDecoder_Tester_T *ptTester);
bool vdt_auto_decoder_test(void);
void vdt_report_test_result(const VideoDecoder_Tester_T *ptTester);

void vdt_init(VideoDecoder_Tester_T *ptTester);
void vdt_deinit(VideoDecoder_Tester_T *ptTester);

void vdt_set_tester(VideoDecoder_Tester_T *ptTester, VideoDecoder_Tester_Init_T *ptInit);
void vdt_write_video_dump(const VideoDecoder_Tester_T *ptTester, bool bIsInput, const unsigned char *pucBuffer, int iBufferSize);
void vdt_dump_result_data(VideoDecoder_Tester_T *ptTester, VideoDecoder_StreamInfo_T *ptStreamInfo, VideoDecoder_Frame_T *ptFrame);

#endif	// DEC_TEST_H