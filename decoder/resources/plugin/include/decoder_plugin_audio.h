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

#ifndef DECODER_PLUGIN_AUDIO_H__
#define DECODER_PLUGIN_AUDIO_H__

#include <stdio.h>
#include <stdint.h>

#define audio_printf (void)g_printf
#define audio_free (void)g_free

#define DP_AUDIO_STREAM_BUFFER_SIZE	(1024 * 1024)

typedef enum
{
	DP_Audio_ErrorNone = 0,
	DP_Audio_ErrorDecode = -1,
	DP_Audio_ErrorMoreData = -2
} DP_Audio_Error_E;

typedef enum
{
	DP_Audio_Config_Descrioptor,
	DP_Audio_Config_Pcm,
	DP_Audio_Config_MAX
} DP_Audio_Config_E;

typedef enum
{
	DP_Audio_AAC_Profile_Main,
	DP_Audio_AAC_Profile_LTP,
	DP_Audio_AAC_Profile_LC,
	DP_Audio_AAC_Profile_HE,
	DP_Audio_AAC_Profile_HE_PS
} DP_Audio_AAC_Profile_E;

typedef enum
{
	DP_Audio_AAC_StreamFormat_MP2ADTS,
	DP_Audio_AAC_StreamFormat_ADTS,
	DP_Audio_AAC_StreamFormat_MP4LOAS,
	DP_Audio_AAC_StreamFormat_MP4LATM,
	DP_Audio_AAC_StreamFormat_ADIF,
	DP_Audio_AAC_StreamFormat_MP4FF,
	DP_Audio_AAC_StreamFormat_RAW
} DP_Audio_AAC_StreamFormat_E;

typedef struct
{
	unsigned int uiChannels;
	unsigned int uiBitsPerSample;
	unsigned int uiSampleRate;
} DP_Audio_Info_T;

typedef struct
{
	unsigned int uiChannels;
	unsigned int uiBitsPerSample;
	unsigned int uiSampleRate;
} DP_Audio_Pcm_T;

typedef struct
{
	DP_Audio_Pcm_T tPcm;
	DP_Audio_Info_T tStream;

	struct
	{
		DP_Audio_AAC_Profile_E eProfile;
		DP_Audio_AAC_StreamFormat_E eStreamFormat;
	} aac;

	struct
	{
		int iLayer;
		int iDABMode;
	} mp3;

	struct
	{
		int iWAVForm_TagID;
		unsigned int uiNBlockAlign;
		int iEndian;
		int iContainerType;
	} pcm;

	void* codecdata;
	int codecdataLen;
} DP_Audio_Descriptor_T;

typedef struct
{
	bool bMultiFrame;

	unsigned int uiStreamLen;
	unsigned int uiSamplesPerCh;

	unsigned char ucStream[DP_AUDIO_STREAM_BUFFER_SIZE];
} DP_Audio_Stream_T;

typedef struct
{
	unsigned int uiStreamLen;
	unsigned int uiOffset;

	unsigned char *pucStream;
} ST_Stream_T;

#define DecPlugin_Id2Config(uiId, e_config) {\
	if ((uiId) == (unsigned int)DP_Audio_Config_Pcm) { \
		(e_config) = DP_Audio_Config_Pcm; \
	} \
	else if ((uiId) == (unsigned int)DP_Audio_Config_Descrioptor) { \
		(e_config) = DP_Audio_Config_Descrioptor; \
	} \
	else { \
		(e_config) = DP_Audio_Config_MAX; \
	} \
}

#endif	// DECODER_PLUGIN_AUDIO_H__
