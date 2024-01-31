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

#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <stdint.h>
#include <stdbool.h>


/* decoding result flags */
#define DECODING_SUCCESS_FRAME		(0x0001)
#define DECODING_SUCCESS_FIELD		(0x0002)
#define DECODING_SUCCESS			(0x0003)
#define DECODING_SUCCESS_SKIPPED	(0x0004)
#define DISPLAY_OUTPUT_SUCCESS		(0x0010)
#define DECODED_OUTPUT_SUCCESS		(0x0020)
#define OUTPUT_SUCCESS				(0x0030)
#define DECODING_BUFFER_FULL		(0x0100)
#define RESOLUTION_CHANGED			(0x1000)

#define DEFAULT_OUTPUT_WIDTH_MAX	(1920)
#define DEFAULT_OUTPUT_HEIGHT_MAX	(1088)
#define DEFAULT_OUTPUT_FRAME_RATE   (30000)
#define DEFAULT_FRAME_BUF_CNT		(6)

typedef enum
{
	VideoDecoder_Addr_PA = 0,
	VideoDecoder_Addr_VA,
	VideoDecoder_Addr_K_VA,
	VideoDecoder_Addr_MAX
} VideoDecoder_Addr_E;

typedef enum
{
	VideoDecoder_Comp_Y = 0,
	VideoDecoder_Comp_U,
	VideoDecoder_Comp_V,
	VideoDecoder_Comp_MAX
} VideoDecoder_Comp_E;

typedef enum
{
	VideoDecoder_Codec_None,
	VideoDecoder_Codec_MPEG2,
	VideoDecoder_Codec_MPEG4,
	VideoDecoder_Codec_H263,
	VideoDecoder_Codec_H264,
	VideoDecoder_Codec_H265,
	VideoDecoder_Codec_VP9,
	VideoDecoder_Codec_VP8,
	VideoDecoder_Codec_MJPEG,
	VideoDecoder_Codec_WMV,
	VideoDecoder_Codec_Max
} VideoDecoder_Codec_E;

typedef enum
{
	VideoDecoder_ErrorNone,
	VideoDecoder_ErrorBadParameter,
	VideoDecoder_ErrorInitFail,
	VideoDecoder_ErrorDecodingFail,
	VideoDecoder_ErrorGetOutputFrame,
	VideoDecoder_ErrorNotSupportedCodec,
	VideoDecoder_ErrorSequenceHeaderDecoding,
	VideoDecoder_ErrorSetVPUClock,
	VideoDecoder_ErrorUnrecoverable
} VideoDecoder_Error_E;

typedef enum
{
	VideoDecoder_TS_None,
	VideoDecoder_TS_DisplayOrder,  /** Output the display order of the timestamps of frames in ascending order */
	VideoDecoder_TS_DecodingOrder, /** Output the decoding order of the timestamps of frames in ascending order */
	VideoDecoder_TS_Generate,      /** If no timestamps exist, generate timestamps according to a framerate */
} VideoDecoder_TS_Mode_E;

typedef enum
{
	VideoDecoder_Reorder_None,
	VideoDecoder_Reorder_Profile,     /** Reordering works according to video profile */
	VideoDecoder_Reorder_Low_Latency, /** Output immediately after decoding. The VPU assumes that the input stream has no reordering. */
} VideoDecoder_Reorder_Mode_E;

typedef enum
{
	VideoDecoder_FrameBuffer_CompressMode_None,         /** Do not compress frame buffer */
	VideoDecoder_FrameBuffer_CompressMode_MapConverter, /** Compress using Map Converter */
} VideoDecoder_FrameBuffer_CompressMode_E;

typedef struct VideoDecoder_T	VideoDecoder_T;


