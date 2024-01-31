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

#include "vpu_test_common.h"
#include "vpu_test_decoder.h"

#include "video_decoder.h"

#define DLOGD(...) 		if(decoder_log_level > 2) { DLOG(__VA_ARGS__); }	//debug
#define DLOGI(...) 		if(decoder_log_level > 1) { DLOG(__VA_ARGS__); }	//info
#define DLOGE(...) 		if(decoder_log_level > 0) { DLOG(__VA_ARGS__); }	//error

#define MAX_RESOLUTION_WIDTH	(3840)
#define MAX_RESOLUTION_HEIGHT	(2160)
#define SUPPORT_OUTPUT_FILE_WRITE

#define READ_BUFFER_SIZE    	((1024) * (1024))
#define MAX_PATH_SIZE	(1024)

typedef struct DecoderConfig_t
{
    int width;
    int height;

    char filename[MAX_PATH_SIZE];

    VideoDecoder_Codec_E codec;

    //VPU_CODEC_BufferType_E eOutputBufferMode;
    int writeOutputYuv;
    int logLevel;
	int numDecIterations;

    int maxFrameBuffer;
    int stopCount;
    int mapConvOption; //0: mapConvOption off, 1: mapConvOption on, 2: mapConvOption on and wtl on
} DecoderConfig_t;

typedef struct DecoderContext_t
{
	VideoDecoder_T* video_decoder;
	VideoDecoder_InitConfig_T init_config;

    int dec_id;
	int found_seqheader;

    int isRun;
    pthread_t hThread;

    DecoderConfig_t tConfig;
} DecoderContext_t;

static int decoder_log_level = 2; //log

static const char* strCodec[] =
{
	"None",
    "H.264",
    "VC1",
    "MPEG2",
    "MPEG4",
    "H.263",
	"Divx",
	"AVS",
	"MJPG",
	"VP8",
	"MVC",
    "HEVC",
    "VP9",
};

static int parseDecoderCommand(DecoderContext_t* pctx, char* parameters)
{
    DecoderConfig_t* pconfig = &pctx->tConfig;

    char* token = strtok(parameters, " ");
	while (token != NULL)
	{
		DLOGI("next token:%s", token);

		if (strncmp(token, "-d", 2) == 0)
		{
			char* filename = strtok(NULL, " ");
            char* width = strtok(NULL, " ");
            char* height = strtok(NULL, " ");

            strcpy(pconfig->filename, filename);
            pconfig->width = atoi(width);
            pconfig->height = atoi(height);

            DLOGI("Decoder input, Filename: %s, Width: %d, Height: %d", pconfig->filename, pconfig->width, pconfig->height);
        }
        else if (strncmp(token, "-v", 2) == 0)
		{
            char* codec = strtok(NULL, " ");

			if((strcmp(codec, "h264") == 0) || (strcmp(codec, "avc") == 0))
			{
                pconfig->codec = VideoDecoder_Codec_H264;
            }
            else if(strcmp(codec, "mpeg4") == 0)
			{
                pconfig->codec = VideoDecoder_Codec_MPEG4;
            }
            else if(strcmp(codec, "h263") == 0)
            {
                pconfig->codec = VideoDecoder_Codec_H263;
            }
            else if((strcmp(codec, "hevc") == 0) || (strcmp(codec, "h265") == 0))
			{
                pconfig->codec = VideoDecoder_Codec_H265;
            }
            else if(strcmp(codec, "vp9") == 0)
            {
                pconfig->codec = VideoDecoder_Codec_VP9;
            }
            else if((strcmp(codec, "mjpg") == 0) || (strcmp(codec, "mjpeg") == 0))
			{
                pconfig->codec = VideoDecoder_Codec_MJPEG;
            }

			DLOGI("Video Codec: %s, eCodec:%d", codec, pconfig->codec);
        }
		else if (strncmp(token, "-n", 2) == 0)
        {
			char* nb_itor = strtok(NULL, " ");

            pconfig->numDecIterations = atoi(nb_itor);
            DLOGI("numDecIterations:%d", pconfig->numDecIterations);
        }
		else if (strncmp(token, "-output", 8) == 0)
        {
            pconfig->writeOutputYuv = 1;
            DLOGI("writeOutputYuv:%d", pconfig->writeOutputYuv);
        }
		else if (strncmp(token, "-log", 4) == 0)
		{
            char* loglevel = strtok(NULL, " ");

            pconfig->logLevel = atoi(loglevel);
			DLOGI("logLevel: %d", pconfig->logLevel);
			decoder_log_level = pconfig->logLevel;
        }
		else if (strncmp(token, "-maxfb", 6) == 0)
        {
			char* maxfb = strtok(NULL, " ");

            pconfig->maxFrameBuffer = atoi(maxfb);
            DLOGI("maxFrameBuffer:%d", pconfig->maxFrameBuffer);
        }
		else if (strncmp(token, "-stopCount", 10) == 0)
        {
            char* stopcount = strtok(NULL, " ");

            pconfig->stopCount = atoi(stopcount);
            DLOGI("stopCount:%d", pconfig->stopCount)
        }
		else if (strncmp(token, "-mapConv", 8) == 0)
        {
			char* mapconv = strtok(NULL, " ");

            pconfig->mapConvOption = atoi(mapconv);
            DLOGI("mapConvOption:%d", pconfig->mapConvOption);
        }

    	token = strtok(NULL, " ");
    }

    return 0;
}

