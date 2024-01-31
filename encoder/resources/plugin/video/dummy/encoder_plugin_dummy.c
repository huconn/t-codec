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


#include <encoder_plugin.h>
#include <encoder_plugin_video.h>
#include <encoder_plugin_video_common.h>


typedef struct
{
	VEPC_DRIVER_INFO_T tDriverInfo;

	bool bInited;
} ENC_Plugin_DUMMY_T;


static int encoder_plugin_dummy_Alloc_Memory(ENC_Plugin_DUMMY_T *ptPlugin);
static int encoder_plugin_dummy_Free_Memory(ENC_Plugin_DUMMY_T *ptPlugin);
static void encoder_plugin_dummy_Set_CLK(ENC_Plugin_DUMMY_T *ptPlugin, TENC_PLUGIN_INIT_T *ptSetInit);

static TENC_PLUGIN_H encoder_plugin_dummy_Open(void);
static int encoder_plugin_dummy_Close(TENC_PLUGIN_H hPlugin);
static int encoder_plugin_dummy_Init(TENC_PLUGIN_H hPlugin, TENC_PLUGIN_INIT_T *ptSetInit);
static int encoder_plugin_dummy_Deinit(TENC_PLUGIN_H hPlugin);
static int encoder_plugin_dummy_Encode(TENC_PLUGIN_H hPlugin, void *pvEncodeInput, void *pvEncodingResult);
static int encoder_plugin_dummy_Set(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
static int encoder_plugin_dummy_Get(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
static int encoder_plugin_dummy_Process_Seq_Header(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_SequenceHeader_Result_T *ptSequenceHeader);


static int encoder_plugin_dummy_Alloc_Memory(ENC_Plugin_DUMMY_T *ptPlugin)
{
	int ret = 0;

	TEPV_FUNC_LOG();

	if (ptPlugin != NULL) {
		ret = video_encode_plugin_alloc_bitstream_buffer(&ptPlugin->tDriverInfo);

		if (ret > 0) {
			ret = video_encode_plugin_alloc_bitwork_buffer(&ptPlugin->tDriverInfo);
		} else {
			ret = -1;
		}

		if (ret > 0) {
			ret = video_encode_plugin_alloc_me_search_buffer(&ptPlugin->tDriverInfo);
		} else {
			ret = -1;
		}

		if (ptPlugin->tDriverInfo.iSliceMode == 1) {
			if (ret > 0) {
				ret = video_encode_plugin_alloc_slice_buffer(&ptPlugin->tDriverInfo);
			} else {
				ret = -1;
			}
		}

		if (ret > 0) {
			ret = video_encode_plugin_alloc_raw_image_buffer(&ptPlugin->tDriverInfo);
		} else {
			ret = -1;
		}
	}

	return ret;
}

static int encoder_plugin_dummy_Free_Memory(ENC_Plugin_DUMMY_T *ptPlugin)
{
	TEPV_FUNC_LOG();

	if (ptPlugin != NULL) {
		if (ptPlugin->tDriverInfo.pucSeqHeaderBufferOutVA != NULL) {
			TEPV_FREE(ptPlugin->tDriverInfo.pucSeqHeaderBufferOutVA);
		}
		video_encode_plugin_free_alloc_buffers(&ptPlugin->tDriverInfo);
	}

	return 0;
}

static void encoder_plugin_dummy_Set_CLK(ENC_Plugin_DUMMY_T *ptPlugin, TENC_PLUGIN_INIT_T *ptSetInit)
{
	TEPV_FUNC_LOG();

	if (ptPlugin != NULL) {
		VEPK_CONTENTS_INFO_t tContentInfo = {0, };

		tContentInfo.type 		= VEPK_VPU_ENC;

		if ((ptSetInit->iWidth > 0) && (ptSetInit->iHeight > 0)) {
			tContentInfo.width 		= (unsigned int)ptSetInit->iWidth;
			tContentInfo.height	 	= (unsigned int)ptSetInit->iHeight;
		}

		(void)video_encode_plugin_cmd(&ptPlugin->tDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_MGR, VEPC_COMMAND_VPU_SET_CLK, &tContentInfo);
	}
}

static TENC_PLUGIN_H encoder_plugin_dummy_Open(void)
{
	void *tmpPtr = TEPV_CALLOC(sizeof(ENC_Plugin_DUMMY_T));
	ENC_Plugin_DUMMY_T *ptPlugin = NULL;

	TEPV_FUNC_LOG();

	if (tmpPtr != NULL) {
		TEPV_CAST(ptPlugin, tmpPtr);
	}

	return (TENC_PLUGIN_H)ptPlugin;
}

static int encoder_plugin_dummy_Close(TENC_PLUGIN_H hPlugin)
{
	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		ENC_Plugin_DUMMY_T *ptPlugin = NULL;

		TEPV_CAST(ptPlugin, hPlugin);

		if (ptPlugin != NULL) {
			TEPV_FREE(ptPlugin);
		}
	}

	return 0;
}

static int encoder_plugin_dummy_Init(TENC_PLUGIN_H hPlugin, TENC_PLUGIN_INIT_T *ptSetInit)
{
	EP_Video_Error_E ret = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		ENC_Plugin_DUMMY_T *ptPlugin = NULL;
		TEPV_CAST(ptPlugin, hPlugin);

		ptPlugin->tDriverInfo.iInstIdx 			= -1;
		ptPlugin->tDriverInfo.uiCodec 			= (unsigned int)VEPK_STD_H264;
		ptPlugin->tDriverInfo.iInstType 		= (int)VEPK_VPU_ENC;
		ptPlugin->tDriverInfo.iNumOfInst 		= 0;

		ret = video_encode_plugin_open_drivers(&ptPlugin->tDriverInfo);

		if (ret != (EP_Video_Error_E)EP_Video_ErrorNone) {
			TEPV_ERROR_MSG("[%s(%d)] [ERROR] video_plugin_open_drivers() \n", __func__, __LINE__);
			(void)video_encode_plugin_close_drivers(&ptPlugin->tDriverInfo);
		} else {
			ptPlugin->tDriverInfo.isSeqHeaderDone 	= false;

			ptPlugin->tDriverInfo.iWidth 			= ptSetInit->iWidth;
			ptPlugin->tDriverInfo.iHeight 			= ptSetInit->iHeight;

			ptPlugin->tDriverInfo.iBitstreamFormat 	= VEPK_STD_H264;

			ptPlugin->tDriverInfo.iFrameRate 		= ptSetInit->iFrameRate;
			ptPlugin->tDriverInfo.iKeyInterval 		= ptSetInit->iKeyFrameInterval;
			ptPlugin->tDriverInfo.iTargetKbps		= (ptSetInit->iBitRate / 1024);

			ptPlugin->tDriverInfo.iSliceMode 		= ptSetInit->iSliceMode;
			ptPlugin->tDriverInfo.iSliceSizeUnit	= ptSetInit->iSliceSizeUnit;
			ptPlugin->tDriverInfo.iSliceSize		= ptSetInit->iSliceSize;

			ptPlugin->tDriverInfo.bNalStartCode		= true;

			video_encode_plugin_set_init_info(&ptPlugin->tDriverInfo);

			encoder_plugin_dummy_Set_CLK(ptPlugin, ptSetInit);
			(void)encoder_plugin_dummy_Alloc_Memory(ptPlugin);

			(void)video_encode_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_INIT, NULL);

			video_encode_plugin_get_init_info(&ptPlugin->tDriverInfo);
			ptPlugin->bInited = true;
		}
	} else {
		TEPV_ERROR_MSG("[%s(%d)] [ERROR] INPUT \n", __func__, __LINE__);
		ret = (EP_Video_Error_E)EP_Video_ErrorInit;
	}

	return (int)ret;
}

