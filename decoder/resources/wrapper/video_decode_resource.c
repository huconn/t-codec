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


#include <stdbool.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "decoder_plugin.h"
#include "decoder_plugin_video.h"
#include "decoder_plugin_interface.h"

#include "video_decode_resource.h"

#if defined(TCC_VPU_V3)
#include "vdec_v3.h"
#endif

#define VDR_PRINT 				(void)g_printf
#define VDR_SNPRINT 			(void)g_snprintf
#define VDR_MSG 				VDR_PRINT
#define VDR_ERR_MSG				VDR_PRINT

#define VDR_MALLOC				g_malloc0
#define VDR_FREE				g_free

#define VDR_DONOTHING(fmt, ...) 	;

#define VDR_CAST(tar, src) 		(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))

#define VDR_STR_LEN 			(128)

#define VDR_MJPEG				"mjpeg"
#define VDR_H263				"h263"
#define VDR_H264				"h264"
#define VDR_H265				"h265"
#define VDR_MPG2				"mpeg2"
#define VDR_MPG4				"mpeg4"
#define VDR_VP8					"vp8"
#define VDR_VP9					"vp9"


typedef struct VD_VPU_LEGACY_T
{
	DEC_PLUGIN_IF_H hPlugin_IF;
	DEC_PLUGIN_H hPlugin;
	DecoderPluginDesc_T *ptPluginDesc;
} VD_VPU_LEGACY_T;


static bool vd_res_init_vpu_legacy_resource(VD_RESOURCE_T *ptResource, VideoDecoder_Codec_E eCodec);


VD_RESOURCE_T *vd_res_create_resource(VideoDecoder_Codec_E eCodec, VD_RESOURCE_TYPE_T eVDType)
{
	VD_RESOURCE_T *ptRes = NULL;
	bool bRet = true;

	ptRes = VDR_MALLOC(sizeof(VD_RESOURCE_T));

	if (ptRes != NULL) {
		if (eVDType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			ptRes->eCodec 	= eCodec;
			ptRes->eType 	= eVDType;

			bRet = vd_res_init_vpu_legacy_resource(ptRes, eCodec);
			#else
			ptRes = NULL;
			bRet = false;
			#endif
		}
		else if(eVDType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			ptRes->eCodec = eCodec;
			ptRes->eType = eVDType;

			bRet = vd_res_init_vpu_legacy_resource(ptRes, eCodec);
#else
			vd_res_destory_resource(ptRes);
			ptRes = NULL;
			bRet = false;
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type: %d \n", __func__, __LINE__, (int)eVDType);
			bRet = false;
		}
	} else {
		VDR_ERR_MSG("[%s(%d) [ERROR] INIT \n", __func__, __LINE__);
	}

	if (bRet == false) {
		vd_res_destory_resource(ptRes);
		ptRes = NULL;
	}

	return ptRes;
}

void vd_res_destory_resource(VD_RESOURCE_T *ptResource)
{
	if (ptResource != NULL) {
		if (ptResource->ptResourceContext != NULL) {
			if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
				#if defined(TCC_VPU_LEGACY)
				VD_VPU_LEGACY_T *ptVpuLegacy = NULL;

				VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);
				if (ptVpuLegacy->ptPluginDesc != NULL) {
					(void)ptVpuLegacy->ptPluginDesc->close(ptVpuLegacy->hPlugin);
				}

				if (ptVpuLegacy->hPlugin_IF != NULL) {
					#if defined(TCC_VPU_LEGACY)
					decoder_plugin_interface_Unload(ptVpuLegacy->hPlugin_IF);
					#endif
				}

				VDR_FREE(ptResource->ptResourceContext);
				#endif
			}
			else if (ptResource->eType == VD_TYPE_VPU_V3)
			{
#if defined(TCC_VPU_V3)
				vdec_handle_h vdecc_handle;
				VDR_CAST(vdecc_handle, ptResource->ptResourceContext);

				vdec_release_instance(vdecc_handle);
#endif
			}
			else
			{
				VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			}
		}

		VDR_FREE(ptResource);
		ptResource = NULL;
	}
}