#define SCAN_MAX_SIZE 1024 /** THe sequnce header is positioned in 1024 bytes */
static int ScanSequenceHeader(unsigned char *pPayloadBuffer, long lStreamLength, VideoDecoder_Codec_E codec)
{
    unsigned char *p_buff = pPayloadBuffer;
    unsigned char *p_buff_start = p_buff;
    unsigned char *p_buff_end = p_buff + (lStreamLength > SCAN_MAX_SIZE ? SCAN_MAX_SIZE : lStreamLength) - 4;
    unsigned char *p_buff_data_end = p_buff + lStreamLength - 4;
    unsigned int syncword = 0xFFFFFFFF;
    unsigned int nal_type = 0;

    syncword = ((unsigned int)p_buff[0] << 16) | ((unsigned int)p_buff[1] << 8) | (unsigned int)p_buff[2];

    if(codec == VideoDecoder_Codec_H264)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001)
            {
                p_buff_end = p_buff_data_end;

                if((syncword & 0x1F) == 7) // nal_type: SPS
                    return (int)(p_buff - p_buff_start);
            }
            p_buff++;
        }
    }
    else if(codec == VideoDecoder_Codec_H265)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001)
            {
                p_buff_end = p_buff_data_end;

                nal_type = (syncword >> 1) & 0x3F;

                if(nal_type == 32 || nal_type == 33) // nal_type: VPS/SPS
                    return (int)(p_buff - p_buff_start);
            }
            p_buff++;
        }
    }
    else if(codec == VideoDecoder_Codec_MPEG4)
    {
        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if((syncword >> 8) == 0x000001 )
            {
                p_buff_end = p_buff_data_end;

                if(syncword >= 0x00000120 && syncword <= 0x0000012F) // video_object_layer_start_code
                    return (int)(p_buff - p_buff_start);
            }


            p_buff++;
        }
    }
    else
    {
        unsigned int seqhead_sig = 0;

        switch(codec)
        {
        //case VideoDecoder_Codec_H264:
        //    seqhead_sig = 0x0000010F; //VC-1 sequence start code
        //    break;
        case VideoDecoder_Codec_MPEG2:
            seqhead_sig = 0x000001B3; //MPEG video sequence start code
            break;
        case VideoDecoder_Codec_VP9:
            return 0;
        default:
            DLOGE("Stream type is not supported (type: %ld)", codec);
            return 0;
        }

        while(p_buff < p_buff_end)
        {
            syncword <<= 8;
            syncword |= p_buff[3];

            if(syncword == seqhead_sig)
                return (int)(p_buff - p_buff_start);

            p_buff++;
        }
    }

    return -1;
}

