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

#ifndef VIDEO_ENCODER_RESOURCE_H
#define VIDEO_ENCODER_RESOURCE_H


#include "video_encoder.h"


typedef enum
{
	VE_TYPE_NONE = 0,
	VE_TYPE_VPU_LEGACY,
	VE_TYPE_VPU_V3,

} VE_RESOURCE_TYPE_T;

typedef struct VE_RESOURCE_T
{
	VideoEncoder_Codec_E eCodec;
	VE_RESOURCE_TYPE_T eType;

	void *ptResourceContext;
} VE_RESOURCE_T;


VE_RESOURCE_T *ve_res_create_resource(VideoEncoder_Codec_E eCodec, VE_RESOURCE_TYPE_T eVEType);
void ve_res_destory_resource(VE_RESOURCE_T *ptResource);
int ve_res_init_resource(VE_RESOURCE_T *ptResource, const VideoEncoder_InitConfig_T *ptInit);
void ve_res_deinit_resource(VE_RESOURCE_T *ptResource);

int ve_res_set_seq_header(VE_RESOURCE_T *ptResource, VideoEncoder_Set_Header_Result_T *ptHeader);
int ve_res_encode(VE_RESOURCE_T *ptResource, VideoEncoder_Encode_Input_T *ptEncode, VideoEncoder_Encode_Result_T *ptEncodeResult);


#endif	// VIDEO_ENCODER_RESOURCE_H