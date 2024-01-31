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


#include <decoder_plugin.h>
#include <decoder_plugin_video.h>
#include <decoder_plugin_video_common.h>


#define MJPEG_MIN_OUTPUT_WIDTH				DEFAULT_VIDEO_MIN_OUTPUT_WIDTH
#define MJPEG_MIN_OUTPUT_HEIGHT				DEFAULT_VIDEO_MIN_OUTPUT_HEIGHT
#define MJPEG_MAX_OUTPUT_WIDTH				DEFAULT_IMAGE_MAX_OUTPUT_WIDTH
#define MJPEG_MAX_OUTPUT_HEIGHT				DEFAULT_IMAGE_MAX_OUTPUT_HEIGHT
#define MJPEG_DEFAULT_FRAMERATE				DEFAULT_VIDEO_DEFAULT_FRAMERATE

#define MJPEG_MAX_FRAME_BUFFER_COUNT		(31)


typedef struct
{
	VPC_DRIVER_INFO_T tDriverInfo;

	bool bInited;
} DEC_Plugin_MJPEG_T;


static int decoder_plugin_mjpeg_AllocMemory(DEC_Plugin_MJPEG_T *ptPlugin);
static int decoder_plugin_mjpeg_FreeMemory(DEC_Plugin_MJPEG_T *ptPlugin);
static int decoder_plugin_mjpeg_DecodeSequenceHeader(DEC_Plugin_MJPEG_T *ptPlugin, DP_Video_Config_SequenceHeader_T *ptSequenceHeader);

static DEC_PLUGIN_H decoder_plugin_mjpeg_Open(void);
static int decoder_plugin_mjpeg_Close(DEC_PLUGIN_H hPlugin);
static int decoder_plugin_mjpeg_Init(DEC_PLUGIN_H hPlugin, void *pvInitConfig);
static int decoder_plugin_mjpeg_Deinit(DEC_PLUGIN_H hPlugin);
static int decoder_plugin_mjpeg_Decode(DEC_PLUGIN_H hPlugin, unsigned int uiStreamLen, unsigned char *pucStream, void *pvDecodingResult);
static int decoder_plugin_mjpeg_Set(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
static int decoder_plugin_mjpeg_Get(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);


static int decoder_plugin_mjpeg_AllocMemory(DEC_Plugin_MJPEG_T *ptPlugin)
{
	int ret = 0;
	int iBitstreamBufSize = 0;

	DPV_FUNC_LOG();

	iBitstreamBufSize = video_plugin_alloc_bitstream_buffer(&ptPlugin->tDriverInfo);

	if (iBitstreamBufSize > 0) {
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}

static int decoder_plugin_mjpeg_FreeMemory(DEC_Plugin_MJPEG_T *ptPlugin)
{
	DPV_FUNC_LOG();

	video_plugin_free_alloc_buffers(&ptPlugin->tDriverInfo);
	(void)video_plugin_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_FREE_MEMORY, NULL);

	return 0;
}

static int decoder_plugin_mjpeg_DecodeSequenceHeader(DEC_Plugin_MJPEG_T *ptPlugin, DP_Video_Config_SequenceHeader_T *ptSequenceHeader)
{
	int iRetVal = 0;

	DPV_FUNC_LOG();

	iRetVal = video_plugin_set_seq_hearder(&ptPlugin->tDriverInfo, ptSequenceHeader);

	if (iRetVal == 0) {
		int iAvailBufSize = ptPlugin->tDriverInfo.iInstIdx + (int)VPK_VPU_DEC;

		video_plugin_get_header_info(&ptPlugin->tDriverInfo);

		if (video_plugin_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_VPU_GET_FREEMEM_SIZE, &iAvailBufSize) < 0) {
			iRetVal = -1;
		} else {
			iRetVal = video_plugin_alloc_frame_buffer(&ptPlugin->tDriverInfo, iAvailBufSize, MJPEG_MAX_FRAME_BUFFER_COUNT);

			#if (DPV_PRINT_DETAIL_INFO == 1)
			DPV_DETAIL_MSG("[%s(%d)] ==================================================================== \n", __func__, __LINE__);
			DPV_DETAIL_MSG("[%s(%d)]  - m_iFrameBufferCount: %d  \n", __func__, __LINE__, ptPlugin->tDriverInfo.iFrameBufferCount);
			DPV_DETAIL_MSG("[%s(%d)]  - iFrameBufferSize: %d  \n", __func__, __LINE__, ptPlugin->tDriverInfo.iFrameBufferSize);
			DPV_DETAIL_MSG("[%s(%d)]  - PicRes: %d X %d \n", __func__, __LINE__, ptPlugin->tDriverInfo.iPicWidth, ptPlugin->tDriverInfo.iPicHeight);
			DPV_DETAIL_MSG("[%s(%d)]  - m_iMinFrameBufferSize : %d \n", __func__, __LINE__, ptPlugin->tDriverInfo.iMinFrameBufferSize);
			DPV_DETAIL_MSG("[%s(%d)] ==================================================================== \n", __func__, __LINE__);
			#endif
		}
	}

	if (iRetVal == 0) {
		VPC_RESOLUTION_INFO_T tMaxResInfo = {MJPEG_MAX_OUTPUT_WIDTH, MJPEG_MAX_OUTPUT_HEIGHT};
		VPC_RESOLUTION_INFO_T tMinResInfo = {MJPEG_MIN_OUTPUT_WIDTH, MJPEG_MIN_OUTPUT_HEIGHT};

		if (video_plugin_is_pic_resolution_ok(&ptPlugin->tDriverInfo, tMaxResInfo, tMinResInfo) < 0) {
			DPV_ERROR_MSG("[%s(%d)] [ERROR] Picture Resolution \n", __func__, __LINE__);
			iRetVal = -1;
		}
	}

	return iRetVal;
}