static int parse_video_frame(DecoderContext_t *pctx, VideoDecoder_Codec_E codec, FILE* hInputFile, unsigned char* readBuffer)
{

	int iReadByte = 0;
	int iDataSize = 0;

	do
	{
		if ((iReadByte = fread(readBuffer + iDataSize, 1, 1, hInputFile)) <= 0)
		{
			if (iReadByte == 0)
			{
				// DLOGD("End of file!");
			}
			else
			{
				DLOGD("read error!");
			}
			goto parse_video_fail;
		}

		iDataSize += iReadByte;

		if (iDataSize > 8)
		{
			if (codec == VideoDecoder_Codec_H263)
			{
				if (readBuffer[iDataSize - 3] == 0x00 && readBuffer[iDataSize - 2] == 0x00 && (readBuffer[iDataSize - 1] >> 4) == 0x08) // H263 start code (22 bit)
				{
					if (pctx->found_seqheader == 1)
					{
						fseek(hInputFile, -3, SEEK_CUR);

						iDataSize -= 3;
						break;
					}
					else
					{
						pctx->found_seqheader = 1;
					}
				}
			}
			else  if (codec == VideoDecoder_Codec_H264)
			{
				if(readBuffer[iDataSize - 5] == 0x00 &&  readBuffer[iDataSize - 4] == 0x00 && readBuffer[iDataSize - 3] == 0x00
					&& readBuffer[iDataSize - 2] == 0x01 /* && readBuffer[iDataSize - 1] == 0x09 */) // AUD start code
				{
					if (pctx->found_seqheader == 1)
					{
						fseek(hInputFile, -5, SEEK_CUR);

						iDataSize -= 5;
						break;
					}
					else
					{
						pctx->found_seqheader = 1;
					}
				}
			}
			else  if (codec == VideoDecoder_Codec_H265)
			{
				if(readBuffer[iDataSize - 5] == 0x00 &&  readBuffer[iDataSize - 4] == 0x00 && readBuffer[iDataSize - 3] == 0x00
					&& readBuffer[iDataSize - 2] == 0x01 /* && readBuffer[iDataSize - 1] == 0x46 */ )
				{
					if (pctx->found_seqheader == 1)
					{
						fseek(hInputFile, -5, SEEK_CUR);

						iDataSize -= 5;
						break;
					}
					else
					{
						pctx->found_seqheader = 1;
					}
				}
			}
			else  if (codec == VideoDecoder_Codec_MPEG4)
			{
			if(readBuffer[iDataSize - 4] == 0x00 && readBuffer[iDataSize - 3] == 0x00
				&& readBuffer[iDataSize - 2] == 0x01 && readBuffer[iDataSize - 1] == 0xB6) // MPEG-4 start code
				{
					if (pctx->found_seqheader == 1)
					{
						fseek(hInputFile, -4, SEEK_CUR);

						iDataSize -= 4;
						break;
					}
					else
					{
						pctx->found_seqheader = 1;
					}
				}
			}
			else  if (codec == VideoDecoder_Codec_MJPEG)
			{
				if (readBuffer[iDataSize - 3] == 0xFF && readBuffer[iDataSize - 2] == 0xD8 && readBuffer[iDataSize - 1] == 0xFF) // jpg start code
				{
					//DBG_ERR("find jpeg start code %x %x %x, size:%d", readBuffer[iDataSize - 3], readBuffer[iDataSize - 2], readBuffer[iDataSize - 1], iDataSize-3);
					fseek(hInputFile, -3, SEEK_CUR);
					iDataSize -= 3;
					break;
				}
			}
			else
			{
				DLOGE("Not supported yet, codec:%d", codec);
			}
		}
	} while (1);

	return iDataSize;

parse_video_fail:
	return 0;
}

