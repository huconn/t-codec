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

#ifndef AUDIO_DECODER_H__
#define AUDIO_DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_BUFFER_SIZE	(1024 * 1024)

typedef enum
{
	AudioDecoder_ErrorNone,
	AudioDecoder_ErrorNoneRemainInputStream,
	AudioDecoder_ErrorBadParameter,
	AudioDecoder_ErrorInsufficientResources,
	AudioDecoder_ErrorInitFail,
	AudioDecoder_ErrorDecodingFail,
	AudioDecoder_ErrorNotSupportedCodec,
	AudioDecoder_ErrorNotReady
} AudioDecoder_Error_E;

typedef enum
{
	AudioDecoder_Codec_None,
	AudioDecoder_Codec_AAC,
	AudioDecoder_Codec_MP3,
	AudioDecoder_Codec_PCM,
	AudioDecoder_Codec_DDP,
} AudioDecoder_Codec_E;

typedef enum
{
	AudioDecoder_AAC_Profile_Main,
	AudioDecoder_AAC_Profile_LTP,
	AudioDecoder_AAC_Profile_LC,
	AudioDecoder_AAC_Profile_HE,
	AudioDecoder_AAC_Profile_HE_PS
} AudioDecoder_AAC_Profile_E;

typedef enum
{
	AudioDecoder_AAC_StreamFormat_MP2ADTS,
	AudioDecoder_AAC_StreamFormat_ADTS,
	AudioDecoder_AAC_StreamFormat_MP4LOAS,
	AudioDecoder_AAC_StreamFormat_MP4LATM,
	AudioDecoder_AAC_StreamFormat_ADIF,
	AudioDecoder_AAC_StreamFormat_MP4FF,
	AudioDecoder_AAC_StreamFormat_RAW
} AudioDecoder_AAC_StreamFormat_E;

/**
 * This enumeration defines Dynamic range control mode to be used for PCM outputs. */
typedef enum
{
	AudioDecoder_DDP_DRC_MODE_CUSTOM_ANALOG = 0,  /**< Custom mode, analog dialnorm */
	AudioDecoder_DDP_DRC_MODE_CUSTOM_DIGITAL,     /**< Custom mode, digital dialnorm */
	AudioDecoder_DDP_DRC_MODE_LINE,               /**< Line mode */
	AudioDecoder_DDP_DRC_MODE_RF,                 /**< RF mode */
} AudioDecoder_DDP_DrcMode_E;

/**
 * Structure to hold Dynamic range control parameters. */
typedef struct
{
	AudioDecoder_DDP_DrcMode_E eDrcMode;  /**< DRC Mode, default is DRC_MODE_LINE */
	unsigned int  drc_high_cut;  /**< An integer between 0 and 100 for compression cut scale factor.
									A value of 0 is no cut and a value of 100 is the maximum factor. default is 100*/
	unsigned int  low_boost;     /*<< An integer between 0 and 100 for compression boost scale factor.
									A value of 0 is no boost and a value of 100 is the maximum factor. default is 100 */
	bool enable;                 /**< True to enable or false to disable drcMode */
} AudioDecoder_DDP_DrcParams_T;

/**
 * This enumeration defines Stereo mode */
typedef enum
{
	AudioDecoder_DDP_STEREO_AUTO = 0,  /**< Auto mode, reads the stereo downmix mode parameter
							              from the Dolby stream and sets the downmix mode accordingly. */
	AudioDecoder_DDP_STEREO_LTRT,      /**< Lt/Rt downmix */
	AudioDecoder_DDP_STEREO_LORO,      /**< Lo/Ro downmix */
} AudioDecoder_DDP_StereoMode_E;

