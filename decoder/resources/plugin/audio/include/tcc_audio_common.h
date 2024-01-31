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

#ifndef TCC_AUDIO_COMMON__
#define TCC_AUDIO_COMMON__


#ifdef __cplusplus
extern "C" {
#endif

#include "adec.h"

#define AUDIO_ID_AAC 0
#define AUDIO_ID_MP3 1
#define AUDIO_ID_AMR 2
#define AUDIO_ID_AC3 3

//#define AUDIO_ID_PCM 4	//--> WAV
#define AUDIO_ID_MP3HD 4
#define AUDIO_ID_MP2 5
#define AUDIO_ID_DTS 6
#define AUDIO_ID_QCELP 7
#define AUDIO_ID_AMRWBP 8
#define AUDIO_ID_WMA 9
#define AUDIO_ID_EVRC 10
#define AUDIO_ID_FLAC 11
#define AUDIO_ID_COOK 12
#define AUDIO_ID_G722 13
#define AUDIO_ID_G729 14
#define AUDIO_ID_APE 15
//#define AUDIO_ID_MSADPCM 16	//--> WAV
#define AUDIO_ID_WAV 16
//#define AUDIO_ID_IMAADPCM 17	//--> WAV
#define AUDIO_ID_DRA 17
#define AUDIO_ID_VORBIS 18
#define AUDIO_ID_BSAC 19

#define AUDIO_ID_MP1  20
#define AUDIO_ID_DDP  21
#define AUDIO_ID_TRUEHD  23
#define AUDIO_ID_DTSHD   1006 // SSG
#define AUDIO_ID_AAC_GOOGLE	24
#define AUDIO_ID_OPUS 25

#define AUDIO_SIZE_MAX ( (4608 * 2) *8*2)
#define AUDIO_MAX_INPUT_SIZE	(1024*96)

#define AUDIO_FILENAME_LEN 			512

#define AUDIO_PCM_OUTPUT_MAX_LENGTH 	1024*200
#define AUDIO_PCM_OUTPUT_LEFT_LENGTH 	1024*50
#define AUDIO_PCM_OUTPUT_RIGHT_LENGTH 	1024*50
#define AUDIO_PARSER_WORKBUF_LENGTH		1024*100

#define AUDIO_ERROR_DETECT_COUNT 5

#define MAX_COMPONENT_AUDIO_PARSER	1

#define CODEC_SEEK_FF                       0x00000000
#define CODEC_SEEK_REW                      0x00000001
#define CODEC_SEEK_OTHER                    0x00000002


/*Audio Parser Name Definition*/
#define AUDIO_CDK_DMX_NAME		"OMX.tcc.demuxer.cdk_audio"
#define AUDIO_CDK_DEC_NAME 		"OMX.tcc.audio_decoder.cdk"
#define AUDIO_DEQ_NAME 			"OMX.tcc.deq.component"

#define TCC_AUDIO_EXIFPARSE_INCLUDE

#define AUDIO_SEEK_X1_MODE	1
#define AUDIO_SEEK_X2_MODE	2
#define AUDIO_SEEK_X4_MODE  4
#define AUDIO_SEEK_X8_MODE  8
#define AUDIO_SEEK_X16_MODE 16

#define AUDIO_SEEK_DIRECTION_REW	0
#define AUDIO_SEEK_DIRECTION_FF		1

#define AUDIO_DYNAMIC_SETP_OFF	0
#define AUDIO_DYNAMIC_SETP1		1
#define AUDIO_DYNAMIC_SETP2		2
#define AUDIO_DYNAMIC_SETP3		3
#define AUDIO_DYNAMIC_SETP4		4
#define AUDIO_DYNAMIC_SETP5		5
#define AUDIO_DYNAMIC_SETP6		6
#define AUDIO_DYNAMIC_SETP7		7
#define AUDIO_DYNAMIC_SETP8		8

#define AUDIO_CH_LR_MODE		0
#define AUDIO_CH_LL_MODE		1
#define AUDIO_CH_RR_MODE		2

#define  SCERR_NO									0
#define  SCERR_NO_PROC_DEVICE                      -1
#define  SCERR_DRIVER_NO_FIND                      -2
#define  SCERR_NO_SOUNDCARD                        -3
#define  SCERR_NO_MIXER                            -4
#define  SCERR_OPEN_SOUNDCARD                      -5
#define  SCERR_OPEN_MIXER                          -6
#define  SCERR_PRIVILEGE_SOUNDCARD                 -7
#define  SCERR_PRIVILEGE_MIXER                     -8

#define  SCERR_NO_FILE                             -10
#define  SCERR_NOT_OPEN                            -11
#define  SCERR_NOT_WAV_FILE                        -12
#define  SCERR_NO_wav_info                         -13

#define MAX_FILENAME_SIZE		256
#define TRUE                 1
#define FALSE                0

typedef int fnCdkAudioDec (int iOpCode, unsigned long* pHandle, void* pParam1, void* pParam2 );

typedef enum
{
	AUDIO_NORMAL_MODE		= 0x0002,	// normal playback mode (ex: file-play)
	AUDIO_BROADCAST_MODE	= 0x0004,	// broadcasting mode (ex: dab, dmb)
	AUDIO_DDP_TO_DD_MODE	= 0x0008,	// ddp to dd converting mode
}TCC_AUDIO_PROCESS_MODE;

typedef struct decoder_func_list_t
{
	fnCdkAudioDec  *pfMainFunction;
	void *pfCodecSpecific[8];
} decoder_func_list_t;

typedef enum Audio_Enc_Codec_Type
{
	AUDIO_ENC_CODEC_NULL =0,
	AUDIO_ENC_CODEC_MP3 ,
	AUDIO_ENC_CODEC_PCM,
	AUDIO_ENC_CODEC_ADPCM,
	AUDIO_ENC_CODEC_WMA,
	AUDIO_ENC_CODEC_AAC,
	AUDIO_ENC_CODEC_NUM
}Audio_Enc_Codec_Type ;

#define MP3_DATA_SIZE 1500
#define MP2_DATA_SIZE 2048
#define APE_DATA_SIZE (7*1024)*2

#ifdef __cplusplus
}
#endif

#endif /*TCC_AUDIO_COMMON__*/

