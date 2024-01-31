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
#include <unistd.h> // for close
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <TU_JP_Lib_C_Std.h>
#include <TU_JP_Lib_C_D_LinkedList.h>
#include <TU_JP_Semaphore.h>

#if 0
#include <tccfb_ioctrl.h>
#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
#include <tcc_vsync_ioctl.h>
#endif
#endif

#include "video_index_control.h"


#define VIC_SHORT_STRING_LEN		(32)

#define VIC_DONOTHING(fmt, ...) 	;
#define VIC_CAST(tar, src) 			(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))


typedef struct
{
	int iIndex;
	void *pvAddress;
} VideoIndexCtrl_ListData_T;

struct VideoIndexCtrl_T
{
	int iDevFd;
	int iFrameId;
	int iMaxIndexCnt;

	VideoIndexCtrl_DriverType_E eDriverType;

	VideoIndexCtrl_CallBack_T tCallBack;

	JP_Semaphore_T *hSemaphore;

	JP_D_LinkedList_IF_T *ptListDisplay;
	JP_D_LinkedList_IF_T *ptListClear;
};

static void SF_VideoIndexCtrl_Pop(const VideoIndexCtrl_T *ptVideoIndexCtrl, JP_D_LinkedList_T *hList)
{
	const JP_D_LinkedList_IF_T *ptListDisplay = ptVideoIndexCtrl->ptListDisplay;
	const JP_D_LinkedList_IF_T *ptListClear = ptVideoIndexCtrl->ptListClear;

	void *ptTmpPtr = ptListDisplay->GetListData(hList);
	VideoIndexCtrl_ListData_T *ptListData = NULL;

	VIC_CAST(ptListData, ptTmpPtr);

	(void)ptListClear->InsertList(ptListClear->ptListSt, NULL, ptListData, sizeof(VideoIndexCtrl_ListData_T));
	(void)ptListDisplay->DeleteList(hList);
}

