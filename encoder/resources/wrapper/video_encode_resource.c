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

#include "encoder_plugin.h"
#include "encoder_plugin_video.h"
#include "encoder_plugin_interface.h"

#include "video_encode_resource.h"

#if defined(TCC_VPU_V3)
#include "venc_v3.h"
#endif

#define VER_PRINT 				(void)g_printf
#define VER_SNPRINT 			(void)g_snprintf
#define VER_MSG 				VER_PRINT
#define VER_ERR_MSG				VER_PRINT

#define VER_MALLOC				g_malloc0
#define VER_FREE				g_free

#define VER_CAST(tar, src) 		(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))

#define VER_STR_LEN 			(128)

#define VER_MJPEG				"mjpeg"
#define VER_H264				"h264"
#define VER_H265				"h265"


typedef struct VE_VPU_LEGACY_T
{
	TENC_PLUGIN_IF_H hPlugin_IF;
	TENC_PLUGIN_H hPlugin;

	EncoderPluginDesc_T *ptPluginDesc;
} VE_VPU_LEGACY_T;


static bool ve_res_init_vpu_legacy_resource(VE_RESOURCE_T *ptResource, VideoEncoder_Codec_E eCodec);


VE_RESOURCE_T *ve_res_create_resource(VideoEncoder_Codec_E eCodec, VE_RESOURCE_TYPE_T eVEType)
{
	VE_RESOURCE_T *ptRes = VER_MALLOC(sizeof(VE_RESOURCE_T));
	bool bRet = true;

	if (ptRes != NULL) {
		if (eVEType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			ptRes->eCodec 	= eCodec;
			ptRes->eType 	= eVEType;

			bRet = ve_res_init_vpu_legacy_resource(ptRes, eCodec);
			#else
			ve_res_destory_resource(ptRes);
			ptRes = NULL;
			bRet = false;
			#endif
		}
		else if(eVEType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			ptRes->eCodec = eCodec;
			ptRes->eType = eVEType;

			bRet = ve_res_init_vpu_legacy_resource(ptRes, eCodec);
#else
			ve_res_destory_resource(ptRes);
			ptRes = NULL;
			bRet = false;
#endif
		}
		else {
			VER_ERR_MSG("[%s(%d) [ERROR] Unsupported Type \n", __func__, __LINE__);
			bRet = false;
		}
	} else {
		VER_ERR_MSG("[%s(%d) [ERROR] INIT \n", __func__, __LINE__);
	}

	if (bRet == false) {
		VER_ERR_MSG("[%s(%d) ve_res_destory_resource \n", __func__, __LINE__);
		ve_res_destory_resource(ptRes);
		ptRes = NULL;
	}

	return ptRes;
}

void ve_res_destory_resource(VE_RESOURCE_T *ptResource)
{
	if (ptResource != NULL) {
		if (ptResource->eType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VE_VPU_LEGACY_T *ptVpuLegacy = NULL;

			if (ptResource->ptResourceContext != NULL) {
				VER_CAST(ptVpuLegacy, ptResource->ptResourceContext);

				if (ptVpuLegacy->ptPluginDesc != NULL) {
					(void)ptVpuLegacy->ptPluginDesc->close(ptVpuLegacy->hPlugin);
				}

				if (ptVpuLegacy->hPlugin_IF != NULL) {
					encoder_plugin_interface_Unload(ptVpuLegacy->hPlugin_IF);
				}

				VER_FREE(ptResource->ptResourceContext);
			}
			#endif
		}
		else if (ptResource->eType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			venc_handle_h venc_handle;
			VER_CAST(venc_handle, ptResource->ptResourceContext);

			venc_release_instance(venc_handle);
#endif
		}
		else
		{
			VER_ERR_MSG("[%s(%d) not supported \n", __func__, __LINE__);
		}

		VER_FREE(ptResource);
	}
}

