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
#include <stdbool.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "test_common.h"

#ifdef ENCODER_TEST_INCLUDE
#include "enc_test.h"
#endif

#ifdef DECODER_TEST_INCLUDE
#include "dec_test.h"
#endif

#ifdef ENCODER_TEST_INCLUDE
#define RUN_ENC (0)
#endif

#ifdef DECODER_TEST_INCLUDE
#define RUN_DEC (1)
#endif


typedef enum
{
	Stress_Tester_Dec_H264_FHD = 0,
	Stress_Tester_Dec_H265_UHD,
	Stress_Tester_Dec_JPEG_MAX_RES,
	Stress_Tester_Dec_Max
} Stress_Tester_Dec_Type_E;

typedef enum
{
	Stress_Tester_Enc_H264_FHD = 0,
	Stress_Tester_Enc_Max
} Stress_Tester_Enc_Type_E;

typedef struct
{
	bool bIsExit;
	pthread_t hExitThread;

#if (RUN_DEC == 1)
	int iDecTryIdx;
	VideoDecoder_Tester_Init_T *ptDecInit[Stress_Tester_Dec_Max];
	VideoDecoder_Tester_T *ptDecTester[Stress_Tester_Dec_Max];
	pthread_t hDecTestsThreads[Stress_Tester_Dec_Max];
#endif

#if (RUN_ENC == 1)
	int iEncTryIdx;
	VideoEncoder_Tester_Init_T *ptEncInit[Stress_Tester_Enc_Max];
	VideoEncoder_Tester_T *ptEncTester[Stress_Tester_Enc_Max];
	pthread_t hEncTestsThreads[Stress_Tester_Enc_Max];
#endif
} Stress_Tester_T;


static VID_Tester_Type_E stress_test_get_dec_test_type(Stress_Tester_Dec_Type_E eType);
static VID_Tester_Type_E stress_test_get_enc_test_type(Stress_Tester_Enc_Type_E eType);

static void *stress_test_check_exit(void *param);
static void *stress_test_run_dec_tests(void *param);
static void *stress_test_run_enc_tests(void *param);


int main(int argc, const char *argv[])
{
	int ret = 0;
	Stress_Tester_T tStressTester = {0, };

#if (RUN_DEC == 1)
	VideoDecoder_Tester_Init_T tDecInit[Stress_Tester_Dec_Max] = {0, };
	VideoDecoder_Tester_T tDecTester[Stress_Tester_Dec_Max] = {0, };
#endif

#if (RUN_ENC == 1)
	VideoEncoder_Tester_Init_T tEncInit[Stress_Tester_Dec_Max] = {0, };
	VideoEncoder_Tester_T tEncTester[Stress_Tester_Dec_Max] = {0, };
#endif

	(void)argc;
	(void)argv;

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT(" TEST STARTS!: %d.%d.%d \n", TEST_MAJOR_VER, TEST_MINOR_VER, TEST_PATACH_VER);
	TEST_COMMON_PRINT("========================================================== \n");

	(void)pthread_create(&tStressTester.hExitThread, NULL, stress_test_check_exit, &tStressTester);

#if (RUN_DEC == 1)
	for (int i = 0; i < (int)Stress_Tester_Dec_Max; i++) {
		tDecInit[i].iMaxCnt 	= -1;
		tDecInit[i].eType 		= stress_test_get_dec_test_type(i);
		tDecInit[i].iIndex 		= i;

		tDecTester[i].bIsRunning = false;

		tStressTester.ptDecInit[i] 		= &tDecInit[i];
		tStressTester.ptDecTester[i] 	= &tDecTester[i];
	}

	// for test
	//(void)g_snprintf(tDecInit[Stress_Tester_Dec_H264_FHD].strInputFileName, TEST_COMMON_PATH_STR, "/home/root/test/1920X1080.h264");
	//tStressTester.ptDecInit[Stress_Tester_Dec_H264_FHD]->iMaxCnt 		= 100;
	//tStressTester.ptDecInit[Stress_Tester_Dec_H265_UHD]->iMaxCnt 		= 100;
	//tStressTester.ptDecInit[Stress_Tester_Dec_JPEG_MAX_RES]->iMaxCnt 	= 100;

	for (int i = 0; i < (int)Stress_Tester_Dec_Max; i++) {
		tStressTester.iDecTryIdx = i;
		(void)pthread_create(&tStressTester.hDecTestsThreads[i], NULL, stress_test_run_dec_tests, &tStressTester);
		(void)usleep(20 * 1000);
	}
#endif

#if (RUN_ENC == 1)
	for (int i = 0; i < (int)Stress_Tester_Enc_Max; i++) {
		tEncInit[i].iMaxCnt 	= -1;
		tEncInit[i].eType 		= stress_test_get_enc_test_type(i);
		tEncInit[i].iIndex 		= i;

		tEncTester[i].bIsRunning = false;

		tStressTester.ptEncInit[i] 		= &tEncInit[i];
		tStressTester.ptEncTester[i] 	= &tEncTester[i];
	}

	// for test
	//(void)g_snprintf(tEncInit[Stress_Tester_Dec_H264_FHD].strInputFileName, TEST_COMMON_PATH_STR, "/home/root/test/1920X1080.yuv");
	//tStressTester.ptEncInit[Stress_Tester_Enc_H264_FHD]->iMaxCnt = 100;

	for (int i = 0; i < (int)Stress_Tester_Enc_Max; i++) {
		tStressTester.iEncTryIdx = i;
		(void)pthread_create(&tStressTester.hEncTestsThreads[i], NULL, stress_test_run_enc_tests, &tStressTester);
		(void)usleep(20 * 1000);
	}
#endif

	while (tStressTester.bIsExit == false) {
		(void)usleep(20 * 1000);
	}

	(void)pthread_join(tStressTester.hExitThread, NULL);

#if (RUN_DEC == 1)
	for (int i = 0; i < (int)Stress_Tester_Dec_Max; i++) {
		(void)pthread_join(tStressTester.hDecTestsThreads[i], NULL);
	}
#endif

#if (RUN_ENC == 1)
	for (int i = 0; i < (int)Stress_Tester_Enc_Max; i++) {
		(void)pthread_join(tStressTester.hEncTestsThreads[i], NULL);
	}
#endif

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT(" TEST ENDS!: %d \n", ret);
	TEST_COMMON_PRINT("========================================================== \n");

	return ret;
}

