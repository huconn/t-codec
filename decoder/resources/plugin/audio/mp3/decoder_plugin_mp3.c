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
#include <stdbool.h>
#include <stdint.h>

#include <dlfcn.h>

#include <TU_JP_Lib_C_Std.h>

#include <adec.h>
#include <tcc_audio_common.h>

#include <decoder_plugin.h>
#include <decoder_plugin_audio.h>

#include <glib.h>
#include <glib/gprintf.h>

#define CODEC_MP3_DEC_LIB_NAME	"libtccmp3dec.so"

typedef struct
{
	int		m_iLayer;
	//!     DAB mode selection for layer 2 decoding ( 0: OFF,  1: ON)
	int     m_iDABMode;

	//!     reserved for future needs
	int     reserved[32-2];
} ST_MP3_DecoderPrivate_T;

typedef struct
{
	int iMinStreamSize;

	unsigned long hDecoder;

	void *hLib;

	fnCdkAudioDec *pfDecFunc;
	decoder_func_list_t* pDecFunctionList;

	adec_init_t tDecInit;
	audio_pcminfo_t tPcmInfo;
	audio_streaminfo_t tStreamInfo;

	ST_Stream_T tInputStream;
} DEC_Plugin_MP3_T;

static int SF_MP3_LoadLibrary(DEC_Plugin_MP3_T *ptPlugin, const char *pstrLibName)
{
	decoder_func_list_t *ptCodecFunc;
	int ret;

	ptPlugin->hLib = dlopen(pstrLibName , 0x00101); //RTLD_LAZY | RTLD_GLOBAL, RTLD_LAZY=0x00001 RTLD_GLOBAL=0x00100

	if(ptPlugin->hLib != NULL)
	{
		ptCodecFunc = (decoder_func_list_t *)DecPlugin_CastAddr_VOID_UINTPTR(dlsym(ptPlugin->hLib, "FucntionList"));

		if(ptCodecFunc == NULL)
		{
			audio_printf("[MP3] dlsym failed! - %s\n", pstrLibName);

			(void)dlclose(ptPlugin->hLib);
			ptPlugin->hLib = NULL;

			ret = -1;
		}
		else
		{
			audio_printf("[MP3] %s Loaded\n", pstrLibName);

			ptPlugin->pfDecFunc = ptCodecFunc->pfMainFunction;
			ptPlugin->pDecFunctionList = (decoder_func_list_t *)ptCodecFunc;

			ret = 0;
		}
	}
	else
	{
		audio_printf("[MP3] dlopen failed! - %s\n", pstrLibName);
		ret = -1;
	}

	return ret;
}

static void SF_MP3_UnloadLibrary(DEC_Plugin_MP3_T *ptPlugin)
{
	if(ptPlugin->hLib != NULL)
	{
		(void)dlclose(ptPlugin->hLib);
		ptPlugin->hLib = NULL;
	}
}