int ve_res_init_resource(VE_RESOURCE_T *ptResource, const VideoEncoder_InitConfig_T *ptInit)
{
	int ret = 0;
	if ((ptResource != NULL) && (ptInit != NULL)) {
		if (ptResource->eType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VE_VPU_LEGACY_T *ptVpuLegacy = NULL;
			TENC_PLUGIN_INIT_T tInit = {0, };

			tInit.iWidth 				= ptInit->iImageWidth;
			tInit.iHeight 				= ptInit->iImageHeight;

			tInit.iFrameRate 			= ptInit->iFrameRate;
			tInit.iKeyFrameInterval 	= ptInit->iKeyFrameInterval;
			tInit.iBitRate 				= ptInit->iBitRate;

			tInit.iSliceMode 			= ptInit->iSliceMode;
			tInit.iSliceSizeUnit 		= ptInit->iSliceSizeUnit;
			tInit.iSliceSize 			= ptInit->iSliceSize;

			VER_CAST(ptVpuLegacy, ptResource->ptResourceContext);
			ret = (int)ptVpuLegacy->ptPluginDesc->init(ptVpuLegacy->hPlugin, &tInit);
			#else
			ret = -1;
			#endif
		}
		else if (ptResource->eType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			enum venc_codec_id venc_codec = VENC_CODEC_NONE;
			venc_handle_h venc_handle;
			venc_init_t init_info;

			VER_CAST(venc_handle, ptResource->ptResourceContext);

			memset(&init_info, 0x00, sizeof(venc_init_t));

			switch(ptResource->eCodec)
			{
				case VideoEncoder_Codec_H264:
					venc_codec = VENC_CODEC_H264;
				break;

				case VideoEncoder_Codec_H265:
					venc_codec = VENC_CODEC_HEVC;
				break;

				case VideoEncoder_Codec_JPEG:
					venc_codec = VENC_CODEC_MJPG;
				break;

				default:
					venc_codec = VENC_CODEC_NONE;
				break;
			}

			init_info.codec = venc_codec;
			init_info.source_format = VENC_SOURCE_YUV420P;
			init_info.pic_width = ptInit->iImageWidth;
			init_info.pic_height = ptInit->iImageHeight;
			init_info.framerate = ptInit->iFrameRate;
			init_info.bitrateKbps = ptInit->iBitRate / 1024;
			init_info.key_interval = ptInit->iKeyFrameInterval;

			init_info.slice_mode = ptInit->iSliceMode;
			init_info.slice_size_mode = ptInit->iSliceSizeUnit;
			init_info.slice_size = ptInit->iSliceSize;

			ret = venc_init(venc_handle, &init_info);
#endif
		}
		else
		{
			VER_ERR_MSG("[%s(%d) not supported \n", __func__, __LINE__);
		}
	}

	return ret;
}

void ve_res_deinit_resource(VE_RESOURCE_T *ptResource)
{
	if (ptResource != NULL) {
		if (ptResource->eType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			VE_VPU_LEGACY_T *ptVpuLegacy = NULL;

			VER_CAST(ptVpuLegacy, ptResource->ptResourceContext);
			(void)ptVpuLegacy->ptPluginDesc->deinit(ptVpuLegacy->hPlugin);
			#endif
		}
		else if (ptResource->eType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			venc_handle_h venc_handle;
			VER_CAST(venc_handle, ptResource->ptResourceContext);

			(void)venc_close(venc_handle);
#endif
		}
		else
		{
			VER_ERR_MSG("[%s(%d) not supported \n", __func__, __LINE__);
		}
	}
}