static VID_Tester_Type_E stress_test_get_enc_test_type(Stress_Tester_Enc_Type_E eType)
{
	VID_Tester_Type_E eRet = VID_Tester_Type_None;

	switch (eType) {
		case Stress_Tester_Enc_H264_FHD:
			eRet = VID_Tester_Type_H264_FHD;
			break;

		default:
			break;
	}

	return eRet;
}

static VID_Tester_Type_E stress_test_get_dec_test_type(Stress_Tester_Dec_Type_E eType)
{
	VID_Tester_Type_E eRet = VID_Tester_Type_None;

	switch (eType) {
		case Stress_Tester_Dec_H264_FHD:
			eRet = VID_Tester_Type_H264_FHD;
			break;

		case Stress_Tester_Dec_H265_UHD:
			eRet = VID_Tester_Type_H265_UHD;
			break;

		case Stress_Tester_Dec_JPEG_MAX_RES:
			eRet = VID_Tester_Type_JPEG_MAX_RES;
			break;

		default:
			break;
	}

	return eRet;
}

static void *stress_test_run_dec_tests(void *param)
{
#if (RUN_DEC == 1)
	Stress_Tester_T *ptStressTester = (Stress_Tester_T *)param;

	if (ptStressTester != NULL) {

		int iDecTryIdx = ptStressTester->iDecTryIdx;

		VideoDecoder_Tester_Init_T *ptDecInit 	= ptStressTester->ptDecInit[iDecTryIdx];
		VideoDecoder_Tester_T *ptDecTester 		= ptStressTester->ptDecTester[iDecTryIdx];

		if ((ptDecInit != NULL) && (ptDecTester != NULL)) {
			TEST_COMMON_PRINT("[%s(%d)] RUN: %d \n", __func__, __LINE__, iDecTryIdx);
			(void)vdt_run_test(ptDecInit, ptDecTester);
		}

		TEST_COMMON_PRINT("[%s(%d)] EXIT: %d \n", __func__, __LINE__, iDecTryIdx);

	}
#endif

	return NULL;
}

static void *stress_test_run_enc_tests(void *param)
{
#if (RUN_ENC == 1)
	Stress_Tester_T *ptStressTester = (Stress_Tester_T *)param;

	if (ptStressTester != NULL) {

		int iEncTryIdx = ptStressTester->iEncTryIdx;


		VideoEncoder_Tester_Init_T *ptEncInit 	= ptStressTester->ptEncInit[iEncTryIdx];
		VideoEncoder_Tester_T *ptEncTester 		= ptStressTester->ptEncTester[iEncTryIdx];

		if ((ptEncInit != NULL) && (ptEncTester != NULL)) {
			TEST_COMMON_PRINT("[%s(%d)] RUN: %d \n", __func__, __LINE__, iEncTryIdx);
			(void)vet_run_test(ptEncInit, ptEncTester);
		}

		TEST_COMMON_PRINT("[%s(%d)] EXIT: %d \n", __func__, __LINE__, iEncTryIdx);
	}
#endif

	return NULL;
}

static void *stress_test_check_exit(void *param)
{
	Stress_Tester_T *ptStressTester = (Stress_Tester_T *)param;

	if (ptStressTester != NULL) {
		while (ptStressTester->bIsExit == false) {
			char cInput = getchar();

			if ((cInput == 'q') || (cInput == 'Q')) {
				ptStressTester->bIsExit = true;

			#if (RUN_DEC == 1)
				for (int i = 0; i < (int)Stress_Tester_Dec_Max; i++) {
					if (ptStressTester->ptDecTester[i] == NULL) {
						continue;
					} else {
						VideoDecoder_Tester_T *ptDecTester = ptStressTester->ptDecTester[i];

						if (ptDecTester->bIsRunning == true) {
							ptDecTester->bIsRunning = false;
						}
					}
				}
			#endif

			#if (RUN_ENC == 1)
				for (int i = 0; i < (int)Stress_Tester_Enc_Max; i++) {
					if (ptStressTester->ptEncTester[i] == NULL) {
						continue;
					} else {
						VideoEncoder_Tester_T *ptEncTester = ptStressTester->ptEncTester[i];

						if (ptEncTester->bIsRunning == true) {
							ptEncTester->bIsRunning = false;
						}
					}
				}
			#endif
			}
		}
	}

	TEST_COMMON_PRINT("[%s(%d)] EXIT \n", __func__, __LINE__);

	return NULL;
}