static DEC_PLUGIN_H decoder_plugin_mp3_Open(void)
{
	int iVal;

	adec_init_t *ptDecInit;
	audio_pcminfo_t *ptPcmInfo;
	audio_streaminfo_t *ptStreamInfo;

	ST_MP3_DecoderPrivate_T *ptDecoderPrivate;

	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(g_malloc(sizeof(DEC_Plugin_MP3_T)));
	if (ptPlugin != NULL)
	{
		(void)memset(ptPlugin, 0, sizeof(DEC_Plugin_MP3_T));

		iVal = SF_MP3_LoadLibrary(ptPlugin, CODEC_MP3_DEC_LIB_NAME);

		if(iVal == 0)
		{
			ptPlugin->iMinStreamSize = 0;

			ptDecInit 		= &ptPlugin->tDecInit;
			ptPcmInfo 		= &ptPlugin->tPcmInfo;
			ptStreamInfo 	= &ptPlugin->tStreamInfo;

			ptDecInit->m_pfMalloc 	= g_malloc;
			ptDecInit->m_pfRealloc 	= g_realloc;
			ptDecInit->m_pfFree 	= g_free;
			ptDecInit->m_pfMemcpy 	= memcpy;
			ptDecInit->m_pfMemmove 	= memmove;
			ptDecInit->m_pfMemset 	= memset;

			ptDecInit->m_pucExtraData 	= NULL;
			ptDecInit->m_iExtraDataLen 	= 0;

			ptDecInit->m_psAudiodecOutput 	= ptPcmInfo;
			ptDecInit->m_psAudiodecInput 	= ptStreamInfo;

			ptDecInit->m_iDownMixMode = 1;

			ptDecoderPrivate = (ST_MP3_DecoderPrivate_T *)DecPlugin_CastAddr_VOID_UINTPTR(ptDecInit->m_unAudioCodecParams);

			ptDecoderPrivate->m_iDABMode = 0;
			ptDecoderPrivate->m_iLayer = 3;

			ptPcmInfo->m_ePcmInterLeaved = (TCAS_U32)TCAS_ENABLE;
			ptPcmInfo->m_uiBitsPerSample = 16;

			ptPcmInfo->m_iNchannelOrder[CH_LEFT_FRONT] 		= 1;	//first channel
			ptPcmInfo->m_iNchannelOrder[CH_RIGHT_FRONT] 	= 2;	//second channel

			ptStreamInfo->m_uiNumberOfChannel 	= 2;
			ptStreamInfo->m_eSampleRate 		= 44100;
		}
		else
		{
			audio_free(ptPlugin);
			ptPlugin = NULL;
		}
	}

	return (DEC_PLUGIN_H)ptPlugin;
}

static int decoder_plugin_mp3_Close(DEC_PLUGIN_H hPlugin)
{
	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);
	if(ptPlugin != NULL)
	{
		SF_MP3_UnloadLibrary(ptPlugin);
		audio_free(ptPlugin);
	}
	return 0;
}

static int decoder_plugin_mp3_Init(DEC_PLUGIN_H hPlugin)
{
	int iRetVal = 0;
	int nLayer = 3;

	adec_init_t *ptDecInit;
	audio_pcminfo_t *ptPcmInfo;
	const audio_streaminfo_t *ptStreamInfo;

	const ST_MP3_DecoderPrivate_T *ptDecoderPrivate;

	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);

	ptDecInit 	= &ptPlugin->tDecInit;
	ptPcmInfo 	= &ptPlugin->tPcmInfo;
	ptStreamInfo 	= &ptPlugin->tStreamInfo;

	ptDecoderPrivate = (const ST_MP3_DecoderPrivate_T *)DecPlugin_CastAddr_VOID_UINTPTR(ptDecInit->m_unAudioCodecParams);

	if(ptStreamInfo->m_uiNumberOfChannel > 2u)
	{
		ptDecInit->m_iDownMixMode = 0;
		ptPcmInfo->m_iNchannelOrder[CH_CENTER] = 3;

		switch(ptStreamInfo->m_uiNumberOfChannel)
		{
		case 4:
			{
				ptPcmInfo->m_iNchannelOrder[CH_LFE] = 4;
			}
			break;
		case 5:
			{
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_REAR] 	= 4;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_REAR] 	= 5;
			}
			break;
		case 6:
			{
				ptPcmInfo->m_iNchannelOrder[CH_LFE] 		= 4;
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_REAR] 	= 5;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_REAR] 	= 6;
			}
			break;
		case 7:
			{
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_REAR] 	= 4;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_REAR] 	= 5;
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_SIDE] 	= 6;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_SIDE] 	= 7;
			}
			break;
		case 8:
			{
				ptPcmInfo->m_iNchannelOrder[CH_LFE] 		= 4;
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_REAR] 	= 5;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_REAR] 	= 6;
				ptPcmInfo->m_iNchannelOrder[CH_LEFT_SIDE] 	= 7;
				ptPcmInfo->m_iNchannelOrder[CH_RIGHT_SIDE] 	= 8;
			}
			break;
		default:
			{
				ptDecInit->m_iDownMixMode = 1;
			}
			break;
		}
	}

	audio_printf("[MP3] load Layer-%d \n", ptDecoderPrivate->m_iLayer);

	if (nLayer == ptDecoderPrivate->m_iLayer) {
		iRetVal = ptPlugin->pfDecFunc(AUDIO_INIT, &ptPlugin->hDecoder, ptDecInit, NULL);
	} else {
		nLayer = ptDecoderPrivate->m_iLayer;
		if ((nLayer > 0) && (nLayer < 3)){
			ptPlugin->pfDecFunc = (fnCdkAudioDec *)ptPlugin->pDecFunctionList->pfCodecSpecific[nLayer-1];
			iRetVal = ptPlugin->pfDecFunc(AUDIO_INIT, &ptPlugin->hDecoder, ptDecInit, NULL);
		} else {
			iRetVal = -1;
		}
	}

	if (iRetVal >= 0) {
		audio_printf("[%s %d] In: %d %d, Out: %d %d\n",__func__,__LINE__,
			ptStreamInfo->m_uiNumberOfChannel,ptStreamInfo->m_eSampleRate,
			ptPcmInfo->m_uiNumberOfChannel,ptPcmInfo->m_eSampleRate);
	}

	return iRetVal;
}

