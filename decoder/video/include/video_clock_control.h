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

#ifndef VIDEO_CLOCK_CONTROL_H
#define VIDEO_CLOCK_CONTROL_H

#define MAX_VIDEO_CLOCK_COUNT	(20)


typedef struct
{
	int64_t iClock;
	unsigned int uiId;
} VideoClock_Clock_T;

typedef struct
{
	int iFrameRate;
	int iClockCount;
	int iInterlaceFrameIndex;

	VideoClock_Clock_T tPrevOutputClock;
	VideoClock_Clock_T tInterlaceFrameClock;
	VideoClock_Clock_T tClocks[MAX_VIDEO_CLOCK_COUNT];
} VideoClock_T;


void VideoClockCtrl_SetFrameRate(VideoClock_T *ptVideoClock, int iFrameRate);
int VideoClockCtrl_GetClockCount(const VideoClock_T *ptVideoClock);
void VideoClockCtrl_ResetClock(VideoClock_T *ptVideoClock);
void VideoClockCtrl_InsertClock(VideoClock_T *ptVideoClock, int64_t iClock, unsigned int uiClockId, bool bField, int iFieldIndex, bool bError);
int VideoClockCtrl_GetClock(VideoClock_T *ptVideoClock, VideoClock_Clock_T *ptClock);

#endif	// VIDEO_CLOCK_CONTROL_H
