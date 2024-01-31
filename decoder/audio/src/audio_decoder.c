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

#include <glib.h>
#include <glib/gprintf.h>

#include <TU_JP_Lib_C_Std.h>

#include <decoder_plugin.h>
#include <decoder_plugin_audio.h>
#include <decoder_plugin_interface.h>

#include "audio_decoder.h"

struct AudioDecoder_T
{
	AudioDecoder_Codec_E eCodec;
	DEC_PLUGIN_IF_H hPlugin_IF;
	DEC_PLUGIN_H hPlugin;
	DecoderPluginDesc_T *ptPluginDesc;
	AudioDecoder_Pcm_T tPcm;
};

static AudioDecoder_Error_E SF_ADec_SetCodec(AudioDecoder_T *ptAudioDecoder, AudioDecoder_Codec_E eCodec)
{
	char strPlugin[50];

	AudioDecoder_Error_E eRetErr = AudioDecoder_ErrorNone;

	const DecoderPluginDesc_T *ptPluginDesc;

	if(ptAudioDecoder == NULL)
	{
		eRetErr = AudioDecoder_ErrorBadParameter;
	}
	else
	{
		ptAudioDecoder->eCodec = eCodec;

		switch(eCodec)
		{
		case AudioDecoder_Codec_AAC:
		{
			(void)g_sprintf(strPlugin, "aac");
		}
		break;
		case AudioDecoder_Codec_MP3:
		{
			(void)g_sprintf(strPlugin, "mp3");
		}
		break;
		case AudioDecoder_Codec_PCM:
		{
			(void)g_sprintf(strPlugin, "pcm");
		}
		break;
		case AudioDecoder_Codec_DDP:
		{
			(void)g_sprintf(strPlugin, "ddp");
		}
		break;
		default:
		{
			eRetErr = AudioDecoder_ErrorNotSupportedCodec;
		}
		break;
		}

		if(eRetErr == AudioDecoder_ErrorNone)
		{
			(void)g_printf("[%s:%d] eCodec %d(%s)\n",__func__,__LINE__,eCodec,strPlugin);
			ptAudioDecoder->hPlugin_IF = decoder_plugin_interface_Load(strPlugin, &ptAudioDecoder->ptPluginDesc);

			if(ptAudioDecoder->hPlugin_IF != NULL)
			{
				ptPluginDesc = ptAudioDecoder->ptPluginDesc;
				ptAudioDecoder->hPlugin = ptPluginDesc->open();
				if(ptAudioDecoder->hPlugin != NULL)
				{
					eRetErr = AudioDecoder_ErrorNone;
				}
				else
				{
					decoder_plugin_interface_Unload(ptAudioDecoder->hPlugin_IF);
					ptAudioDecoder->hPlugin_IF = NULL;
					eRetErr = AudioDecoder_ErrorBadParameter;
				}
			}
			if(ptAudioDecoder->hPlugin_IF == NULL)
			{
				eRetErr = AudioDecoder_ErrorNotSupportedCodec;
			}
		}
	}

	return eRetErr;
}

AudioDecoder_Error_E AudioDecoder_Decode(
	AudioDecoder_T *ptAudioDecoder,
	const AudioDecoder_StreamInfo_T *ptStream,
	AudioDecoder_Pcm_T *ptPcm)
{
	AudioDecoder_Error_E eRetErr = AudioDecoder_ErrorNone;
	DP_Audio_Error_E eError;

	DP_Audio_Pcm_T tPcm;
	DP_Audio_Stream_T tDecodedPCM;
	const DecoderPluginDesc_T *ptPluginDesc;

	if((ptAudioDecoder == NULL) || (ptStream == NULL) || (ptPcm == NULL))
	{
		eRetErr = AudioDecoder_ErrorBadParameter;
	}
	else if(ptAudioDecoder->eCodec == AudioDecoder_Codec_None)
	{
		eRetErr = AudioDecoder_ErrorDecodingFail;
	}
	else
	{
		ptPluginDesc = ptAudioDecoder->ptPluginDesc;

		eError = (DP_Audio_Error_E)ptPluginDesc->decode(ptAudioDecoder->hPlugin,
			ptStream->uiStreamLen, ptStream->pucStream, &tDecodedPCM);

		if((eError != DP_Audio_ErrorNone) && (eError != DP_Audio_ErrorMoreData))
		{
			eRetErr = AudioDecoder_ErrorDecodingFail;
		}
		else
		{
			(void)ptPluginDesc->get(ptAudioDecoder->hPlugin, DP_Audio_Config_Pcm, &tPcm);

			if(tPcm.uiChannels == 2u)
			{
				ptPcm->stereo.uiBufferLen = tDecodedPCM.uiStreamLen;
				(void)memcpy(ptPcm->stereo.ucOutputBuffer, tDecodedPCM.ucStream, tDecodedPCM.uiStreamLen);
			}

			ptPcm->uiChannels 		= tPcm.uiChannels;
			ptPcm->uiBitsPerSample 	= tPcm.uiBitsPerSample;
			ptPcm->uiSampleRate 	= tPcm.uiSampleRate;

			// Store last decoded infomation for future purpose
			(void)memcpy(&ptAudioDecoder->tPcm, ptPcm, sizeof(AudioDecoder_Pcm_T));

			eRetErr = AudioDecoder_ErrorNone;
		}
	}

	return eRetErr;
}