static int decoder_plugin_mp3_Deinit(DEC_PLUGIN_H hPlugin)
{
	int iRetVal;

	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);

	ST_Stream_T *ptInputStream = &ptPlugin->tInputStream;

	iRetVal = ptPlugin->pfDecFunc(AUDIO_CLOSE, &ptPlugin->hDecoder, NULL, NULL);

	ptPlugin->hDecoder = 0;

	if(ptInputStream->pucStream != NULL)
	{
		audio_free(ptInputStream->pucStream);
		ptInputStream->pucStream = NULL;
	}

	ptInputStream->uiStreamLen 	= 0;
	ptInputStream->uiOffset 	= 0;

	return iRetVal;
}

static int decoder_plugin_mp3_Decode(DEC_PLUGIN_H hPlugin, unsigned int uiStreamLen, const unsigned char *pucStream, void *pvDecodedStream)
{
	unsigned int uiOffset = 0;
	unsigned int uiSamplesPerCh = 0;

	DP_Audio_Error_E eRetErr = DP_Audio_ErrorNone;

	audio_pcminfo_t *ptPcmInfo;
	audio_streaminfo_t *ptStreamInfo;

	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);

	DP_Audio_Stream_T *ptDecodedStream = (DP_Audio_Stream_T *)DecPlugin_CastAddr_VOID_UINTPTR(pvDecodedStream);

	ST_Stream_T *ptInputStream = &ptPlugin->tInputStream;

	(void)memset(ptDecodedStream, 0, sizeof(DP_Audio_Stream_T));

	if(ptInputStream->uiStreamLen == 0u)
	{
		ptInputStream->pucStream = DecPlugin_CastAddr_VOID_UCHARPTR(g_malloc(uiStreamLen));
		if(ptInputStream->pucStream == NULL)
		{
			eRetErr = DP_Audio_ErrorDecode;
		}
	}
	else
	{
		unsigned char *pucTemp = ptInputStream->pucStream;
		unsigned int mallocsize;
		(void)__builtin_uadd_overflow(ptInputStream->uiStreamLen, uiStreamLen, &mallocsize);
		ptInputStream->pucStream = DecPlugin_CastAddr_VOID_UCHARPTR(g_malloc(mallocsize));
		if(ptInputStream->pucStream == NULL)
		{
			eRetErr = DP_Audio_ErrorDecode;
		}
		else
		{
			(void)memcpy(ptInputStream->pucStream, &pucTemp[ptInputStream->uiOffset], ptInputStream->uiStreamLen);
			audio_free(pucTemp);
		}
	}

	if (eRetErr == DP_Audio_ErrorNone)
	{
		ptInputStream->uiOffset = 0;

		(void)memcpy(&ptInputStream->pucStream[ptInputStream->uiStreamLen], pucStream, uiStreamLen);

		(void)__builtin_uadd_overflow(ptInputStream->uiStreamLen, uiStreamLen, &ptInputStream->uiStreamLen);

		ptPcmInfo 		= &ptPlugin->tPcmInfo;
		ptStreamInfo 	= &ptPlugin->tStreamInfo;

		ptStreamInfo->m_pcStream 		= ptInputStream->pucStream;
		ptStreamInfo->m_iStreamLength 	= (int)ptInputStream->uiStreamLen;

		while(ptStreamInfo->m_iStreamLength > ptPlugin->iMinStreamSize)
		{
			TCAS_ERROR_TYPE eError;

			ptPcmInfo->m_pvChannel[0] = (void *)(&ptDecodedStream->ucStream[uiOffset]);

			eError = (TCAS_ERROR_TYPE)ptPlugin->pfDecFunc(AUDIO_DECODE, &ptPlugin->hDecoder, ptStreamInfo, ptPcmInfo);

			if((eError == TCAS_SUCCESS) || (eError == TCAS_ERROR_CONCEALMENT_APPLIED))
			{
				//uiOffset += (ptPcmInfo->m_uiNumberOfChannel * ptPcmInfo->m_uiSamplesPerChannel * sizeof(short));
				unsigned int curOffset;
				(void)__builtin_umul_overflow(ptPcmInfo->m_uiNumberOfChannel, ptPcmInfo->m_uiSamplesPerChannel, &curOffset);
				(void)__builtin_umul_overflow(curOffset, sizeof(short), &curOffset);
				(void)__builtin_uadd_overflow(uiOffset, curOffset, &uiOffset);

				//uiSamplesPerCh += ptPcmInfo->m_uiSamplesPerChannel;
				(void)__builtin_uadd_overflow(uiSamplesPerCh, ptPcmInfo->m_uiSamplesPerChannel, &uiSamplesPerCh);
			}
			else
			{
				if(eError == TCAS_ERROR_MORE_DATA)
				{
					eRetErr = DP_Audio_ErrorMoreData;
				}
				else
				{
					eRetErr = DP_Audio_ErrorDecode;
				}
				break;
			}
		}

		if(eRetErr == DP_Audio_ErrorDecode)
		{
			audio_free(ptInputStream->pucStream);
			ptInputStream->pucStream = NULL;

			ptInputStream->uiStreamLen 	= 0;
			ptInputStream->uiOffset 	= 0;
		}
		else
		{
			if(ptStreamInfo->m_iStreamLength > 0)
			{
				ptInputStream->uiOffset = (ptInputStream->uiStreamLen >= (unsigned int)ptStreamInfo->m_iStreamLength) ?
								(ptInputStream->uiStreamLen - (unsigned int)ptStreamInfo->m_iStreamLength) : (0u);
				ptInputStream->uiStreamLen 	= (unsigned int)ptStreamInfo->m_iStreamLength;

				ptDecodedStream->bMultiFrame = true;
			}
			else
			{
				audio_free(ptInputStream->pucStream);
				ptInputStream->pucStream = NULL;

				ptInputStream->uiStreamLen 	= 0;
				ptInputStream->uiOffset 	= 0;
			}

			ptDecodedStream->uiStreamLen 		= uiOffset;
			ptDecodedStream->uiSamplesPerCh 	= uiSamplesPerCh;
		}
	}

	return (int)eRetErr;
}

