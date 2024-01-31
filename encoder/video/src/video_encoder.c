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

#include <glib.h>
#include <glib/gprintf.h>

#include "video_encode_resource.h"
#include "video_encoder.h"


#define VE_PRINT 				(void)g_printf
#define VE_CAST(tar, src) 		(void)memcpy((void*)&(tar), (void*)&(src), sizeof(void*))
#define VE_CALLOC				g_malloc0
#define VE_FREE					(void)g_free

#define VE_ERROR_MSG 			VE_PRINT


struct VideoEncoder_T
{
	VideoEncoder_Codec_E eCodec;

	VE_RESOURCE_TYPE_T eType;
	VE_RESOURCE_T *ptResource;

	uint64_t uiFrameID;
};


static VideoEncoder_Error_E SF_VideoEncoder_SetCodec(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Codec_E eCodec);


VideoEncoder_Error_E VideoEncoder_Encode(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Encode_Input_T *ptEncode, VideoEncoder_Encode_Result_T *ptEncodeResult)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
		(void)ve_res_encode(ptVideoEncoder->ptResource, ptEncode, ptEncodeResult);
	}

	return ret;
}

VideoEncoder_Error_E VideoEncoder_Set_Header(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Set_Header_Result_T *ptHeader)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
		ret = (VideoEncoder_Error_E)ve_res_set_seq_header(ptVideoEncoder->ptResource, ptHeader);
	}

	return ret;
}

VideoEncoder_Error_E VideoEncoder_Init(VideoEncoder_T *ptVideoEncoder, const VideoEncoder_InitConfig_T *ptConfig)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
		if (SF_VideoEncoder_SetCodec(ptVideoEncoder, ptConfig->eCodec) != VideoEncoder_Error_None) {
			ret = VideoEncoder_Error_InitFail;
		} else {
			int iVal = ve_res_init_resource(ptVideoEncoder->ptResource, ptConfig);

			if (iVal != 0) {
				ret = VideoEncoder_Error_InitFail;
			}
		}
	}

	return ret;
}

VideoEncoder_Error_E VideoEncoder_Deinit(const VideoEncoder_T *ptVideoEncoder)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
		ve_res_deinit_resource(ptVideoEncoder->ptResource);
	}

	return ret;
}

VideoEncoder_T *VideoEncoder_Create(void)
{
	VideoEncoder_T *ptVideoEncoder = NULL;

	void *tmpPtr = VE_CALLOC(sizeof(VideoEncoder_T));

	if (tmpPtr != NULL) {
		VE_CAST(ptVideoEncoder, tmpPtr);
	} else {
		VE_ERROR_MSG("[%s(%d) [ERROR] calloc failed! \n", __func__, __LINE__);
	}

	return ptVideoEncoder;
}

VideoEncoder_Error_E VideoEncoder_Destroy(VideoEncoder_T *ptVideoEncoder)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
		if (ptVideoEncoder->eCodec != VideoEncoder_Codec_None) {
			ve_res_destory_resource(ptVideoEncoder->ptResource);
		}

		VE_FREE(ptVideoEncoder);
	}

	return ret;
}

static VideoEncoder_Error_E SF_VideoEncoder_SetCodec(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Codec_E eCodec)
{
	VideoEncoder_Error_E ret = (VideoEncoder_Error_E)VideoEncoder_Error_None;

	if (ptVideoEncoder == NULL) {
		VE_ERROR_MSG("[%s(%d) [ERROR] input \n", __func__, __LINE__);
		ret = VideoEncoder_Error_BadParameter;
	} else {
#if defined(TCC_VPU_LEGACY)
		ptVideoEncoder->eType = VE_TYPE_VPU_LEGACY;
#elif defined(TCC_VPU_V3)
		ptVideoEncoder->eType = VE_TYPE_VPU_V3;
#endif
		ptVideoEncoder->ptResource = ve_res_create_resource(eCodec, ptVideoEncoder->eType);

		if (ptVideoEncoder->ptResource != NULL) {
			ptVideoEncoder->eCodec = eCodec;
		} else {
			VE_ERROR_MSG("[%s(%d) [ERROR] RESOURCE \n", __func__, __LINE__);
			ptVideoEncoder->ptResource = NULL;
			ptVideoEncoder->eCodec = VideoEncoder_Codec_None;
			ret = VideoEncoder_Error_NotSupportedCodec;
		}
	}

	return ret;
}