bool vd_res_init_resource(VD_RESOURCE_T *ptResource, const VideoDecoder_InitConfig_T *ptInit)
{
	bool ret = false;
	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			int iVal = 0;
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;
			DP_Video_Config_Init_T tInitConfig = {0, };

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			tInitConfig.iDummy = 112;
			iVal = ptVpuLegacy->ptPluginDesc->init(ptVpuLegacy->hPlugin, &tInitConfig);

			if (iVal != 0) {
				ret = false;
			}

			iVal = ptVpuLegacy->ptPluginDesc->set(ptVpuLegacy->hPlugin, DP_Video_Config_Set_VPU_Clock, NULL);

			if (iVal != 0) {
				ret = false;
			}

			if (ptInit->eCodec != ptResource->eCodec) {
				VDR_ERR_MSG("[%s(%d) [ERROR] CODEC: %d, %d \n", __func__, __LINE__, (int)ptInit->eCodec, (int)ptResource->eCodec);
			}
			#else
			ret = false;
			#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			vdec_handle_h vdec_handle;
			vdec_init_t vdec_init_info;
			enum vdec_return vdec_ret = VDEC_RETURN_SUCCESS;

			VDR_CAST(vdec_handle, ptResource->ptResourceContext);

			memset(&vdec_init_info, 0x00, sizeof(vdec_init_t));

			switch(ptInit->eCodec)
			{
				case VideoDecoder_Codec_MPEG2:
					vdec_init_info.codec_id = VDEC_CODEC_MPEG2;
				break;

				case VideoDecoder_Codec_MPEG4:
					vdec_init_info.codec_id = VDEC_CODEC_MPEG4;
				break;

				case VideoDecoder_Codec_H263:
					vdec_init_info.codec_id = VDEC_CODEC_H263;
				break;

				case VideoDecoder_Codec_H264:
					vdec_init_info.codec_id = VDEC_CODEC_AVC;
				break;

				case VideoDecoder_Codec_H265:
					vdec_init_info.codec_id = VDEC_CODEC_HEVC;
				break;

				case VideoDecoder_Codec_VP9:
					vdec_init_info.codec_id = VDEC_CODEC_VP9;
				break;

				case VideoDecoder_Codec_VP8:
					vdec_init_info.codec_id = VDEC_CODEC_VP8;
				break;

				case VideoDecoder_Codec_MJPEG:
					vdec_init_info.codec_id = VDEC_CODEC_MJPG;
				break;

				default:
					VDR_DONOTHING();
				break;
			}

			if(ptInit->tCompressMode.eMode == VideoDecoder_FrameBuffer_CompressMode_None)
			{
				if(ptInit->uiBitDepth == 0) //8bit
				{
					//vdec_init_info.output_format = VDEC_OUTPUT_LINEAR_10_TO_8_BIT_YUV420;
					vdec_init_info.output_format = VDEC_OUTPUT_LINEAR_NV12;
				}
				else
				{
					vdec_init_info.output_format = VDEC_OUTPUT_LINEAR_NV12;
				}

				// for NV12
				#if 0
				if(ptInit->uiBitDepth == 0) //8bit
				{
					vdec_init_info.output_format = VDEC_OUTPUT_LINEAR_10_TO_8_BIT_NV12;
				}
				else
				{
					vdec_init_info.output_format = VDEC_OUTPUT_LINEAR_NV12;
				}
				#endif
			}
			else
			{	//VideoDecoder_FrameBuffer_CompressMode_MapConverter
				vdec_init_info.output_format = VDEC_OUTPUT_COMPRESSED_MAPCONV;
				//vdec_init_info.output_format = VDEC_OUTPUT_COMPRESSED_AFBC
			}

			vdec_init_info.enable_ringbuffer_mode = 0U;
			vdec_init_info.enable_dma_buf_id = 0U;
			vdec_init_info.enable_avc_field_display = 0U;
			vdec_init_info.max_support_width = ptInit->uiMax_resolution_width;
			vdec_init_info.max_support_height = ptInit->uiMax_resolution_height;

			if((ptInit->uiMax_resolution_width != 0) && (ptInit->uiMax_resolution_height != 0))
			{
				//vdec_init_info.enable_max_framebuffer = 1U;
			}

			vdec_init_info.additional_frame_count = ptInit->number_of_surface_frames;

			vdec_ret = vdec_init(vdec_handle, &vdec_init_info);
			if(vdec_ret == VDEC_RETURN_FAIL)
			{
				ret = false;
			}
			else
			{
				ret = true;
			}
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			ret = false;
		}
	}

	return ret;
}

