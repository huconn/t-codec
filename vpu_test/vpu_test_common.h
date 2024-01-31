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


#ifndef VPU_TEST_COMMON_H
#define VPU_TEST_COMMON_H

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


#include <time.h>
#define TC_RESET		"\033""[39m"
#define DLOG(fmt, args...) \
		do { \
			struct timespec _now;    \
			struct tm _tm;  \
			clock_gettime(CLOCK_REALTIME, &_now);   \
			localtime_r(&_now.tv_sec, &_tm);     \
			printf("[%04d-%02d-%02d %02d:%02d:%02d.%03ld][%s:%d]" fmt "\n" TC_RESET, \
				_tm.tm_year + 1900, _tm.tm_mon+1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec, _now.tv_nsec/1000000, \
				__FUNCTION__, __LINE__, ## args); \
		}while(0)

long long TCC_GetTimeUs(void);

#endif //VPU_TEST_COMMON_H
