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


#include <time.h>

#include "vid_test_tools.h"


long long vt_tools_get_time(void)
{
	long long ret_time;
	struct timespec now;

	(void)clock_gettime(CLOCK_MONOTONIC, &now);
	ret_time = (now.tv_sec * 1000000L) + (now.tv_nsec / 1000);

	return ret_time;
}

void vt_write_a_result(TUCR_Handle_T *ptTUCRHandle, char *strMainCat, char *strSubCat, int iError)
{
	#ifdef GET_CSV_REPORT_INCLUDE
	TUCR_Set_T tReportSet = {0, };

	tReportSet.iIsFailed = iError;
	(void)strcpy(tReportSet.strMainCat, strMainCat);
	(void)strcpy(tReportSet.strSubCat, strSubCat);
	tucr_set_a_test_case_result(ptTUCRHandle, &tReportSet);
	#else
	(void)ptTUCRHandle;
	(void)strMainCat;
	(void)strSubCat;
	(void)iError;
	#endif
}