void vd_res_deinit_resource(VD_RESOURCE_T *ptResource)
{
	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);
			(void)ptVpuLegacy->ptPluginDesc->deinit(ptVpuLegacy->hPlugin);
			#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			vdec_handle_h vdec_handle;

			VDR_CAST(vdec_handle, ptResource->ptResourceContext);

			(void)vdec_close(vdec_handle);
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
		}
	}
}

int vd_res_decode(VD_RESOURCE_T *ptResource, const VideoDecoder_StreamInfo_T *ptStream, VideoDecoder_DecodingResult_T *ptDecodingResult)
{
	int iVal = 0;
	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;
			DP_Video_DecodingResult_T tLegacyResult = {0, };

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			iVal = ptVpuLegacy->ptPluginDesc->decode(ptVpuLegacy->hPlugin, ptStream->uiStreamLen, ptStream->pucStream, &tLegacyResult);

			ptDecodingResult->ulResult 			= tLegacyResult.ulResult;
			ptDecodingResult->iIntanceIdx 		= tLegacyResult.iIntanceIdx;
			ptDecodingResult->bIsFrameOut 		= tLegacyResult.bIsFrameOut;
			ptDecodingResult->bIsInsertClock 	= tLegacyResult.bIsInsertClock;
			ptDecodingResult->bIsInsertFrame 	= tLegacyResult.bIsInsertFrame;
			ptDecodingResult->iDecodingStatus 	= tLegacyResult.iDecodingStatus;
			ptDecodingResult->iOutputStatus 	= tLegacyResult.iOutputStatus;
			ptDecodingResult->iDecodedIdx 		= tLegacyResult.iDecodedIdx;
			ptDecodingResult->iDispOutIdx 		= tLegacyResult.iDispOutIdx;
			ptDecodingResult->bField 			= tLegacyResult.bField;
			ptDecodingResult->bInterlaced 		= tLegacyResult.bInterlaced;
			ptDecodingResult->bOddFieldFirst 	= tLegacyResult.bOddFieldFirst;
			ptDecodingResult->fAspectRatio 		= tLegacyResult.fAspectRatio;
			ptDecodingResult->uiWidth 			= tLegacyResult.uiWidth;
			ptDecodingResult->uiHeight 			= tLegacyResult.uiHeight;
			ptDecodingResult->uiCropLeft 		= tLegacyResult.uiCropLeft;
			ptDecodingResult->uiCropRight  		= tLegacyResult.uiCropRight;
			ptDecodingResult->uiCropTop 		= tLegacyResult.uiCropTop;
			ptDecodingResult->uiCropBottom 		= tLegacyResult.uiCropBottom;
			ptDecodingResult->uiCropWidth 		= tLegacyResult.uiCropWidth;
			ptDecodingResult->uiCropHeight 		= tLegacyResult.uiCropHeight;

			for (int i = 0; i < (int)VideoDecoder_Addr_MAX; i++) {
				for (int j = 0; j < (int)VideoDecoder_Comp_MAX; j++) {
					ptDecodingResult->pucDispOut[i][j] = tLegacyResult.pucDispOut[i][j];
				}
			}
			#else
			iVal = -1;
			#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			vdec_handle_h vdec_handle;
			enum vdec_return vdec_ret = VDEC_RETURN_SUCCESS;

			vdec_decode_input_t vdec_input;
			vdec_decode_output_t vdec_output;

			VDR_CAST(vdec_handle, ptResource->ptResourceContext);

			memset(&vdec_input, 0x00, sizeof(vdec_decode_input_t));
			memset(&vdec_output, 0x00, sizeof(vdec_decode_output_t));

			vdec_input.input_buffer = ptStream->pucStream;
			vdec_input.input_size = ptStream->uiStreamLen;
			vdec_input.skip_mode = VDEC_FRAMESKIP_AUTO;

			vdec_ret = vdec_decode(vdec_handle, &vdec_input, &vdec_output);
			if(vdec_ret == VDEC_RETURN_FAIL)
			{
				iVal = -1;
			}
			else
			{
				if(vdec_ret == VDEC_RETURN_SUCCESS)
				{
					vdec_decode_output_t* vpu_out = &vdec_output;

					if(vpu_out->display_status != VDEC_DISP_STAT_SUCCESS)
					{
						ptDecodingResult->bIsFrameOut = false;
					}
					else
					{
						ptDecodingResult->bIsFrameOut = true;
					}

					if(vpu_out->decoded_status != VDEC_STAT_SUCCESS)
					{
						ptDecodingResult->bIsInsertFrame = false;
					}
					else
					{
						ptDecodingResult->bIsInsertFrame = true;
					}

					ptDecodingResult->iDecodingStatus = vpu_out->decoded_status;
					ptDecodingResult->iOutputStatus = vpu_out->display_status;
					ptDecodingResult->iDecodedIdx = vpu_out->decoded_idx;
					ptDecodingResult->iDispOutIdx = vpu_out->display_idx;
					ptDecodingResult->bInterlaced = vpu_out->interlaced_frame;
					ptDecodingResult->bOddFieldFirst = (vpu_out->top_field_first == 1) ? 0: 1;
					ptDecodingResult->uiWidth = vpu_out->display_width;
					ptDecodingResult->uiHeight = vpu_out->display_height;
					ptDecodingResult->uiCropLeft = vpu_out->display_crop.left;
					ptDecodingResult->uiCropRight = vpu_out->display_crop.right;
					ptDecodingResult->uiCropTop = vpu_out->display_crop.top;
					ptDecodingResult->uiCropBottom = vpu_out->display_crop.bottom;
					ptDecodingResult->uiCropWidth = (vpu_out->display_crop.right - vpu_out->display_crop.left);
					ptDecodingResult->uiCropHeight = (vpu_out->display_crop.bottom - vpu_out->display_crop.top);

					for (int i = 0; i < VDEC_ADDR_MAX; i++)
					{
						for (int j = 0; j < VDEC_COMP_MAX; j++)
						{
							ptDecodingResult->pucDispOut[i][j] = vdec_output.display_output[i][j];
						}
					}
				}

				iVal = 0;
			}
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			iVal = -1;
		}
	}

	return iVal;
}