int ve_res_set_seq_header(VE_RESOURCE_T *ptResource, VideoEncoder_Set_Header_Result_T *ptHeader)
{
	int ret = 0;

	if (ptResource != NULL) {
		if (ptResource->eType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			EP_Video_SequenceHeader_Result_T tSequenceHeader = {0, };
			VE_VPU_LEGACY_T *ptVpuLegacy = NULL;

			VER_CAST(ptVpuLegacy, ptResource->ptResourceContext);
			ret = (int)ptVpuLegacy->ptPluginDesc->set(ptVpuLegacy->hPlugin, EP_Video_Config_Set_SequenceHeader, &tSequenceHeader);

			ptHeader->iHeaderSize 	= tSequenceHeader.iHeaderSize;
			ptHeader->pucBuffer 	= tSequenceHeader.pucBuffer;
			#else
			ret = -1;
			#endif
		}
		else if (ptResource->eType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			venc_handle_h venc_handle;
			venc_seq_header_t header_info;

			VER_CAST(venc_handle, ptResource->ptResourceContext);

			memset(&header_info, 0x00, sizeof(venc_seq_header_t));

			ret = venc_put_seqheader(venc_handle, &header_info);
			if(ret == 0)
			{
				unsigned char* p = NULL;
				ptHeader->iHeaderSize = header_info.seq_header_out_size;
				ptHeader->pucBuffer = header_info.seq_header_out;
				p = ptHeader->pucBuffer;

				if(p == NULL)
				{
					VER_ERR_MSG("[%s(%d) venc_put_seqheader error, out:%p, size:%d\n", __func__, __LINE__, ptHeader->pucBuffer, ptHeader->iHeaderSize);
				}
			}
			else
			{
				ret = -1;
			}
#endif
		}
		else
		{
			VER_ERR_MSG("[%s(%d) not supported \n", __func__, __LINE__);
		}
	}

	return ret;
}

int ve_res_encode(VE_RESOURCE_T *ptResource, VideoEncoder_Encode_Input_T *ptEncode, VideoEncoder_Encode_Result_T *ptEncodeResult)
{
	int ret = 0;

	if (ptResource != NULL) {
		if (ptResource->eType == VE_TYPE_VPU_LEGACY) {
			#if defined(TCC_VPU_LEGACY)
			EP_Video_Encode_Result_T tEncodeResult = {0, };
			VE_VPU_LEGACY_T *ptVpuLegacy = NULL;

			VER_CAST(ptVpuLegacy, ptResource->ptResourceContext);

			ret = (int)ptVpuLegacy->ptPluginDesc->encode(ptVpuLegacy->hPlugin, ptEncode, &tEncodeResult);

			ptEncodeResult->iFrameSize 			= tEncodeResult.iFrameSize;
			ptEncodeResult->pucBuffer 			= tEncodeResult.pucBuffer;
			ptEncodeResult->ePictureType 		= (VideoEncoder_Codec_Picture_E)tEncodeResult.eType;
			#else
			ret = -1;
			#endif
		}
		else if (ptResource->eType == VE_TYPE_VPU_V3)
		{
#if defined(TCC_VPU_V3)
			venc_handle_h venc_handle;
			venc_input_t input_info;
			venc_output_t output_info;

			VER_CAST(venc_handle, ptResource->ptResourceContext);

			memset(&input_info, 0x00, sizeof(venc_input_t));
			memset(&output_info, 0x00, sizeof(venc_output_t));

			input_info.input_y = ptEncode->pucVideoBuffer[VE_IN_Y];
			input_info.input_crcb[0] = ptEncode->pucVideoBuffer[VE_IN_U];
			input_info.input_crcb[1] = ptEncode->pucVideoBuffer[VE_IN_V];

			ret = venc_encode(venc_handle, &input_info, &output_info);
			if(ret == 0)
			{
				unsigned char* p = NULL;
				ptEncodeResult->pucBuffer = output_info.bitstream_out;
				ptEncodeResult->iFrameSize = output_info.bitstream_out_size;

				switch(output_info.pic_type)
				{
					case VENC_PICTYPE_I:
						ptEncodeResult->ePictureType = VideoEncoder_Codec_Picture_I;
					break;

					case VENC_PICTYPE_P:
						ptEncodeResult->ePictureType = VideoEncoder_Codec_Picture_P;
					break;

					default:
						ptEncodeResult->ePictureType = VideoEncoder_Codec_Picture_UnKnown;
					break;
				}

				p = ptEncodeResult->pucBuffer;
				if(p == NULL)
				{
					VER_ERR_MSG("[%s(%d) venc_encode error, out:%p, size:%d\n", __func__, __LINE__, ptEncodeResult->pucBuffer, ptEncodeResult->iFrameSize);
				}
			}
			else
			{
				ret = -1;
			}
#endif
		}
		else
		{
			VER_ERR_MSG("[%s(%d) not supported \n", __func__, __LINE__);
		}
	}

	return ret;
}