typedef struct
{
	unsigned long ulResult;

	int iIntanceIdx;
	bool bIsFrameOut;
	bool bIsInsertClock;
	bool bIsInsertFrame;

	int iDecodingStatus;
	int iOutputStatus;
	int iDecodedIdx;
	int iDispOutIdx;

	bool bField;
	bool bInterlaced;
	bool bOddFieldFirst;

	float fAspectRatio;

	unsigned int uiWidth;
	unsigned int uiHeight;

	unsigned int uiCropLeft;
	unsigned int uiCropRight;
	unsigned int uiCropTop;
	unsigned int uiCropBottom;
	unsigned int uiCropWidth;
	unsigned int uiCropHeight;

	unsigned char *pucDispOut[VideoDecoder_Addr_MAX][VideoDecoder_Comp_MAX];
} VideoDecoder_DecodingResult_T;

typedef struct
{
	unsigned int uiTop;
	unsigned int uiLeft;
	unsigned int uiHeight;
	unsigned int uiWidth;
} VideoDecoder_Rect_T;

typedef struct
{
	unsigned char *pucData;
	unsigned int uiLen;
} VideoDecoder_UserData_T;

typedef struct
{
	unsigned char *pucData;
	unsigned int uiLen;
} VideoDecoder_MapConvData_T;

typedef struct
{
	VideoDecoder_MapConvData_T tMapConvData;
} VideoDecoder_CompresedData_T;

typedef struct
{
	unsigned char *pucData;
	unsigned int uiLen;
} VideoDecoder_HdrMetaData_T;

typedef struct
{
	unsigned long ulResult;
	uint64_t uiID;
	int64_t iTimeStamp;
	unsigned char *pucPhyAddr[VideoDecoder_Comp_MAX];
	unsigned char *pucVirAddr[VideoDecoder_Comp_MAX];

	int iDisplayIndex;
	bool bInterlaced;
	bool bOddFieldFirst;
	float fAspectRatio;
	unsigned int uiWidth;
	unsigned int uiHeight;
	VideoDecoder_Rect_T tCropRect;
	VideoDecoder_UserData_T tUserData;
	VideoDecoder_CompresedData_T tCompresedData;
	VideoDecoder_HdrMetaData_T tHdrMetaData;
} VideoDecoder_Frame_T;

typedef struct
{
	unsigned char *pucStream; 		/** Buffer address contains codec config data */
	unsigned int uiStreamLen; 		/** The length of pucStream */
	unsigned int uiNumOutputFrames; /** [Deprecated] Number of output frames. Application can hold decoded output frames for buffering, depecated */
} VideoDecoder_SequenceHeaderInputParam_T;

/**
 * This structure holds configurations for generate timestamp mode */
typedef struct
{
	unsigned int uiFramerate;     /** Framerate. ex) 30, 60 */
	int64_t iInitialTimestamp;    /** Timestamp of first video frame, in miliseconds
									* If value is uiFramerate=30, iInitialTimestamp=1000 then timestamps output 1000, 1033, 1067 ..*/
} VideoDecoder_TS_GenModeConfig_T;

/**
 * This structure holds timestamp mode. */
typedef struct
{
	VideoDecoder_TS_Mode_E eMode;
	VideoDecoder_TS_GenModeConfig_T tGenModeConfig; /** Valid if mode is generate*/
} VideoDecoder_TimestampMode_T;

/**
 * This structure holds re-ordering mode */
typedef struct
{
	VideoDecoder_Reorder_Mode_E eMode;
} VideoDecoder_ReorderingMode_T;

/**
 * This structure holds secure mode */
typedef struct
{
	bool bEnable;
} VideoDecoder_SecureMode_T;

/**
 * This structure holds framebuffer compress mode */
typedef struct
{
	VideoDecoder_FrameBuffer_CompressMode_E eMode;
} VideoDecoder_FrameBuffer_CompressMode_T;

/**
 * This structure holds userdata mode */
typedef struct
{
	bool bEnable;
} VideoDecoder_UserdataMode_T;

/**
 * This structure holds one field mode */