bool vd_res_decode_seq_header(VD_RESOURCE_T *ptResource, const VideoDecoder_SequenceHeaderInputParam_T *ptParam)
{
	bool bRet = true;
	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
	#if defined(TCC_VPU_LEGACY)
			int iVal = 0;
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;

			DP_Video_Config_ExtraFrameCount_T tExtraFrameCount;
			DP_Video_Config_SequenceHeader_T tSequenceHeader;

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			if (ptParam->uiNumOutputFrames < (UINT32_MAX / 2U)) {
				tExtraFrameCount.iExtraFrameCount = (int)DEFAULT_FRAME_BUF_CNT + (int)ptParam->uiNumOutputFrames;
			} else {
				tExtraFrameCount.iExtraFrameCount = (int)DEFAULT_FRAME_BUF_CNT;
			}

			iVal = ptVpuLegacy->ptPluginDesc->set(ptVpuLegacy->hPlugin, DP_Video_Config_Set_ExtraFrameCount, &tExtraFrameCount);

			if (iVal != 0) {
				bRet = false;
			} else {
				if (ptParam->uiStreamLen < (UINT32_MAX / 2U)) {
					tSequenceHeader.iStreamLen 	= (int)ptParam->uiStreamLen;
					tSequenceHeader.pucStream 	= ptParam->pucStream;

					iVal = ptVpuLegacy->ptPluginDesc->set(ptVpuLegacy->hPlugin, DP_Video_Config_Set_SequenceHeader, &tSequenceHeader);

					if (iVal != 0) {
						bRet = false;
					}
				} else {
					bRet = false;
				}
			}
	#else
			bRet = false;
	#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
	#if defined(TCC_VPU_V3)
			enum vdec_return vdec_ret = VDEC_RETURN_SUCCESS;
			vdec_handle_h vdec_handle;

			vdec_seqheader_input_t vdec_seq_input;
			vdec_seqheader_output_t vdec_seq_output;

			VDR_CAST(vdec_handle, ptResource->ptResourceContext);

			memset(&vdec_seq_input, 0x00, sizeof(vdec_seqheader_input_t));
			memset(&vdec_seq_output, 0x00, sizeof(vdec_seqheader_output_t));

			vdec_seq_input.input_buffer = ptParam->pucStream;
			vdec_seq_input.input_size = ptParam->uiStreamLen;

			vdec_ret = vdec_seq_header(vdec_handle, &vdec_seq_input, &vdec_seq_output);
			if(vdec_ret == VDEC_RETURN_SUCCESS)
			{
				bRet = true;
			}
			else
			{
				bRet = false;
			}
	#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			bRet = false;
		}

	}

	return bRet;
}