//Currently Unused code
/*
AudioDecoder_Error_E AudioDecoder_Flush(AudioDecoder_T *ptAudioDecoder)
{
	return AudioDecoder_ErrorNotSupportedCodec;
}
*/

AudioDecoder_Error_E AudioDecoder_Init(AudioDecoder_T *ptAudioDecoder, AudioDecoder_Codec_E eCodec, const AudioDecoder_CodecInfo_T *ptCodecInfo)
{
	AudioDecoder_Error_E eRetErr = AudioDecoder_ErrorNone;
	int iVal;
	DP_Audio_Descriptor_T tCodecInfo = {0, };
	const DecoderPluginDesc_T *ptPluginDesc;

	if((ptAudioDecoder == NULL) || (ptCodecInfo == NULL))
	{
		eRetErr = AudioDecoder_ErrorBadParameter;
	}
	else
	{
		eRetErr = SF_ADec_SetCodec(ptAudioDecoder, eCodec);
		if(eRetErr == AudioDecoder_ErrorNotSupportedCodec)
		{
			(void)g_printf("AudioCodecNum %d is not supported codec\n",ptAudioDecoder->eCodec);
		}
		else if ((eRetErr == AudioDecoder_ErrorBadParameter) || (ptAudioDecoder->eCodec == AudioDecoder_Codec_None))
		{
			(void)g_printf("[%s:%d] Failed to set audio codec (err:%d, eCodec:%d)\n", __func__, __LINE__, eRetErr, ptAudioDecoder->eCodec);
			eRetErr = AudioDecoder_ErrorInitFail;
		}
		else // eRetErr = AudioDecoder_ErrorNone
		{
			(void)g_printf("[AUDIO] CodecInfo [Ch: %u][BPS: %u][Samplerate: %u][Codecdate : %p][CodecDateLen: %lu]\n",
				ptCodecInfo->uiChannels, ptCodecInfo->uiBitsPerSample, ptCodecInfo->uiSampleRate,
				ptCodecInfo->codecdata, ptCodecInfo->codecdataLen);

			tCodecInfo.tStream.uiChannels		= ptCodecInfo->uiChannels;
			tCodecInfo.tStream.uiBitsPerSample	= ptCodecInfo->uiBitsPerSample;
			tCodecInfo.tStream.uiSampleRate 	= ptCodecInfo->uiSampleRate;

			tCodecInfo.tPcm.uiChannels 		= ptCodecInfo->uiChannels;
			tCodecInfo.tPcm.uiBitsPerSample = ptCodecInfo->uiBitsPerSample;
			tCodecInfo.tPcm.uiSampleRate 	= ptCodecInfo->uiSampleRate;

			tCodecInfo.codecdata = ptCodecInfo->codecdata;
			if (ptCodecInfo->codecdataLen < (unsigned int)INT32_MAX) {
				tCodecInfo.codecdataLen = (int)ptCodecInfo->codecdataLen;
			}

			switch(ptAudioDecoder->eCodec)
			{
			case AudioDecoder_Codec_AAC:
				{
					tCodecInfo.aac.eProfile 		= (DP_Audio_AAC_Profile_E)ptCodecInfo->aac.eProfile;
					tCodecInfo.aac.eStreamFormat 	= (DP_Audio_AAC_StreamFormat_E)ptCodecInfo->aac.eStreamFormat;
				}
				break;
			case AudioDecoder_Codec_MP3:
				{
					tCodecInfo.mp3.iLayer 		= ptCodecInfo->mp3.iLayer;
					tCodecInfo.mp3.iDABMode		= ptCodecInfo->mp3.iDABMode;
				}
				break;
			case AudioDecoder_Codec_PCM:
				{
					tCodecInfo.pcm.iWAVForm_TagID 	= ptCodecInfo->pcm.iWAVForm_TagID;
					tCodecInfo.pcm.uiNBlockAlign	= ptCodecInfo->pcm.uiNBlockAlign;
					tCodecInfo.pcm.iEndian			= ptCodecInfo->pcm.iEndian;
					tCodecInfo.pcm.iContainerType	= ptCodecInfo->pcm.iContainerType;
				}
				break;
			case AudioDecoder_Codec_DDP:
				{
					//tCodecInfo.ddp.is_ddp = ptCodecInfo->ddp.is_ddp;
					//tCodecInfo.ddp.lfe = ptCodecInfo->ddp.lfe;
					//tCodecInfo.ddp.eStereoMode = ptCodecInfo->ddp.eStereoMode;
					//tCodecInfo.ddp.tDrcParams = ptCodecInfo->ddp.tDrcParams;
				}
				break;
			default:
				{
					//AudioDecoder_Codec_None
				}
				break;
			}
			ptPluginDesc = ptAudioDecoder->ptPluginDesc;

			iVal = ptPluginDesc->set(ptAudioDecoder->hPlugin, DP_Audio_Config_Descrioptor, &tCodecInfo);

			if(iVal != 0)
			{
				(void)g_printf("[%s:%d] Failed to set audio decoder \n", __func__, __LINE__);
				eRetErr = AudioDecoder_ErrorInitFail;
			}
			else
			{
				iVal = ptPluginDesc->init(ptAudioDecoder->hPlugin, NULL);

				if(iVal != 0)
				{
					(void)g_printf("[%s:%d] Failed to init audio decoder \n", __func__, __LINE__);
					eRetErr = AudioDecoder_ErrorInitFail;
				}
				else
				{
					//re-set PCM info
					(void)ptPluginDesc->get(ptAudioDecoder->hPlugin, DP_Audio_Config_Pcm, &tCodecInfo.tPcm);

					(void)g_printf("[AUDIO Init] INPUT - [Ch: %d][BPS: %d][Samplerate: %d]\n",
						tCodecInfo.tStream.uiChannels, tCodecInfo.tStream.uiBitsPerSample,tCodecInfo.tStream.uiSampleRate);
					(void)g_printf("[AUDIO Init] OUTPUT- [Ch: %d][BPS: %d][Samplerate: %d]\n",
						tCodecInfo.tPcm.uiChannels, tCodecInfo.tPcm.uiBitsPerSample,tCodecInfo.tPcm.uiSampleRate);

					eRetErr = AudioDecoder_ErrorNone;
				}
			}
		}
	}
	return eRetErr;
}