static int decoder_plugin_mp3_Set(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	DEC_Plugin_MP3_T *ptPlugin = (DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);

	DP_Audio_Config_E eConfig;
	DecPlugin_Id2Config(uiId, eConfig);

	switch(eConfig)
	{
	case DP_Audio_Config_Descrioptor:
		{
			adec_init_t *ptDecInit;
			audio_pcminfo_t *ptPcmInfo;
			audio_streaminfo_t *ptStreamInfo;

			ST_MP3_DecoderPrivate_T *ptDecoderPrivate;

			const DP_Audio_Descriptor_T *ptDescriptor = (const DP_Audio_Descriptor_T *)DecPlugin_CastAddr_VOID_UINTPTR(pvStructure);

			ptDecInit 	= &ptPlugin->tDecInit;
			ptPcmInfo 	= &ptPlugin->tPcmInfo;
			ptStreamInfo 	= &ptPlugin->tStreamInfo;

			audio_printf("[MP3_SET] Descriptor Info[Stream: %d %d %d Pcm:%d CodecData:%p %d]\n",
				ptDescriptor->tStream.uiChannels,ptDescriptor->tStream.uiSampleRate,ptDescriptor->tStream.uiBitsPerSample,
				ptDescriptor->tPcm.uiBitsPerSample,ptDescriptor->codecdata,ptDescriptor->codecdataLen);

			if(ptDescriptor->codecdataLen > 0)
			{
				ptDecInit->m_pucExtraData = DecPlugin_CastAddr_VOID_UCHARPTR(g_malloc((unsigned int)ptDescriptor->codecdataLen));
				(void)memcpy(ptDecInit->m_pucExtraData, ptDescriptor->codecdata, (unsigned int)ptDescriptor->codecdataLen);
				ptDecInit->m_iExtraDataLen = ptDescriptor->codecdataLen;
			}

			ptPcmInfo->m_uiBitsPerSample = ptDescriptor->tPcm.uiBitsPerSample;

			ptStreamInfo->m_uiNumberOfChannel 	= ptDescriptor->tStream.uiChannels;
			ptStreamInfo->m_eSampleRate 		= ptDescriptor->tStream.uiSampleRate;
			ptStreamInfo->m_uiBitsPerSample		= ptDescriptor->tStream.uiBitsPerSample;

			ptDecoderPrivate = (ST_MP3_DecoderPrivate_T *)DecPlugin_CastAddr_VOID_UINTPTR(ptDecInit->m_unAudioCodecParams);

			ptDecoderPrivate->m_iDABMode = ptDescriptor->mp3.iDABMode;
			ptDecoderPrivate->m_iLayer = ptDescriptor->mp3.iLayer;
		}
		break;
	default:
		{
			//ToDo
		}
		break;
	}

	return iRetVal;
}