int vd_res_release_frame(VD_RESOURCE_T *ptResource, int iDisplayIndex)
{
	int ret = 0;

	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;

			DP_Video_Config_ReleaseFrame_T tReleaseFrame = {0, };

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			tReleaseFrame.iDisplayIndex = iDisplayIndex;
			ret = ptVpuLegacy->ptPluginDesc->set(ptVpuLegacy->hPlugin, DP_Video_Config_Set_ReleaseFrame, &tReleaseFrame);
			#else
			ret = -1;
			#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			enum vdec_return vdec_ret = VDEC_RETURN_SUCCESS;
			vdec_handle_h vdec_handle;
			vdec_buf_clear_t vdec_buf_clear_info;

			VDR_CAST(vdec_handle, ptResource->ptResourceContext);
			memset(&vdec_buf_clear_info, 0x00, sizeof(vdec_buf_clear_t));

			vdec_buf_clear_info.display_index = iDisplayIndex;
			vdec_buf_clear_info.dma_buf_id = -1;

			vdec_ret = vdec_buf_clear(vdec_handle, &vdec_buf_clear_info);
			if(vdec_ret == VDEC_RETURN_SUCCESS)
			{
				ret = 0;
			}
			else
			{
				ret = -1;
			}
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			ret = -1;
		}
	}

	return ret;
}

unsigned int vd_res_get_CbCrInterleaveMode(VD_RESOURCE_T *ptResource)
{
	unsigned int ret = 0U;
	if (ptResource != NULL) {
		if (ptResource->eType == VD_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			DP_Video_Config_CbCrInterleaveMode_T tCbCrInterleaveMode = {0, };
			VD_VPU_LEGACY_T *ptVpuLegacy = NULL;

			VDR_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			(void)ptVpuLegacy->ptPluginDesc->get(ptVpuLegacy->hPlugin, DP_Video_Config_Get_CbCrInterleaveMode, &tCbCrInterleaveMode);
			ret = (unsigned int)tCbCrInterleaveMode.bCbCrInterleaveMode;
			#else
			ret = 0U;
			#endif
		}
		else if (ptResource->eType == VD_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			ret = 0U; //false
#endif
		}
		else
		{
			VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			ret = 0U;
		}
	}

	return ret;
}