static DEC_PLUGIN_H decoder_plugin_mjpeg_Open(void)
{
	void *tmpPtr = DPV_CALLOC(sizeof(DEC_Plugin_MJPEG_T));
	DEC_Plugin_MJPEG_T *ptPlugin = NULL;

	DPV_FUNC_LOG();

	if (tmpPtr != NULL) {
		DPV_CAST(ptPlugin, tmpPtr);
	}

	return (DEC_PLUGIN_H)ptPlugin;
}

static int decoder_plugin_mjpeg_Close(DEC_PLUGIN_H hPlugin)
{
	DEC_Plugin_MJPEG_T *ptPlugin = NULL;

	DPV_FUNC_LOG();

	DPV_CAST(ptPlugin, hPlugin);

	if (ptPlugin != NULL) {
		DPV_FREE(ptPlugin);
	}

	return 0;
}


static int decoder_plugin_mjpeg_Init(DEC_PLUGIN_H hPlugin, void *pvInitConfig)
{
	DP_Video_Error_E ret = (DP_Video_Error_E)DP_Video_ErrorNone;

	DEC_Plugin_MJPEG_T *ptPlugin = NULL;
	DP_Video_Config_Init_T *ptInitConfig = NULL;

	DPV_FUNC_LOG();

	DPV_CAST(ptPlugin, hPlugin);
	DPV_CAST(ptInitConfig, pvInitConfig);

	ptPlugin->tDriverInfo.iInstIdx 		= -1;
	ptPlugin->tDriverInfo.uiCodec 		= VPK_STD_MJPG;
	ptPlugin->tDriverInfo.iInstType 	= (int)VPK_VPU_DEC;
	ptPlugin->tDriverInfo.iNumOfInst 	= 0;
	ptPlugin->tDriverInfo.iOpenType 	= (int)VPK_DEC_WITH_ENC;
	ptPlugin->tDriverInfo.iNumOfOpen 	= 1;
	ret = video_plugin_open_drivers(&ptPlugin->tDriverInfo);

	if (ret != (DP_Video_Error_E)DP_Video_ErrorNone) {
		DPV_ERROR_MSG("[%s(%d)] [ERROR] video_plugin_open_drivers() \n", __func__, __LINE__);
		(void)video_plugin_close_drivers(&ptPlugin->tDriverInfo);
	} else {
		VPC_DRIVER_INFO_T *ptDriverInfo = &ptPlugin->tDriverInfo;

		ptDriverInfo->uiRegBaseVirtualAddr 	= 0U;
		ptDriverInfo->iBitstreamFormat 		= VPK_STD_MJPG;
		ptDriverInfo->iPicWidth				= MJPEG_MAX_OUTPUT_WIDTH;
		ptDriverInfo->iPicHeight			= MJPEG_MAX_OUTPUT_HEIGHT;
		ptDriverInfo->bEnableUserData		= 0;
		ptDriverInfo->bCbCrInterleaveMode 	= 1;
		ptDriverInfo->iFilePlayEnable 		= 1;
		ptDriverInfo->iFrameRate 			= MJPEG_DEFAULT_FRAMERATE;
		ptDriverInfo->iBitRateMbps 			= 10;
		ptDriverInfo->iExtFunction 			= 0;

		video_plugin_set_init_info(ptDriverInfo);

		(void)decoder_plugin_mjpeg_AllocMemory(ptPlugin);
		(void)video_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_INIT, NULL);

		video_plugin_set_frame_skip_mode(&ptPlugin->tDriverInfo, VPC_FRAME_SKIP_MODE_NONIDR_SEARCH);
		ptPlugin->bInited = true;
	}

	return (int)ret;
}