static int encoder_plugin_dummy_Deinit(TENC_PLUGIN_H hPlugin)
{
	int ret = 0;

	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		ENC_Plugin_DUMMY_T *ptPlugin = NULL;

		TEPV_CAST(ptPlugin, hPlugin);

		if (ptPlugin->bInited == false) {
			ret = -1;
		} else {
			(void)video_encode_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_CLOSE, NULL);
			(void)encoder_plugin_dummy_Free_Memory(ptPlugin);
			(void)video_encode_plugin_close_drivers(&ptPlugin->tDriverInfo);
		}
	}

	return ret;
}

static int encoder_plugin_dummy_Process_Seq_Header(VEPC_DRIVER_INFO_T *ptDriverInfo, EP_Video_SequenceHeader_Result_T *ptSequenceHeader)
{
	EP_Video_Error_E eRetErr = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (ptDriverInfo != NULL) {
		if (ptDriverInfo->isSeqHeaderDone == false) {
			int ret = -1;

			ptDriverInfo->isSeqHeaderDone = true;

			(void)video_encode_plugin_alloc_frame_buffer(ptDriverInfo);
			(void)video_encode_plugin_alloc_seq_header_buffer(ptDriverInfo);

			if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_MPEG4) {
				ret = video_encode_plugin_process_seq_header(ptDriverInfo, VEPK_MPEG4_VOS_HEADER);

				if (ret == 0) {
					ret = video_encode_plugin_process_seq_header(ptDriverInfo, VEPK_MPEG4_VIS_HEADER);
				}
			} else if (ptDriverInfo->uiCodec == (unsigned int)VEPK_STD_H264) {
				ret = video_encode_plugin_process_seq_header(ptDriverInfo, VEPK_AVC_SPS_RBSP);

				if (ret == 0) {
					ret = video_encode_plugin_process_seq_header(ptDriverInfo, VEPK_AVC_PPS_RBSP);
				}
			} else {
				ret = video_encode_plugin_process_seq_header(ptDriverInfo, AVC_PPS_RBSP);
			}

			ptSequenceHeader->iHeaderSize = ptDriverInfo->iSeqHeaderBufferOutSize;
			ptSequenceHeader->pucBuffer = ptDriverInfo->pucSeqHeaderBufferOutVA;

			if (ret != 0) {
				eRetErr = (EP_Video_Error_E)EP_Video_ErrorHeader;
			}
		}
	}

	return (int)eRetErr;
}
static int encoder_plugin_dummy_Encode(TENC_PLUGIN_H hPlugin, void *pvEncodeInput, void *pvEncodingResult)
{
	EP_Video_Error_E eRetErr = (EP_Video_Error_E)EP_Video_ErrorNone;

	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		int ret = -1;

		ENC_Plugin_DUMMY_T *ptPlugin = NULL;
		EP_Video_Encode_Input_T *ptEncodeInput = NULL;
		EP_Video_Encode_Result_T *ptEncodeResult = NULL;

		TEPV_CAST(ptPlugin, hPlugin);
		TEPV_CAST(ptEncodeInput, pvEncodeInput);
		TEPV_CAST(ptEncodeResult, pvEncodingResult);

		video_encode_plugin_set_encode_info(&ptPlugin->tDriverInfo, ptEncodeInput);
		ret = video_encode_plugin_poll_cmd(&ptPlugin->tDriverInfo, (VEPC_DRIVER_TYPE_E)VEPC_DRIVER_TYPE_VPU, VEPC_COMMAND_V_ENC_ENCODE, NULL);

		if (ret != 0) {
			eRetErr = (EP_Video_Error_E)EP_Video_ErrorEncode;
		}

		video_encode_plugin_get_encode_result_info(&ptPlugin->tDriverInfo);

		ptEncodeResult->iFrameSize 	= ptPlugin->tDriverInfo.iBitStreamOutSize;
		ptEncodeResult->pucBuffer 	= ptPlugin->tDriverInfo.pucBitStreamOutBuffer;

		if (ptPlugin->tDriverInfo.eFrameType == VEPC_FRAME_TYPE_I) {
			ptEncodeResult->eType 	= EP_Video_FrameI;
		} else if (ptPlugin->tDriverInfo.eFrameType == VEPC_FRAME_TYPE_P) {
			ptEncodeResult->eType 	= EP_Video_FrameP;
		} else {
			ptEncodeResult->eType 	= EP_Video_FrameUnKnown;
		}
	}

	return (int)eRetErr;
}