static bool vd_res_init_vpu_legacy_resource(VD_RESOURCE_T *ptResource, VideoDecoder_Codec_E eCodec)
{
	bool bRet = true;

	#if defined(TCC_VPU_LEGACY)
	char strPluginName[VDR_STR_LEN] = "\n";

	if (ptResource != NULL) {
		switch (eCodec) {
			case VideoDecoder_Codec_MPEG2:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_MPG2);
				break;
			case VideoDecoder_Codec_MPEG4:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_MPG4);
				break;
			case VideoDecoder_Codec_H263:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_H263);
				break;
			case VideoDecoder_Codec_H264:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_H264);
				break;
		#if defined(TCC_HEVC_INCLUDE) || defined(TCC_VPU_4K_D2_INCLUDE)
			case VideoDecoder_Codec_H265:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_H265);
				break;
		#endif
		#if defined(TCC_VP9_INCLUDE) || defined(TCC_VPU_4K_D2_INCLUDE)
			case VideoDecoder_Codec_VP9:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_VP9);
				break;
		#endif
			case VideoDecoder_Codec_VP8:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_VP8);
				break;
			case VideoDecoder_Codec_MJPEG:
				VDR_SNPRINT(strPluginName, VDR_STR_LEN, VDR_MJPEG);
				break;
			default:
				bRet = false;
				break;
		}

		//VDR_MSG("[%s(%d) Plugin Name: %s(%d) \n", __func__, __LINE__, strPluginName, eCodec);

		if (bRet == true) {
			#if defined(TCC_VPU_LEGACY)
			VD_VPU_LEGACY_T *ptVpuLegacy = VDR_MALLOC(sizeof(VD_VPU_LEGACY_T));

			if (ptVpuLegacy != NULL) {
				ptVpuLegacy->hPlugin_IF = decoder_plugin_interface_Load(strPluginName, &ptVpuLegacy->ptPluginDesc);

				if (ptVpuLegacy->hPlugin_IF == NULL) {
					VDR_ERR_MSG("[%s(%d) [ERROR] Unsupported: %d \n", __func__, __LINE__, (int)eCodec);
					bRet = false;
				} else {
					ptVpuLegacy->hPlugin = ptVpuLegacy->ptPluginDesc->open();

					if (ptVpuLegacy->hPlugin == NULL) {
						VDR_ERR_MSG("[%s(%d) [ERROR] LOAD \n", __func__, __LINE__);

						decoder_plugin_interface_Unload(ptVpuLegacy->hPlugin_IF);
						ptVpuLegacy->hPlugin_IF = NULL;

						bRet = false;
					} else {
						ptResource->ptResourceContext = ptVpuLegacy;
					}
				}

				if (bRet == false) {
					VDR_FREE(ptVpuLegacy);
					ptVpuLegacy = NULL;
				}
			}
			#endif
		}
	} else {
		bRet = false;
	}
	#elif defined(TCC_VPU_V3)
	{
		vdec_info_t vdec_info;
		vdec_handle_h vdec_handle;
		char str_env_log_level[] = "ENV_VDEC_TEST_LOG_LEVEL";
		char* strLogLevel = getenv(str_env_log_level);

		if(strLogLevel != NULL)
		{
			unsigned int loglevel = 0U;
			loglevel = atoi(strLogLevel);
			VDR_MSG("[%s:%d] dec log level:%d, strLogLevel:%s\n", __FUNCTION__, __LINE__, loglevel, strLogLevel);
			vdec_set_log_level(loglevel);
		}

		vdec_handle = vdec_alloc_instance(&vdec_info);
		if(vdec_handle != NULL)
		{
			ptResource->ptResourceContext = vdec_handle;
			bRet = true;
		}
		else
		{
			bRet = false;
		}
	}
	#else
	bRet = false;
	#endif

	return bRet;
}