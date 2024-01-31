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

#ifndef VIDEO_DECODER_RESOURCE_H
#define VIDEO_DECODER_RESOURCE_H


#include "video_decoder.h"


typedef enum
{
	VD_TYPE_NONE = 0,
	VD_TYPE_VPU_LEGACY,
	VD_TYPE_VPU_V3,
} VD_RESOURCE_TYPE_T;

typedef struct VD_RESOURCE_T
{
	VideoDecoder_Codec_E eCodec;
	VD_RESOURCE_TYPE_T eType;

	void *ptResourceContext;
} VD_RESOURCE_T;


VD_RESOURCE_T *vd_res_create_resource(VideoDecoder_Codec_E eCodec, VD_RESOURCE_TYPE_T eVDType);
void vd_res_destory_resource(VD_RESOURCE_T *ptResource);
bool vd_res_init_resource(VD_RESOURCE_T *ptResource, const VideoDecoder_InitConfig_T *ptInit);
void vd_res_deinit_resource(VD_RESOURCE_T *ptResource);

unsigned int vd_res_get_CbCrInterleaveMode(VD_RESOURCE_T *ptResource);

int vd_res_release_frame(VD_RESOURCE_T *ptResource, int iDisplayIndex);
int vd_res_decode(VD_RESOURCE_T *ptResource, const VideoDecoder_StreamInfo_T *ptStream, VideoDecoder_DecodingResult_T *ptDecodingResult);
bool vd_res_decode_seq_header(VD_RESOURCE_T *ptResource, const VideoDecoder_SequenceHeaderInputParam_T *ptParam);


#endif	// VIDEO_DECODER_RESOURCE_H