static int encoder_plugin_dummy_Set(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		ENC_Plugin_DUMMY_T *ptPlugin = NULL;
		EP_Video_Config_E eConfig = (EP_Video_Config_E)EP_Video_Config_None;

		if ((uiId > (unsigned int)EP_Video_Config_None) && (uiId <= (unsigned int)EP_Video_Config_Get_CbCrInterleaveMode)) {
			eConfig = (EP_Video_Config_E)uiId;
		}

		TEPV_CAST(ptPlugin, hPlugin);

		switch (eConfig) {
			case EP_Video_Config_Set_SequenceHeader: {
				EP_Video_SequenceHeader_Result_T *ptSequenceHeader = NULL;

				TEPV_CHECK_STR_LOG("EP_Video_Config_Set_SequenceHeader");
				TEPV_CAST(ptSequenceHeader, pvStructure);

				iRetVal = encoder_plugin_dummy_Process_Seq_Header(&ptPlugin->tDriverInfo, ptSequenceHeader);
				break;
			}

			default: {
				TEPV_DONOTHING(0);
				break;
			}
		}
	}

	return iRetVal;
}

static int encoder_plugin_dummy_Get(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	TEPV_FUNC_LOG();

	if (hPlugin != NULL) {
		(void)uiId;
		(void)pvStructure;
	}

	return iRetVal;
}

TENCODER_PLUGIN_DEFINE(
	"dummy",
	encoder_plugin_dummy_Open,
	encoder_plugin_dummy_Close,
	encoder_plugin_dummy_Init,
	encoder_plugin_dummy_Deinit,
	encoder_plugin_dummy_Encode,
	encoder_plugin_dummy_Set,
	encoder_plugin_dummy_Get)