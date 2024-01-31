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

#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <stdint.h>


#define VE_IN_Y 	(0)
#define VE_IN_U 	(1)
#define VE_IN_V 	(2)
#define VE_IN_MAX 	(3)


typedef struct VideoEncoder_T VideoEncoder_T;


typedef enum
{
	VideoEncoder_None = 0,
	VideoEncoder_Max
} VideoEncoder_E;

typedef enum
{
	VideoEncoder_SliceSizeUnit_Byte = 0,
	VideoEncoder_SliceSizeUnit_MacroBlock
} VideoEncoder_SliceSizeUnit_E;

typedef enum
{
	VideoEncoder_SliceMode_Single = 0,
	VideoEncoder_SliceMode_Multi
} VideoEncoder_SliceMode_E;

typedef enum
{
	VideoEncoder_Codec_None = 0,
	VideoEncoder_Codec_H264,
	VideoEncoder_Codec_H265,
	VideoEncoder_Codec_JPEG,
	VideoEncoder_Codec_MAX
} VideoEncoder_Codec_E;

typedef enum
{
	VideoEncoder_Codec_Picture_UnKnown = 0,
	VideoEncoder_Codec_Picture_I,
	VideoEncoder_Codec_Picture_P
} VideoEncoder_Codec_Picture_E;

typedef enum
{
	VideoEncoder_Error_None = 0,
	VideoEncoder_Error_BadParameter,
	VideoEncoder_Error_InitFail,
	VideoEncoder_Error_EncodingFail,
	VideoEncoder_Error_NotSupportedCodec,
	VideoEncoder_Error_Unrecoverable
} VideoEncoder_Error_E;

/**
 * This structure holds configurations for VideoEncoder_Init() function */
typedef struct
{
	VideoEncoder_Codec_E eCodec;

	int iImageWidth;
	int iImageHeight;

	int iFrameRate;
	int iKeyFrameInterval;
	int iBitRate;

	int iSliceMode;
	int iSliceSizeUnit;
	int iSliceSize;
} VideoEncoder_InitConfig_T;

/**
 * This structure holds configurations for VideoEncoder_Encode() function */
typedef struct
{
	signed long long llTimeStamp;
	unsigned char *pucVideoBuffer[VE_IN_MAX];
} VideoEncoder_Encode_Input_T;

/**
 * This structure holds results of VideoEncoder_Set_Header() function */
typedef struct
{
	int iHeaderSize;
	unsigned char *pucBuffer;
} VideoEncoder_Set_Header_Result_T;

/**
 * This structure holds results of VideoEncoder_Encode() function */
typedef struct
{
	VideoEncoder_Codec_Picture_E ePictureType;
	int iFrameSize;
	unsigned char *pucBuffer;
} VideoEncoder_Encode_Result_T;


VideoEncoder_Error_E VideoEncoder_Set_Header(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Set_Header_Result_T *ptHeader);
VideoEncoder_Error_E VideoEncoder_Encode(VideoEncoder_T *ptVideoEncoder, VideoEncoder_Encode_Input_T *ptEncode, VideoEncoder_Encode_Result_T *ptEncodeResult);
VideoEncoder_Error_E VideoEncoder_Init(VideoEncoder_T *ptVideoEncoder, const VideoEncoder_InitConfig_T *ptConfig);
VideoEncoder_Error_E VideoEncoder_Deinit(const VideoEncoder_T *ptVideoEncoder);
VideoEncoder_T *VideoEncoder_Create(void);
VideoEncoder_Error_E VideoEncoder_Destroy(VideoEncoder_T *ptVideoEncoder);


#endif	// VIDEO_ENCODER_H