static int decoder_plugin_mp3_Get(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure)
{
	int iRetVal = 0;

	const DEC_Plugin_MP3_T *ptPlugin = (const DEC_Plugin_MP3_T *)DecPlugin_CastAddr_VOID_UINTPTR(hPlugin);

	DP_Audio_Config_E eConfig;
	DecPlugin_Id2Config(uiId, eConfig);

	switch(eConfig)
	{
	case DP_Audio_Config_Pcm:
		{
			const audio_pcminfo_t *ptPcmInfo;
			//audio_streaminfo_t *ptStreamInfo;

			DP_Audio_Pcm_T *ptPcm = (DP_Audio_Pcm_T *)DecPlugin_CastAddr_VOID_UINTPTR(pvStructure);

			ptPcmInfo 	= &ptPlugin->tPcmInfo;
			//ptStreamInfo 	= &ptPlugin->tStreamInfo;

			ptPcm->uiChannels		= ptPcmInfo->m_uiNumberOfChannel;
			ptPcm->uiBitsPerSample 	= ptPcmInfo->m_uiBitsPerSample;
			ptPcm->uiSampleRate 	= (unsigned int)ptPcmInfo->m_eSampleRate;
		}
		break;
	default:
		{
			//ToDo
		}
		break;
	}

	return iRetVal;
}

DECODER_PLUGIN_DEFINE(
	"mp3",
	decoder_plugin_mp3_Open,
	decoder_plugin_mp3_Close,
	decoder_plugin_mp3_Init,
	decoder_plugin_mp3_Deinit,
	decoder_plugin_mp3_Decode,
	decoder_plugin_mp3_Set,
	decoder_plugin_mp3_Get)