static void* DecoderThread(void* private)
{
    int repeat_decoding = 0;
    DecoderContext_t *pctx = (DecoderContext_t *)private;
    DecoderConfig_t *pConfig = &pctx->tConfig;

	DLOGI("[Dec:%d] Decoding thread started", pctx->dec_id);

    do
    {
		VideoDecoder_SequenceHeaderInputParam_T seqheader_input;
		VideoDecoder_StreamInfo_T decode_intput;
		VideoDecoder_Frame_T decode_output;
		VideoDecoder_Error_E video_decoder_ret;

		int ret = 0;
        int framecount = 0;
        long long accumDecTime = 0;
        long long timestamp = 0;

        unsigned char *readBuffer = NULL;
        char strOutputFileName[1024];

        FILE *hInputFile = NULL;
        FILE *hOutputFile = NULL;

		int sequence_header_init = 0;
        int repeat_decoding = 0;
        int detect_seq_header = 0;

        hInputFile = fopen(pConfig->filename, "rb");
        if (hInputFile == NULL)
        {
            DLOGD("file open failed!! - %s\n", pConfig->filename);
            goto error_out;
        }

#ifdef SUPPORT_OUTPUT_FILE_WRITE
		DLOGI("writeOutputYuv:%d, numDecIterations:%d, mapConvOption:%d\n", pConfig->writeOutputYuv, pConfig->numDecIterations, pConfig->mapConvOption);
        if((pConfig->writeOutputYuv == 1) && (pConfig->numDecIterations == 0) && (pConfig->mapConvOption != 1))
        {
            switch (pConfig->codec)
            {
            case VideoDecoder_Codec_H264:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_h264.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            case VideoDecoder_Codec_MPEG4:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_mpeg4.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            case VideoDecoder_Codec_H263:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_h263.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            case VideoDecoder_Codec_H265:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_hevc.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            case VideoDecoder_Codec_VP9:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_vp9.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            case VideoDecoder_Codec_MJPEG:
                sprintf(strOutputFileName, "decode_output_%02d_%dx%d_mjpg.yuv", pctx->dec_id, pConfig->width, pConfig->height);
                break;
            }

            hOutputFile = fopen(strOutputFileName, "wb");
            if (hOutputFile == NULL)
            {
                DLOGE("file open failed!! - %s\n", strOutputFileName);
                goto error_out;
            }
            DLOGI("output file yuv - %s\n", strOutputFileName);
        }
#endif

		pctx->video_decoder = VideoDecoder_Create();
		if(pctx->video_decoder == NULL)
		{
			goto decoding_fail;
		}

		memset(&pctx->init_config, 0x00, sizeof(VideoDecoder_InitConfig_T));

		pctx->init_config.eCodec = pConfig->codec;

		if(pConfig->mapConvOption == 0)
		{
			pctx->init_config.tCompressMode.eMode = VideoDecoder_FrameBuffer_CompressMode_None;
		}
		else
		{
			pctx->init_config.tCompressMode.eMode = VideoDecoder_FrameBuffer_CompressMode_MapConverter;
		}

		if(pConfig->maxFrameBuffer == 1)
		{
        	pctx->init_config.uiMax_resolution_width = MAX_RESOLUTION_WIDTH;
			pctx->init_config.uiMax_resolution_height = MAX_RESOLUTION_HEIGHT;
		}

		pctx->init_config.number_of_surface_frames = 2;

		video_decoder_ret = VideoDecoder_Init(pctx->video_decoder, &pctx->init_config);
		if(video_decoder_ret != VideoDecoder_ErrorNone)
        {
            DLOGE("VPU_DEC_Init() error!!\n");
            pConfig->numDecIterations = repeat_decoding;
            goto decoding_fail;
        }

		detect_seq_header = READ_BUFFER_SIZE;
		readBuffer = malloc(READ_BUFFER_SIZE);
        if (readBuffer == NULL)
        {
            DLOGE("readBuffer allocation failed");
            goto error_out;
        }

        while (pctx->isRun)
        {
            long long dec_start;
            long long dec_finish;

            int iReadByte = 0;
            int iDataSize = 0;
            int iFrameSize;
            int iBufferSize = 0;

            if ((pConfig->stopCount > 0) && (framecount >= pConfig->stopCount))
            {
                break;
            }

            memset(readBuffer, 0x00, READ_BUFFER_SIZE);

            iBufferSize = parse_video_frame(pctx, pConfig->codec, hInputFile, readBuffer);

            if (detect_seq_header == 0)
            {
                if (ScanSequenceHeader(readBuffer, iBufferSize, pConfig->codec) < 0)
                {
                    DLOGE("Not Found sequence header");
                    continue;
                }
                DLOGI("Found sequence header");
                detect_seq_header = 1;
            }

			timestamp += 33;
			if (((timestamp + 1) % 100) == 0)
			{
				timestamp += 1;
			}

            DLOGD("[Dec:%d][Input][C:%4d][T:%lld][S:%5d][0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]",
                pctx->dec_id, framecount, timestamp, iBufferSize,
                readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3],
                readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7],
				readBuffer[8], readBuffer[9], readBuffer[10], readBuffer[11]);

            dec_start = TCC_GetTimeUs();

			if(sequence_header_init == 0)
			{
				memset(&seqheader_input, 0x00, sizeof(VideoDecoder_SequenceHeaderInputParam_T));

				seqheader_input.pucStream = readBuffer;
				seqheader_input.uiStreamLen = iBufferSize;
				seqheader_input.uiNumOutputFrames = 4;

				DLOGI("VideoDecoder_DecodeSequenceHeader, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", iBufferSize, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
				video_decoder_ret = VideoDecoder_DecodeSequenceHeader(pctx->video_decoder, &seqheader_input);
				if(ret != VideoDecoder_ErrorNone)
				{
					DLOGE("VPU sequence header init error:%d", ret);
				}

				sequence_header_init = 1;
			}

			memset(&decode_intput, 0x00, sizeof(VideoDecoder_StreamInfo_T));
            memset(&decode_output, 0x00, sizeof(VideoDecoder_Frame_T));

			decode_intput.pucStream = readBuffer;
			decode_intput.uiStreamLen = iBufferSize;

			DLOGI("VideoDecoder_Decode, size:%d, %02X %02X %02X %02X %02X %02X %02X %02X", iBufferSize, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3], readBuffer[4], readBuffer[5], readBuffer[6], readBuffer[7]);
			video_decoder_ret = VideoDecoder_Decode(pctx->video_decoder, &decode_intput, &decode_output);
			if(ret != VideoDecoder_ErrorNone)
			{
				DLOGE("VPU_DEC_Decode() error!!");
				goto decoding_fail;
			}

            dec_finish = TCC_GetTimeUs();
            accumDecTime = dec_finish - dec_start;
            framecount++;

			if(decode_output.pucPhyAddr[VideoDecoder_Comp_Y] == 0)
			{
				DLOGD("No output");
                continue;
			}

#if 0//ndef DEBUG
            if ((framecount == 0) || ((framecount + 1) % 30 == 0)) // every 1 sec
            // if(framecount % 90 == 0) //every 3 sec
#endif
            /*
            {
                DLOGI("[Dec:%d][Repeat:%4d][C:%5d][T:%6lld][Type:%s][S:%6d][Res:%4dx%4d][Dstatus:%2d][Ostatus:%2d][DecodedIdx:%2d][DispFrameIdx:%2d], avg dec time:%7.2f us",
                              pctx->dec_id, repeat_decoding, framecount, timestamp,
                              strPicType[decode_output.vpu_decode_out.out_info.pic_type], iBufferSize,
                              vdec_decode_output.vpu_decode_out.out_info.display_width, vdec_decode_output.vpu_decode_out.out_info.display_height,
                               vdec_decode_output.vpu_decode_out.out_info.decoded_status, vdec_decode_output.vpu_decode_out.out_info.display_status,
                               vdec_decode_output.vpu_decode_out.out_info.decoded_idx,  vdec_decode_output.vpu_decode_out.out_info.display_idx,
                              ((float)accumDecTime / (float)(framecount + 1)));
            }
            */

		   {
                DLOGI("[Dec:%d][Repeat:%4d][C:%5d][T:%6lld][S:%6d][Res:%4dx%4d][DispFrameIdx:%2d], avg dec time:%7.2f us",
                              pctx->dec_id, repeat_decoding, framecount, timestamp, iBufferSize,
                              decode_output.uiWidth, decode_output.uiHeight,
                              decode_output.iDisplayIndex,
                              ((float)accumDecTime / (float)(framecount + 1)));
            }

            iFrameSize = decode_output.uiWidth * decode_output.uiHeight;

            if (hOutputFile != NULL)
            {
                fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_Y], 1, iFrameSize, hOutputFile);
                fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_U], 1, iFrameSize / 4, hOutputFile);
                fwrite(decode_output.pucVirAddr[VideoDecoder_Comp_V], 1, iFrameSize / 4, hOutputFile);
            }

			//vpu_buf_clear(pctx->video_decoder, decode_output.vpu_decode_out.out_info.display_idx);
            usleep(1);

        } // while(pctx->isRun)