static int decoder_plugin_mjpeg_Deinit(DEC_PLUGIN_H hPlugin)
{
	int ret = 0;

	DEC_Plugin_MJPEG_T *ptPlugin = NULL;

	DPV_FUNC_LOG();

	DPV_CAST(ptPlugin, hPlugin);

	if (ptPlugin->bInited == false) {
		ret = -1;
	} else {
		(void)video_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_CLOSE, NULL);
		(void)decoder_plugin_mjpeg_FreeMemory(ptPlugin);

		(void)video_plugin_close_drivers(&ptPlugin->tDriverInfo);
	}

	return ret;
}

static int decoder_plugin_mjpeg_Decode(DEC_PLUGIN_H hPlugin, unsigned int uiStreamLen, unsigned char *pucStream, void *pvDecodingResult)
{
	DP_Video_Error_E eRetErr = (DP_Video_Error_E)DP_Video_ErrorNone;

	DPV_FUNC_LOG();

	if (uiStreamLen < (UINT32_MAX / 2U)) {
		int iDecResult = 0;

		DP_Video_DecodingResult_T *ptDecodingResult = NULL;
		DEC_Plugin_MJPEG_T *ptPlugin = NULL;

		DPV_CAST(ptPlugin, hPlugin);
		DPV_CAST(ptDecodingResult, pvDecodingResult);

		video_plugin_set_decoding_bitstream_buffer_info(&ptPlugin->tDriverInfo, pucStream, (int)uiStreamLen);
		iDecResult = video_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_DECODE, NULL);

		if (iDecResult == VPK_RETCODE_CODEC_EXIT) {
			eRetErr = (DP_Video_Error_E)DP_Video_ErrorDecode;
			DPV_ERROR_MSG("[%s(%d)] [ERROR] VPC_COMMAND_V_DEC_DECODE \n", __func__, __LINE__);
		} else {
			VPC_DRIVER_INFO_T *ptDriverInfo = &ptPlugin->tDriverInfo;

			video_plugin_get_decode_result_info(ptDriverInfo);

			ptDecodingResult->ulResult = video_plugin_get_video_decode_result(ptDriverInfo, ptDecodingResult);
		}
	} else {
		eRetErr = (DP_Video_Error_E)DP_Video_ErrorDecode;
	}

	return (int)eRetErr;
}

