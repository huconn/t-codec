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


#ifndef TENC_TEST_H
#define TENC_TEST_H


#include "video_encoder.h"

#include "test_common.h"


#define USE_ENC_PERF_REPORT	(0)
#define USE_ENC_DUMP 		(0)


typedef struct
{
	bool bIsInit;

	int iTesterIdx;
	int iMaxCnt;
	bool bIsRunning;

	VideoEncoder_Codec_E eCodec;
	VideoEncoder_Set_Header_Result_T tHeader;
	VideoEncoder_Error_E eEncError;

	int iWidth;
	int iHeight;

	int iFrameRate;
	int iKeyFrameInterval;
	int iBitRate;

	int iSliceMode;
	int iSliceSizeUnit;
	int iSliceSize;

	unsigned char *pucVideoBuffer[TEST_COMMON_MAX_MEM_BUFFER_CNT];

	VideoEncoder_T *ptVideoEncoder;
	pthread_t hEncThread;
	FILE *hEncInputFile;
	char strEncInputFileName[TEST_COMMON_PATH_STR];

	bool bIsInputDumpOn;
	FILE *hInputFile;
	char strInputFileName[TEST_COMMON_PATH_STR];

	bool bIsOutputDumpOn;
	FILE *hOutputFile;
	char strOutputFileName[TEST_COMMON_PATH_STR];

	FILE *hOutputSizeFile;
	char strOutputSizeFileName[TEST_COMMON_PATH_STR];

	VID_Tester_Type_E eTest;

	unsigned int uiCnt;

	long long llMinEnc;
	long long llMaxEnc;
	long long llAccEncDiff;
	long long llAvrEncDiff;

#if (USE_ENC_PERF_REPORT == 1)
	long long *ptEncStart;
	long long *ptEncEnd;
	long long *ptEncDiff;
#endif
} VideoEncoder_Tester_T;

typedef struct
{
	int iIndex; // test index
	VID_Tester_Type_E eType;

	int iMaxCnt; // test count: -1 means infinite
	char strInputFileName[TEST_COMMON_PATH_STR];

	bool bIsDumpInput;
	bool bIsDumpOutput;
} VideoEncoder_Tester_Init_T;


bool vet_run_test(VideoEncoder_Tester_Init_T *ptTestInit, VideoEncoder_Tester_T *ptTester);
bool vet_auto_encoder_test(void);
void vet_report_test_result(const VideoEncoder_Tester_T *ptTester);

void vet_deinit(VideoEncoder_Tester_T *ptTester);
void vet_encode(VideoEncoder_Tester_T *ptTester);

void vet_set_video_memory(VideoEncoder_Tester_T *ptTester, bool bIsFree);
void vet_set_tester(VideoEncoder_Tester_T *ptTester, VideoEncoder_Tester_Init_T *ptTestInit);

void vet_write_video_dump(const VideoEncoder_Tester_T *ptTester, bool bIsInput, const unsigned char *pucBuffer, int iBufferSize);
int vet_is_data_available(const VideoEncoder_Tester_T *ptTester);

#endif	// TENC_TEST_H