AudioDecoder_Error_E AudioDecoder_Deinit(const AudioDecoder_T *ptAudioDecoder)
{
	AudioDecoder_Error_E eRetErr = AudioDecoder_ErrorNone;
	int iVal = -1;

	const DecoderPluginDesc_T *ptPluginDesc;

	if(ptAudioDecoder != NULL)
	{
		ptPluginDesc = ptAudioDecoder->ptPluginDesc;
		if (ptPluginDesc != NULL)
		{
			iVal = ptPluginDesc->deinit(ptAudioDecoder->hPlugin);
		}
		if(iVal != 0)
		{
			eRetErr = AudioDecoder_ErrorBadParameter;
		}
		else {
			eRetErr = AudioDecoder_ErrorNone;
		}
	}
	else
	{
		eRetErr = AudioDecoder_ErrorBadParameter;
	}

	return eRetErr;
}

AudioDecoder_T *AudioDecoder_Create(void)
{
	AudioDecoder_T *ptAudioDecoder;

	ptAudioDecoder = (AudioDecoder_T *)g_malloc0(sizeof(AudioDecoder_T));

	if (ptAudioDecoder != NULL)
	{
		ptAudioDecoder->eCodec = AudioDecoder_Codec_None;
		ptAudioDecoder->hPlugin_IF = NULL;
		ptAudioDecoder->hPlugin = NULL;
		ptAudioDecoder->ptPluginDesc = NULL;
	}
	else
	{
		(void)g_printf("[%s:%d] Failed to allocate memory for AudioDecoder_T\n", __func__, __LINE__);
	}

	return ptAudioDecoder;
}

AudioDecoder_Error_E AudioDecoder_Destroy(AudioDecoder_T *ptAudioDecoder)
{
	AudioDecoder_Error_E eRetErr = AudioDecoder_ErrorNone;

	if(ptAudioDecoder != NULL)
	{
		if(ptAudioDecoder->ptPluginDesc != NULL)
		{
			const DecoderPluginDesc_T *ptPluginDesc = ptAudioDecoder->ptPluginDesc;
			if((ptPluginDesc != NULL) && (ptAudioDecoder->hPlugin != NULL))
			{
				(void)ptPluginDesc->close(ptAudioDecoder->hPlugin);
			}
		}

		if(ptAudioDecoder->hPlugin_IF != NULL)
		{
			decoder_plugin_interface_Unload(ptAudioDecoder->hPlugin_IF);
		}

		g_free(ptAudioDecoder);
		eRetErr = AudioDecoder_ErrorNone;
	}
	else
	{
		eRetErr = AudioDecoder_ErrorBadParameter;
	}
	return eRetErr;
}

//TBD
/*
AudioDecoder_Error_E AudioDecoder_UpdateDolbyInfo(AudioDecoder_T *ptAudioDecoder, AudioDecoder_DDP_DrcParams_T *ptDrcParams)
{
	return AudioDecoder_ErrorNotSupportedCodec;
}
*/