typedef struct
{
	bool bEnable;
} VideoDecoder_OneFieldMode_T;

/**
 * This structure holds configurations for VideoDecoder_Init() function */
typedef struct
{
	VideoDecoder_Codec_E eCodec;

	/** Sets the maximum resolution to decode.
	  * In case of adaptive streaming, the resolution is dynamically changed during single stream playback.
	  * VPU supports seamless resolution change named max frame buffer mode. maximum resolution should be set and it affects the size of video memory.
	  * If not set, default value is applied. */
	unsigned int uiMax_resolution_width;
	unsigned int uiMax_resolution_height;

	/** The number of surface frames. video decoder maintains decoded frame buffers.
	  * Keeps buffering the reference frames and surface frames to prevent overwritting buffers while the user is using it.
	  * Increasing the number frames improves pipelining, but increases the size of video memory. */
	unsigned int number_of_surface_frames;

	/** The decoder may select a method of outputting the timestamp of the video frame according to the purpose.
	  * By default, the decoding order is sorted and outputted in the display order.
	  * For the case where the user directly manages the output buffer, it outputs in decoding order.
	  * If the input stream does not have a timestamp, the decoder generates a timestamp.*/
	VideoDecoder_TimestampMode_T tTsMode;

	/** Decide the policy of frame reordering. This affects the output latency.
	  * Determines how many input frames it takes before a frame is output. */
	VideoDecoder_ReorderingMode_T tReorderMode;

	/** If secure video path is enabled, video decorder protects input/output and internal
	  * buffers and do not allow to access by the CPU. */
	VideoDecoder_SecureMode_T tSecureMode;

	/** Frame buffer Compression mode.
	  * To satisfy real-time display performance of 4K or higher resolution, video decoder compress
	  * frame buffer using the compressor like Map converter used for HEVC codec.
	  * video decoder provides additional data for Compressed data.
	  * We need to pass this data to the video output driver. */
	VideoDecoder_FrameBuffer_CompressMode_T tCompressMode;

	/** Bit depth.
	 * 0: 8bit, 1: 10bit */
	unsigned int uiBitDepth;

	/** If userdata is enabled, video decoder outputs userdata except a header with ITU-T T.35 SEI message format
	 * If the input stream is encoded in HDR, HDR meta data can be extracted.*/
	VideoDecoder_UserdataMode_T tUserdataMode;

	/** Set if the input stream is interlaced and consists of only one field. ex) Top - Top - Top ... */
	VideoDecoder_OneFieldMode_T tOneFieldMode;
} VideoDecoder_InitConfig_T;

typedef struct
{
	int64_t iTimeStamp;
	unsigned char *pucStream;
	unsigned int uiStreamLen;
} VideoDecoder_StreamInfo_T;

VideoDecoder_Error_E VideoDecoder_DecodeSequenceHeader(const VideoDecoder_T *ptVideoDecoder, const VideoDecoder_SequenceHeaderInputParam_T *ptParam);

VideoDecoder_Error_E VideoDecoder_Decode(VideoDecoder_T *ptVideoDecoder, const VideoDecoder_StreamInfo_T *ptStream, VideoDecoder_Frame_T *ptFrame);

VideoDecoder_Error_E VideoDecoder_Flush(const VideoDecoder_T *ptVideoDecoder);

VideoDecoder_Error_E VideoDecoder_Init(VideoDecoder_T *ptVideoDecoder, const VideoDecoder_InitConfig_T *ptConfig);
VideoDecoder_Error_E VideoDecoder_Deinit(const VideoDecoder_T *ptVideoDecoder);

VideoDecoder_T *VideoDecoder_Create(void);
VideoDecoder_Error_E VideoDecoder_Destroy(VideoDecoder_T *ptVideoDecoder);

unsigned int VideoDecoder_GetCbCrInterleaveMode(const VideoDecoder_T *ptVideoDecoder);

#endif	// VIDEO_DECODER_H