decoding_fail:

		DLOGE("[Dec:%d][Repeat:%4d] End of file!! - VPU_DEC_Deinit", pctx->dec_id, repeat_decoding);
		if(pctx->video_decoder != NULL)
		{
			VideoDecoder_Deinit(pctx->video_decoder);
			VideoDecoder_Destroy(pctx->video_decoder);
		}

error_out:
        if (hInputFile != NULL)
        {
            fclose(hInputFile);
            hInputFile = NULL;
        }

        if (hOutputFile != NULL)
        {
            fclose(hOutputFile);
            hOutputFile = NULL;
        }

        if (readBuffer != NULL)
        {
            free(readBuffer);
            readBuffer = NULL;
        }

        DLOGI("[Dec:%d][Repeat:%4d] Decoding thread finished", pctx->dec_id, repeat_decoding);

    } while ((pctx->isRun == 1) && (repeat_decoding++ < pConfig->numDecIterations));

    return NULL;
}

vdec_test_h vdec_test_create(char* parameter)
{
	vdec_test_h handle = NULL;
	DecoderContext_t* dec_ctx = NULL;

	DLOGI("input parameter:%s", parameter);

	dec_ctx = malloc(sizeof(DecoderContext_t));
	if(dec_ctx != NULL)
	{
		(void)parseDecoderCommand(dec_ctx, parameter);
		dec_ctx->dec_id = 0;

	}

	return (vdec_test_h)dec_ctx;
}

