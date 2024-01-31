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
#include <stdint.h>
#include <stdbool.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "video_clock_control.h"


#define MAX_CLOCK_33BIT			(((0x1FFFFFFFF) / 90) * 1000)	// 90kHz (ms)
#define CLOCK_WRAP_DETECT_TERM	(1000000)	// 1sec

#define VCC_CAST(tar, src) 		(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))

typedef enum
{
	VideoClockRegion_A,
	VideoClockRegion_B,
	VideoClockRegion_C
} VideoClockRegion_E;


static VideoClockRegion_E SF_VideoClockCtrl_GetClockRegion(int64_t iClock)
{
	VideoClockRegion_E eRegion = (VideoClockRegion_E)VideoClockRegion_A;

	// |-------------A------------|------------B-----------|-------------C------------|
	// |--CLOCK_WRAP_DETECT_TERM--|------------------------|--CLOCK_WRAP_DETECT_TERM--|
	// |--------------------------------MAX_CLOCK_33BIT-------------------------------|

	if (iClock < CLOCK_WRAP_DETECT_TERM) {
		eRegion = VideoClockRegion_A;
	} else if (iClock > (MAX_CLOCK_33BIT - CLOCK_WRAP_DETECT_TERM)) {
		eRegion = VideoClockRegion_C;
	} else {
		eRegion = VideoClockRegion_B;
	}

	return eRegion;
}

static int SF_VideoClockCtrl_SortClock(const void *a, const void *b)
{
	int ret = 0;
	long lTmpRet = 0L;

	VideoClock_Clock_T *ptClock1 = NULL;
	VideoClock_Clock_T *ptClock2 = NULL;

	VCC_CAST(ptClock1, a);
	VCC_CAST(ptClock2, b);

	if (ptClock1->uiId != ptClock2->uiId) {
		(void)g_printf("[%s(%d)] Different clock ID! - (%d:%d)\n", __func__, __LINE__, ptClock1->uiId, ptClock2->uiId);
		lTmpRet = 0L;
	} else {
		int iRegionGap = 0;

		VideoClockRegion_E eRegion1 = SF_VideoClockCtrl_GetClockRegion(ptClock1->iClock);
		VideoClockRegion_E eRegion2 = SF_VideoClockCtrl_GetClockRegion(ptClock2->iClock);

		if (eRegion1 < eRegion2) {
			iRegionGap = (int)eRegion2 - (int)eRegion1;
		} else {
			iRegionGap = (int)eRegion1 - (int)eRegion2;
		}

		if ((iRegionGap >= 2) && (ptClock1->iClock > ptClock2->iClock)) {
			lTmpRet = ptClock2->iClock - ptClock1->iClock;
		} else {
			lTmpRet = ptClock1->iClock - ptClock2->iClock;
		}
	}

	if (lTmpRet < INT32_MAX) {
		ret = (int)lTmpRet;
	}

	return ret;
}

void VideoClockCtrl_SetFrameRate(VideoClock_T *ptVideoClock, int iFrameRate)
{
	ptVideoClock->iFrameRate = iFrameRate;
}

int VideoClockCtrl_GetClockCount(const VideoClock_T *ptVideoClock)
{
	return ptVideoClock->iClockCount;
}

void VideoClockCtrl_ResetClock(VideoClock_T *ptVideoClock)
{
	ptVideoClock->iFrameRate 			= 30;
	ptVideoClock->iClockCount 			= 0;
	ptVideoClock->iInterlaceFrameIndex 	= -1;

	(void)memset(&ptVideoClock->tPrevOutputClock, 0x00, sizeof(VideoClock_Clock_T));
	(void)memset(&ptVideoClock->tInterlaceFrameClock, 0x00, sizeof(VideoClock_Clock_T));
	(void)memset(&ptVideoClock->tClocks, 0x00, (unsigned int)MAX_VIDEO_CLOCK_COUNT * sizeof(VideoClock_Clock_T));
}

void VideoClockCtrl_InsertClock(VideoClock_T *ptVideoClock, int64_t iClock, unsigned int uiClockId, bool bField, int iFieldIndex, bool bError)
{
	if (bError == true) {
		(void)g_printf("[%s(%d)][ERROR] Reset clock!!\n", __func__, __LINE__);
		//VideoClockCtrl_ResetClock(ptVideoClock);
	} else {
		if (bField == true) {
			ptVideoClock->iInterlaceFrameIndex = iFieldIndex;

			ptVideoClock->tInterlaceFrameClock.iClock 	= iClock;
			ptVideoClock->tInterlaceFrameClock.uiId 	= uiClockId;
		} else {
			if ((ptVideoClock->iInterlaceFrameIndex != -1) && (ptVideoClock->tInterlaceFrameClock.iClock != 0)) {
				if (ptVideoClock->iInterlaceFrameIndex == iFieldIndex) {
					iClock 		= ptVideoClock->tInterlaceFrameClock.iClock;
					uiClockId 	= ptVideoClock->tInterlaceFrameClock.uiId;
				} else {
					(void)g_printf("[%s(%d)] Field index mismatch: %d, %d\n", __func__, __LINE__, ptVideoClock->iInterlaceFrameIndex, iFieldIndex);
				}
			}

			ptVideoClock->iInterlaceFrameIndex = -1;

			(void)memset(&ptVideoClock->tInterlaceFrameClock, 0x00, sizeof(VideoClock_Clock_T));

			if ((ptVideoClock->iClockCount + 1) >= MAX_VIDEO_CLOCK_COUNT) {
				(void)g_printf("[%s(%d)] ClockList overflow\n", __func__, __LINE__);
				VideoClockCtrl_ResetClock(ptVideoClock);
			} else {
				if (ptVideoClock->iClockCount >= 0) {
					ptVideoClock->tClocks[ptVideoClock->iClockCount].iClock 	= iClock;
					ptVideoClock->tClocks[ptVideoClock->iClockCount].uiId 		= uiClockId;
				}

				ptVideoClock->iClockCount++;

				if (ptVideoClock->iClockCount > 1) {
					qsort(ptVideoClock->tClocks, (size_t)ptVideoClock->iClockCount, sizeof(VideoClock_Clock_T), SF_VideoClockCtrl_SortClock);
				}
			}
		}
	}
}

int VideoClockCtrl_GetClock(VideoClock_T *ptVideoClock, VideoClock_Clock_T *ptClock)
{
	int ret = 0;


	if (ptVideoClock->iClockCount == 0) {
		(void)g_printf("[%s(%d)] ClockList empty\n", __func__, __LINE__);
		ret = -1;
	} else {
		int i = 0;

		ptClock->iClock 	= ptVideoClock->tClocks[0].iClock;
		ptClock->uiId 		= ptVideoClock->tClocks[0].uiId;

		ptVideoClock->tPrevOutputClock = ptVideoClock->tClocks[0];

		for (i = 1; i < ptVideoClock->iClockCount; i++) {
			ptVideoClock->tClocks[i - 1] = ptVideoClock->tClocks[i];
		}

		ptVideoClock->iClockCount--;
	}

	return ret;
}