static bool ve_res_init_vpu_legacy_resource(VE_RESOURCE_T *ptResource, VideoEncoder_Codec_E eCodec)
{
	bool bRet = true;

	#if defined(TCC_VPU_LEGACY)
	char strPluginName[VER_STR_LEN] = "\n";

	if (ptResource != NULL) {
		switch (eCodec) {
			case VideoEncoder_Codec_H264:
				VER_SNPRINT(strPluginName, VER_STR_LEN, VER_H264);
				break;
		#if defined(TCC_VPU_4K_E1_INCLUDE)
			case VideoEncoder_Codec_H265:
				VER_SNPRINT(strPluginName, VER_STR_LEN, VER_H265);
				break;
		#endif
			case VideoEncoder_Codec_JPEG:
				VER_SNPRINT(strPluginName, VER_STR_LEN, VER_MJPEG);
				break;
			default:
				#if 0
				VER_ERR_MSG("[%s(%d) [ERROR] Not Supported \n", __func__, __LINE__);
				ret = VideoEncoder_Error_NotSupportedCodec;
				#else
				VER_SNPRINT(strPluginName, VER_STR_LEN, "dummy");
				bRet = false;
				#endif
				break;
		}

		if (bRet == true) {
			VE_VPU_LEGACY_T *ptVpuLegacy = VER_MALLOC(sizeof(VE_VPU_LEGACY_T));

			if (ptVpuLegacy != NULL) {
				ptVpuLegacy->hPlugin_IF = encoder_plugin_interface_Load(strPluginName, &ptVpuLegacy->ptPluginDesc);

				if (ptVpuLegacy->hPlugin_IF == NULL) {
					VER_ERR_MSG("[%s(%d) [ERROR] Unsupported \n", __func__, __LINE__);
					bRet = false;
				} else {
					ptVpuLegacy->hPlugin = ptVpuLegacy->ptPluginDesc->open();

					if (ptVpuLegacy->hPlugin == NULL) {
						VER_ERR_MSG("[%s(%d) [ERROR] LOAD \n", __func__, __LINE__);

						encoder_plugin_interface_Unload(ptVpuLegacy->hPlugin_IF);
						ptVpuLegacy->hPlugin_IF = NULL;

						bRet = false;
					} else {
						ptResource->ptResourceContext = ptVpuLegacy;
					}
				}
			}

			if (bRet == false) {
				if (ptVpuLegacy->hPlugin_IF != NULL) {
					VER_FREE(ptVpuLegacy->hPlugin_IF);
				}

				if (ptVpuLegacy->hPlugin != NULL) {
					VER_FREE(ptVpuLegacy->hPlugin);
				}

				VER_FREE(ptVpuLegacy);
			}
		}
	} else {
		bRet = false;
	}
	#elif defined(TCC_VPU_V3)
	{
		venc_info_t info;
		venc_handle_h venc_handle;

		char str_env_log_level[] = "ENV_VENC_TEST_LOG_LEVEL";
		char* strLogLevel = getenv(str_env_log_level);

		if(strLogLevel != NULL)
		{
			unsigned int loglevel = 0U;
			loglevel = atoi(strLogLevel);
			VER_MSG("[%s:%d] enc log level:%d, strLogLevel:%s\n", __FUNCTION__, __LINE__, loglevel, strLogLevel);
			venc_set_log_level(loglevel);
		}

		venc_handle = venc_alloc_instance(&info);
		if(venc_handle != NULL)
		{
			ptResource->ptResourceContext = venc_handle;
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