//thread run
int vdec_test_start(vdec_test_h handle)
{
	int ret = 0;
	DecoderContext_t* dec_ctx = (DecoderContext_t*)handle;

	dec_ctx->isRun = 1;
    ret = pthread_create(&dec_ctx->hThread, NULL, DecoderThread, dec_ctx);
	if(ret != 0)
	{
		ret = -1;
	}

	return ret;
}

int vdec_test_suspend(vdec_test_h handle)
{
	DecoderContext_t* dec_ctx = (DecoderContext_t*)handle;

	DLOGE("not implemented yet!");
	return -1;
}

int vdec_test_resume(vdec_test_h handle)
{
	DecoderContext_t* dec_ctx = (DecoderContext_t*)handle;

	DLOGE("not implemented yet!");
	return -1;
}

//thread stop
int vdec_test_stop(vdec_test_h handle)
{
	DecoderContext_t* dec_ctx = (DecoderContext_t*)handle;

	dec_ctx->isRun = 0;
	DLOGI("decoder:%d join wait --", dec_ctx->dec_id);
	pthread_join(dec_ctx->hThread, NULL);
	DLOGI("decoder:%d join done ++", dec_ctx->dec_id);
	return 0;
}

//free resource
void vdec_test_destroy(vdec_test_h handle)
{
	DecoderContext_t* dec_ctx = (DecoderContext_t*)handle;

	if(dec_ctx != NULL)
	{
		DLOGI("destroy decoder:%d", dec_ctx->dec_id);

		free(dec_ctx);
		dec_ctx = NULL;
	}
}