int VideoIndexCtrl_Clear(const VideoIndexCtrl_T *ptVideoIndexCtrl)
{
	int ret = 0;

	if (ptVideoIndexCtrl != NULL) {
		JP_D_LinkedList_T *hList = NULL;
		const JP_D_LinkedList_IF_T *ptList = ptVideoIndexCtrl->ptListClear;

		if (ptVideoIndexCtrl->iMaxIndexCnt < 1) {
			ret = -1;
		} else {
			(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

			hList = ptList->GetFirstList(ptList->ptListSt);

			while (hList != NULL) {
				void *ptTmpPtr = ptList->GetListData(hList);
				VideoIndexCtrl_ListData_T *ptListData = NULL;

				VIC_CAST(ptListData, ptTmpPtr);

				if (ptVideoIndexCtrl->tCallBack.fnReleaseFrame != NULL) {
					ptVideoIndexCtrl->tCallBack.fnReleaseFrame(ptListData->iIndex, ptVideoIndexCtrl->tCallBack.pvUserPrivate);

					(void)ptList->DeleteList(hList);
				}

				hList = ptList->GetFirstList(ptList->ptListSt);
			}

			(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
		}
	} else {
		ret = -1;
	}

	return ret;
}

int VideoIndexCtrl_Drop(const VideoIndexCtrl_T *ptVideoIndexCtrl, int iFrameId)
{
	int ret = 0;

	if (ptVideoIndexCtrl != NULL) {
		char strName[VIC_SHORT_STRING_LEN] = "\n";

		JP_D_LinkedList_T *hList = NULL;
		const JP_D_LinkedList_IF_T *ptList = ptVideoIndexCtrl->ptListDisplay;

		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

		if (iFrameId >= 0) {
			(void)g_sprintf(strName, "%d", iFrameId - 1);
		}

		hList = ptList->FindList(ptList->ptListSt, strName);

		if (hList == NULL) {
			(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
			ret = -1;
		} else {
			SF_VideoIndexCtrl_Pop(ptVideoIndexCtrl, hList);
			(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
		}
	} else {
		ret = -1;
	}

	return ret;
}

int VideoIndexCtrl_Flush(const VideoIndexCtrl_T *ptVideoIndexCtrl)
{
	int ret = 0;

	if (ptVideoIndexCtrl != NULL) {
		JP_D_LinkedList_T *hList = NULL;
		const JP_D_LinkedList_IF_T *ptList = ptVideoIndexCtrl->ptListDisplay;

		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

	#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
		switch ((int)ptVideoIndexCtrl->eDriverType) {
		#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
			case (int)VideoIndexCtrl_DriverType_VSync_V1:
			case (int)VideoIndexCtrl_DriverType_VSync_V2: {
				ioctl(ptVideoIndexCtrl->iDevFd, TCC_LCDC_VIDEO_CLEAR_FRAME, 1);
				break;
			}
		#endif
			default: {
				VIC_DONOTHING(0);
				break;
			}
		}
	#endif

		hList = ptList->GetFirstList(ptList->ptListSt);

		while (hList != NULL) {
			SF_VideoIndexCtrl_Pop(ptVideoIndexCtrl, hList);

			hList = ptList->GetFirstList(ptList->ptListSt);
		}

		(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
	} else {
		ret = -1;
	}

	return ret;
}

int VideoIndexCtrl_Update(const VideoIndexCtrl_T *ptVideoIndexCtrl)
{
	int ret = 0;

	if (ptVideoIndexCtrl != NULL) {
		const JP_D_LinkedList_IF_T *ptList = ptVideoIndexCtrl->ptListDisplay;

		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

		if (ptVideoIndexCtrl->eDriverType == VideoIndexCtrl_DriverType_None) {
			unsigned int uiListCount = 0U;
			unsigned int uiMaxIndexCount = 0U;

			JP_D_LinkedList_T *hList = NULL;

			if (ptVideoIndexCtrl->iMaxIndexCnt >= 0) {
				uiMaxIndexCount = (unsigned int)ptVideoIndexCtrl->iMaxIndexCnt;
			}

			uiListCount = ptList->GetListCount(ptList->ptListSt);

			if (uiListCount < uiMaxIndexCount) {
				(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
				ret = -1;
			} else {
				while (uiListCount >= uiMaxIndexCount) {
					hList = ptList->GetFirstList(ptList->ptListSt);

					SF_VideoIndexCtrl_Pop(ptVideoIndexCtrl, hList);

					uiListCount = ptList->GetListCount(ptList->ptListSt);
				}
			}
		} else {
		#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
			int iFrameId = 0;

			char strName[VIC_SHORT_STRING_LEN] = "\n";

			JP_D_LinkedList_T *hListFrom = NULL;
			JP_D_LinkedList_T *hListTo = NULL;

			switch ((int)ptVideoIndexCtrl->eDriverType) {
			#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
				case (int)VideoIndexCtrl_DriverType_VSync_V1: {
					iFrameId = (int)ioctl(ptVideoIndexCtrl->iDevFd, TCC_LCDC_VIDEO_GET_DISPLAYED, 0);
					break;
				}

				case (int)VideoIndexCtrl_DriverType_VSync_V2: {
					ioctl(ptVideoIndexCtrl->iDevFd, TCC_LCDC_VIDEO_GET_DISPLAYED, &iFrameId);
					break;
				}
			#endif
				default: {
					iFrameId = -1;
					break;
				}
			}

			if (iFrameId < 0) {
				ret = -1;
			} else {
				int iOut = 0;

				(void)g_sprintf(strName, "%d", iFrameId - 1);
				hListTo = ptList->FindList(ptList->ptListSt, strName);

				if (hListTo == NULL) {
					int i = 0;
					int iListCount = ptList->GetListCount(ptList->ptListSt);

					JP_D_LinkedList_T *hList = NULL;

					for (i = 0; i < iListCount; i++) {
						if (i == 0) {
							hList = ptList->GetFirstList(ptList->ptListSt);
						} else {
							hList = ptList->GetNextList(ptList->ptListSt, hList);
						}

						if (atoi(ptList->GetListName(hList)) >= (iFrameId - 1)) {
							break;
						}

						hListTo = hList;
					}

					if (hListTo == NULL) {
						(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
						iOut = 1;
					}
				}

				if (iOut == 0) {
					hListFrom = ptList->GetFirstList(ptList->ptListSt);

					while ((hListFrom != NULL) && (hListFrom != hListTo)) {
						SF_VideoIndexCtrl_Pop(ptVideoIndexCtrl, hListFrom);

						hListFrom = ptList->GetFirstList(ptList->ptListSt);
					}

					SF_VideoIndexCtrl_Pop(ptVideoIndexCtrl, hListFrom);
				}
			}
		#else
			ret = -1;
		#endif
		}

		(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
	} else {
		ret = -1;
	}

	return ret;
}

int VideoIndexCtrl_Push(VideoIndexCtrl_T *ptVideoIndexCtrl, int iDisplayIndex, void *pvAddress)
{
	int iFrameId = 0;

	if (ptVideoIndexCtrl != NULL) {
		char strName[VIC_SHORT_STRING_LEN] = "\n";

		VideoIndexCtrl_ListData_T tListData = {0, };
		const JP_D_LinkedList_IF_T *ptList = ptVideoIndexCtrl->ptListDisplay;

		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);
		(void)g_sprintf(strName, "%d", ptVideoIndexCtrl->iFrameId - 1);

		tListData.iIndex 		= iDisplayIndex;
		tListData.pvAddress 	= pvAddress;

		(void)ptList->InsertList(ptList->ptListSt, strName, &tListData, sizeof(VideoIndexCtrl_ListData_T));

		iFrameId = ptVideoIndexCtrl->iFrameId;
		ptVideoIndexCtrl->iFrameId++;

		(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
	} else {
		iFrameId = -1;
	}

	return iFrameId;
}

int VideoIndexCtrl_SetConfig(VideoIndexCtrl_T *ptVideoIndexCtrl, VideoIndexCtrl_Config_E eConfig, void *pvConfigStructure)
{
	int iRetVal = 0;

	if (ptVideoIndexCtrl != NULL) {
		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

		switch ((int)eConfig) {
			case (int)VideoIndexCtrl_Config_CallBack: {
				VideoIndexCtrl_CallBack_T *ptCallBack = NULL;

				VIC_CAST(ptCallBack, pvConfigStructure);

				ptVideoIndexCtrl->tCallBack.pvUserPrivate 	= ptCallBack->pvUserPrivate;
				ptVideoIndexCtrl->tCallBack.fnReleaseFrame 	= ptCallBack->fnReleaseFrame;
				break;
			}
			case (int)VideoIndexCtrl_Config_Driver: {
				VideoIndexCtrl_Driver_T *ptDriver = NULL;

				VIC_CAST(ptDriver, pvConfigStructure);

				ptVideoIndexCtrl->eDriverType = ptDriver->eType;

				if (ptVideoIndexCtrl->eDriverType != VideoIndexCtrl_DriverType_None) {
					ptVideoIndexCtrl->iDevFd = open(ptDriver->strDriver, O_RDWR);

					if (ptVideoIndexCtrl->iDevFd < 0) {
						iRetVal = -1;
					}
				}
				break;
			}
			case (int)VideoIndexCtrl_Config_MaxIndexCnt: {
				VideoIndexCtrl_MaxIndexCnt_T *ptMaxIndexCnt = NULL;

				VIC_CAST(ptMaxIndexCnt, pvConfigStructure);

				ptVideoIndexCtrl->iMaxIndexCnt = ptMaxIndexCnt->iMaxIndexCnt;
				break;
			}

			default: {
				VIC_DONOTHING(0);
				break;
			}
		}

		(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
	} else {
		iRetVal = -1;
	}

	return iRetVal;
}

int VideoIndexCtrl_Reset(VideoIndexCtrl_T *ptVideoIndexCtrl)
{
	int ret = -1;

	if (ptVideoIndexCtrl != NULL) {
		const JP_D_LinkedList_IF_T *ptListDisplay = NULL;
		const JP_D_LinkedList_IF_T *ptListClear = NULL;

		(void)JP_ObtainSemaphore(ptVideoIndexCtrl->hSemaphore, -1);

		ptListDisplay 	= ptVideoIndexCtrl->ptListDisplay;
		ptListClear 	= ptVideoIndexCtrl->ptListClear;

	#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
		switch ((int)ptVideoIndexCtrl->eDriverType) {
		#if defined(CONFIG_TCC_VIDEO_DISPLAY_BY_VSYNC_INT) || defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
			case (int)VideoIndexCtrl_DriverType_VSync_V1:
			case (int)VideoIndexCtrl_DriverType_VSync_V2: {
				ioctl(ptVideoIndexCtrl->iDevFd, TCC_LCDC_VIDEO_CLEAR_FRAME, 1);
				break;
			}
		#endif
			default: {
				VIC_DONOTHING(0);
				break;
			}
		}
	#endif

		(void)ptListDisplay->DeleteListAll(ptListDisplay->ptListSt);
		(void)ptListClear->DeleteListAll(ptListClear->ptListSt);

		ptVideoIndexCtrl->iFrameId = 2;

		(void)JP_ReleaseSemaphore(ptVideoIndexCtrl->hSemaphore);
	}

	return ret;
}

VideoIndexCtrl_T *VideoIndexCtrl_Create(void)
{
	VideoIndexCtrl_T *ptVideoIndexCtrl = NULL;
	void *ptTmpPtr = g_malloc0(sizeof(VideoIndexCtrl_T));

	if (ptTmpPtr != NULL) {
		VIC_CAST(ptVideoIndexCtrl, ptTmpPtr);

		ptVideoIndexCtrl->iDevFd 	= -1;
		ptVideoIndexCtrl->iFrameId 	= 2;

		ptVideoIndexCtrl->hSemaphore = JP_CreateSemaphore(1, 1);

		ptVideoIndexCtrl->ptListDisplay = JP_CreateList();
		ptVideoIndexCtrl->ptListClear 	= JP_CreateList();
	}

	return ptVideoIndexCtrl;
}

int VideoIndexCtrl_Destroy(VideoIndexCtrl_T *ptVideoIndexCtrl)
{
	int ret = 0;

	if (ptVideoIndexCtrl != NULL) {
		if (ptVideoIndexCtrl->iDevFd >= 0) {
			(void)close(ptVideoIndexCtrl->iDevFd);
		}

		(void)JP_DestroySemaphore(ptVideoIndexCtrl->hSemaphore);

		(void)JP_DestroyList(ptVideoIndexCtrl->ptListDisplay);
		(void)JP_DestroyList(ptVideoIndexCtrl->ptListClear);

		g_free(ptVideoIndexCtrl);
	} else {
		ret = -1;
	}

	return ret;
}