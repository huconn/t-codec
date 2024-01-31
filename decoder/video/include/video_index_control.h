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

#ifndef VIDEO_INDEX_CONTROL_H
#define VIDEO_INDEX_CONTROL_H

typedef enum
{
	VideoIndexCtrl_Config_CallBack,
	VideoIndexCtrl_Config_Driver,
	VideoIndexCtrl_Config_MaxIndexCnt
} VideoIndexCtrl_Config_E;

typedef enum
{
	VideoIndexCtrl_DriverType_None,
	VideoIndexCtrl_DriverType_VSync_V1,
	VideoIndexCtrl_DriverType_VSync_V2
} VideoIndexCtrl_DriverType_E;

typedef struct VideoIndexCtrl_T VideoIndexCtrl_T;

typedef void (*pfnReleaseFrame)(int iDisplayIndex, void *pvUserPrivate);

typedef struct
{
	void *pvUserPrivate;

	pfnReleaseFrame fnReleaseFrame;
} VideoIndexCtrl_CallBack_T;

typedef struct
{
	char strDriver[256];

	VideoIndexCtrl_DriverType_E eType;
} VideoIndexCtrl_Driver_T;

typedef struct
{
	int iMaxIndexCnt;
} VideoIndexCtrl_MaxIndexCnt_T;

int VideoIndexCtrl_Clear(const VideoIndexCtrl_T *ptVideoIndexCtrl);
int VideoIndexCtrl_Drop(const VideoIndexCtrl_T *ptVideoIndexCtrl, int iFrameId);
int VideoIndexCtrl_Flush(const VideoIndexCtrl_T *ptVideoIndexCtrl);
int VideoIndexCtrl_Update(const VideoIndexCtrl_T *ptVideoIndexCtrl);
int VideoIndexCtrl_Push(VideoIndexCtrl_T *ptVideoIndexCtrl, int iDisplayIndex, void *pvAddress);
int VideoIndexCtrl_SetConfig(VideoIndexCtrl_T *ptVideoIndexCtrl, VideoIndexCtrl_Config_E eConfig, void *pvConfigStructure);
int VideoIndexCtrl_Reset(VideoIndexCtrl_T *ptVideoIndexCtrl);

VideoIndexCtrl_T *VideoIndexCtrl_Create(void);
int VideoIndexCtrl_Destroy(VideoIndexCtrl_T *ptVideoIndexCtrl);

#endif	// VIDEO_INDEX_CONTROL_H