static int decoder_plugin_mjpeg_Set(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	DP_Video_Config_E eConfig = (DP_Video_Config_E)DP_Video_Config_None;
	DEC_Plugin_MJPEG_T *ptPlugin = NULL;

	DPV_FUNC_LOG();

	if ((uiId > (unsigned int)DP_Video_Config_None) && (uiId <= (unsigned int)DP_Video_Config_Get_CbCrInterleaveMode)) {
		eConfig = (DP_Video_Config_E)uiId;
	}

	DPV_CAST(ptPlugin, hPlugin);

	switch (eConfig) {
		case DP_Video_Config_Set_ExtraFrameCount: {
			DP_Video_Config_ExtraFrameCount_T *ptExtraFrameCount = NULL;

			DPV_CHECK_STR_LOG("DP_Video_Config_Set_ExtraFrameCount");
			DPV_CAST(ptExtraFrameCount, pvStructure);
			ptPlugin->tDriverInfo.iExtraFrameCount = ptExtraFrameCount->iExtraFrameCount;
			break;
		}

		case DP_Video_Config_Set_VPU_Clock: {
			VPK_CONTENTS_INFO_t tContentInfo = {0, };

			DPV_CHECK_STR_LOG("DP_Video_Config_Set_VPU_Clock");

			if ((ptPlugin->tDriverInfo.iFrameRate > 0) && (ptPlugin->tDriverInfo.iBitRateMbps > 0)) {
				int iTmpFrameRate = ptPlugin->tDriverInfo.iFrameRate / 1000;

				tContentInfo.isSWCodec 	= 0;
				tContentInfo.width 		= MJPEG_MAX_OUTPUT_WIDTH;
				tContentInfo.height	 	= MJPEG_MAX_OUTPUT_HEIGHT;
				tContentInfo.bitrate 	= (unsigned int)ptPlugin->tDriverInfo.iBitRateMbps;
				tContentInfo.framerate 	= (unsigned int)iTmpFrameRate;
			}

			(void)video_plugin_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_MGR, VPC_COMMAND_VPU_SET_CLK, &tContentInfo);
			break;
		}

		case DP_Video_Config_Set_SequenceHeader: {
			DP_Video_Config_SequenceHeader_T *ptSequenceHeader = NULL;

			DPV_CHECK_STR_LOG("DP_Video_Config_Set_SequenceHeader");
			DPV_CAST(ptSequenceHeader, pvStructure);

			iRetVal = decoder_plugin_mjpeg_DecodeSequenceHeader(ptPlugin, ptSequenceHeader);
			break;
		}

		case DP_Video_Config_Set_ReleaseFrame: {
			DP_Video_Config_ReleaseFrame_T *ptReleaseFrame = NULL;

			DPV_CHECK_STR_LOG("DP_Video_Config_Set_ReleaseFrame");
			DPV_CAST(ptReleaseFrame, pvStructure);

			(void)video_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VPC_DRIVER_TYPE_E)VPC_DRIVER_TYPE_VPU, VPC_COMMAND_V_DEC_BUF_FLAG_CLEAR, &ptReleaseFrame->iDisplayIndex);
			break;
		}

		default: {
			DPV_DONOTHING(0);
			break;
		}
	}

	return iRetVal;
}

static int decoder_plugin_mjpeg_Get(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	DP_Video_Config_E eConfig = (DP_Video_Config_E)DP_Video_Config_None;
	DEC_Plugin_MJPEG_T *ptPlugin = NULL;

	DPV_FUNC_LOG();

	if ((uiId > (unsigned int)DP_Video_Config_None) && (uiId <= (unsigned int)DP_Video_Config_Get_CbCrInterleaveMode)) {
		eConfig = (DP_Video_Config_E)uiId;
	}

	DPV_CAST(ptPlugin, hPlugin);

	switch (eConfig) {
		case DP_Video_Config_Get_ExtraFrameCount: {
			DP_Video_Config_ExtraFrameCount_T *ptExtraFrameCount = NULL;

			DPV_CHECK_STR_LOG("DP_Video_Config_Get_ExtraFrameCount");
			DPV_CAST(ptExtraFrameCount, pvStructure);

			ptExtraFrameCount->iExtraFrameCount = ptPlugin->tDriverInfo.iExtraFrameCount;
			break;
		}

		case DP_Video_Config_Get_CbCrInterleaveMode: {
			DP_Video_Config_CbCrInterleaveMode_T *ptCbCrInterleaveMode = NULL;

			DPV_CHECK_STR_LOG("DP_Video_Config_Get_CbCrInterleaveMode");
			DPV_CAST(ptCbCrInterleaveMode, pvStructure);

			ptCbCrInterleaveMode->bCbCrInterleaveMode = ptPlugin->tDriverInfo.bCbCrInterleaveMode;
			break;
		}

		default: {
			DPV_DONOTHING(0);
			break;
		}
	}

	return iRetVal;
}

DECODER_PLUGIN_DEFINE(
	DPV_H264,
	decoder_plugin_mjpeg_Open,
	decoder_plugin_mjpeg_Close,
	decoder_plugin_mjpeg_Init,
	decoder_plugin_mjpeg_Deinit,
	decoder_plugin_mjpeg_Decode,
	decoder_plugin_mjpeg_Set,
	decoder_plugin_mjpeg_Get)