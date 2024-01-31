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

#if defined (ENCODER_TEST_INCLUDE) && defined(DECODER_TEST_INCLUDE)
#include "vid_test.h"
#endif


static int auto_encode_to_decode_test(void);
static int auto_encode_and_decode_test(void);


int main(int argc, const char *argv[])
{
	int ret = 0;

	(void)argc;
	(void)argv;

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT(" TEST STARTS!: %d.%d.%d \n", TEST_MAJOR_VER, TEST_MINOR_VER, TEST_PATACH_VER);
	TEST_COMMON_PRINT("========================================================== \n");

	ret = auto_encode_and_decode_test();

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT(" TEST ENDS!: %d \n", ret);
	TEST_COMMON_PRINT("========================================================== \n");

	return ret;
}

static int auto_encode_to_decode_test(void)
{
	int ret = 0;

	#if defined (ENCODER_TEST_INCLUDE) && defined(DECODER_TEST_INCLUDE)
	bool bIsVidError = false;

	TEST_COMMON_PRINT(" TEST: Set Encoder Output as Decoder Input\n");

	bIsVidError = vt_video_test();

	if (bIsVidError == true) {
		ret = -1;
	}

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT("  Video Test Result: %d \n", (int)bIsVidError);
	TEST_COMMON_PRINT("========================================================== \n");
	#endif

	return ret;
}

static int auto_encode_and_decode_test(void)
{
	int ret = 0;

	bool bIsEncError = false;
	bool bIsDecError = false;

	TEST_COMMON_PRINT(" TEST: Video Decoder and Encoder \n");

	#ifdef ENCODER_TEST_INCLUDE
	bIsEncError = vet_auto_encoder_test();
	#endif

	#ifdef DECODER_TEST_INCLUDE
	bIsDecError = vdt_auto_decoder_test();
	#endif

	TEST_COMMON_PRINT("========================================================== \n");
	TEST_COMMON_PRINT("  Video Test Result(s) \n");
	TEST_COMMON_PRINT("---------------------------------------------- \n");

	#ifdef ENCODER_TEST_INCLUDE
	TEST_COMMON_PRINT("    - Encoder Test Result: %d \n", (int)bIsEncError);
	#endif

	#ifdef DECODER_TEST_INCLUDE
	TEST_COMMON_PRINT("    - Decoder Test Result: %d \n", (int)bIsDecError);
	#endif

	TEST_COMMON_PRINT("========================================================== \n");

	if ((bIsEncError == true) || (bIsDecError == true)) {
		ret = -1;
	}

	return ret;
}