typedef struct
{
	unsigned int uiChannels;
	unsigned int uiBitsPerSample;
	unsigned int uiSampleRate;

	struct
	{
		AudioDecoder_AAC_Profile_E eProfile;
		AudioDecoder_AAC_StreamFormat_E eStreamFormat;
	} aac;

	struct
	{
		int iLayer;
		int iDABMode;
	} mp3;

	struct
	{
		int     iWAVForm_TagID;
		unsigned int    uiNBlockAlign;
		int     iEndian;
		int     iContainerType;
	} pcm;

	struct
	{
		bool is_ddp;     /**< is dolby digital plus HD codec */
		bool lfe;        /**< LFE channels to output(if available in bitstream), default is 0,
							  0 : No LFE channels output, 1 : LFE channels output  */
		AudioDecoder_DDP_StereoMode_E eStereoMode;    /**< Stereo downmixing mode, default is 0 */
		AudioDecoder_DDP_DrcParams_T tDrcParams;     /**< Dynamic range control parameters */
	} ddp;

	void *codecdata;
	size_t codecdataLen;
} AudioDecoder_CodecInfo_T;

typedef struct
{
	int64_t iTimeStamp;
	unsigned char *pucStream;
	unsigned int uiStreamLen;
} AudioDecoder_StreamInfo_T;

typedef struct
{
	struct
	{
		unsigned char ucOutputBuffer[STREAM_BUFFER_SIZE];
		unsigned int uiBufferLen;
	} stereo;

	struct
	{
		unsigned char ucOutputBuffer[STREAM_BUFFER_SIZE];
		unsigned int uiBufferLen;
	} multi;

	struct
	{
		void *pOutputbuffer;
		unsigned int uiOutputbufferLen;
	} dd;

	unsigned int uiChannels;
	unsigned int uiBitsPerSample;
	unsigned int uiSampleRate;
} AudioDecoder_Pcm_T;

typedef struct AudioDecoder_T AudioDecoder_T;

/**
 * This function decodes incoming audio elementary stream data.
 *
 * \param[in] ptAudioDecoder
 *             Identifier of the Audio decoder handle.
 * \param[in] ptStream
 *             A input stream descriptor containing input buffer.
 * \param[out] ptPcm
 *             A output decoded PCM descriptor that provides multi outputs containing stereo, multi, dolby digital.
 *
 * \retval AudioDecoder_ErrorNone Success.
 * \retval AudioDecoder_ErrorInsufficientResources Need more data to decode.
 *         Previous input stream data is copied so there is no longer needed.
 * \retval AudioDecoder_ErrorNoneRemainInputStream Success but There is input stream left for decoding
 *         pucStream, uiStreamLen will be updated aftter decoding. Just call again this function.
 */

AudioDecoder_Error_E AudioDecoder_Decode(
	AudioDecoder_T *ptAudioDecoder,
	const AudioDecoder_StreamInfo_T *ptStream,
	AudioDecoder_Pcm_T *ptPcm);

//Currently Unused code
//AudioDecoder_Error_E AudioDecoder_Flush(AudioDecoder_T *ptAudioDecoder);

AudioDecoder_Error_E AudioDecoder_Init(
	AudioDecoder_T *ptAudioDecoder,
	AudioDecoder_Codec_E eCodec,
	const AudioDecoder_CodecInfo_T *ptCodecInfo);

AudioDecoder_Error_E AudioDecoder_Deinit(const AudioDecoder_T *ptAudioDecoder);

AudioDecoder_T *AudioDecoder_Create(void);
AudioDecoder_Error_E AudioDecoder_Destroy(AudioDecoder_T *ptAudioDecoder);

/**
 * This function sets Dolby audio decoder configuration to change dynamic range control.
 * The setting value will be applied from the next frame.
 *
 * It is only valid when AudioDecoder_Init() has been called previously.
 *     Otherwise, this function will return AudioDecoder_ErrorNotReady.
 */
//TBD
//AudioDecoder_Error_E AudioDecoder_UpdateDolbyInfo(AudioDecoder_T *ptAudioDecoder, AudioDecoder_DDP_DrcParams_T *ptDrcParams);


#ifdef __cplusplus
}
#endif

#endif	// AUDIO_DECODER_H__
