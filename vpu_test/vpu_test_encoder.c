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
#include "vpu_test_encoder.h"

#include "video_encoder.h"

#define DLOGD(...) 		if(encoder_log_level > 2) { DLOG(__VA_ARGS__); }	//debugq
#define DLOGI(...) 		if(encoder_log_level > 1) { DLOG(__VA_ARGS__); }	//info
#define DLOGE(...) 		if(encoder_log_level > 0) { DLOG(__VA_ARGS__); }	//error

#define SUPPORT_OUTPUT_FILE_WRITE
#define MAX_BITRATE_INDEX   4
#define MAX_ENCODER_SUPPORT     16

typedef struct EncoderConfig_t
{
    int width;
    int height;
	int framerate;
	int key_interval;

    int userBitrate;
    int writeWithSize;

    char filename[1024];

    VideoEncoder_Codec_E eCodec;
    int yuv_format;
    int logLevel;

	unsigned long yuv_pmap_addr;
	unsigned long yuv_pmap_size;
} EncoderConfig_t;

typedef struct tagBitrateTable
{
    unsigned long ulResolution;
    unsigned long ulBitrate;
} BitrateTable_t;

typedef struct EncoderContext_t
{
	VideoEncoder_T* video_encoder;
	VideoEncoder_InitConfig_T init_config;

    int enc_id;

    int isRun;
    pthread_t hThread;

    EncoderConfig_t tConfig;

	unsigned char* yuv_buffer_va;
	unsigned long yuv_buffer_pa;
	unsigned long yuv_buffer_size;
} EncoderContext_t;

static int encoder_log_level = 2; //log

static const char* strEncCodecExt[] =
{
	"none",
    "h264",
    "hevc",
    "mjpg",
};

#define MAX_BITRATE_INDEX 4
static const BitrateTable_t bitrateTable[MAX_BITRATE_INDEX] =
{
    { ( 720 *  480), ( 2 * 1024 * 1024) },
    { (1280 *  720), ( 6 * 1024 * 1024) },
    { (1920 * 1080), (10 * 1024 * 1024) },
    { (7680 * 4320), (10 * 1024 * 1024) }
};

int alloc_yuv_buffer(EncoderContext_t* pctx)
{
	//yuv format
	int ret = 0;
	void* ptr_mmap = NULL;
	int iMemFd;

	DLOGI("pmap addr:0x%x, size:%d", pctx->tConfig.yuv_pmap_addr, pctx->tConfig.yuv_pmap_size);
	iMemFd = open("/dev/mem", (int)((unsigned int)O_RDWR | (unsigned int)O_NDELAY));
	if(iMemFd >= 0)
	{
		ptr_mmap = mmap(NULL, (size_t)pctx->tConfig.yuv_pmap_size, (int)((unsigned int)PROT_READ | (unsigned int)PROT_WRITE), MAP_SHARED, iMemFd, pctx->tConfig.yuv_pmap_addr);
		if (ptr_mmap == -1)
		{
			//DBG_ERR("shared mmap() failed!, name:%s", ptPrivateData->pmapName);
			pctx->yuv_buffer_va = NULL;
			ret = -1;
		}
		else
		{
			pctx->yuv_buffer_va = ptr_mmap;
			pctx->yuv_buffer_pa = pctx->tConfig.yuv_pmap_addr;
			pctx->yuv_buffer_size = pctx->tConfig.yuv_pmap_size;
			DLOGI("mmap success,  addr:0x%x/0x%x, size:%d", pctx->yuv_buffer_pa, pctx->yuv_buffer_va, pctx->yuv_buffer_size);
		}

		close(iMemFd);
	}

	return ret;
}

void release_yuv_buffer(EncoderContext_t* pctx)
{
	DLOGI("release yuv buffer,  addr:0x%x, size:%d", pctx->yuv_buffer_va, pctx->yuv_buffer_size);
	if((pctx->yuv_buffer_va != NULL) && (pctx->yuv_buffer_size > 0))
	{
		(void)munmap(pctx->yuv_buffer_va, pctx->yuv_buffer_size);
		pctx->yuv_buffer_va = NULL;
		DLOGI("yuv buffer release OK");
	}
}

static int parseEncoderCommand(EncoderContext_t* pctx, char* parameters)
{
    EncoderConfig_t* pconfig = &pctx->tConfig;

	DLOGI("parameters:%s", parameters);

	char* token = strtok(parameters, " ");
	DLOGI("token:%s", token);
    while (token != NULL)
	{
        if (strncmp(token, "-e", 2) == 0)
		{
            char* filename = strtok(NULL, " ");
            char* width = strtok(NULL, " ");
            char* height = strtok(NULL, " ");

            strcpy(pconfig->filename, filename);
			pconfig->width = atoi(width);
			pconfig->height = atoi(height);
			pconfig->framerate = 0;
			pconfig->userBitrate = 0;

			DLOGI("Encoder input, Filename: %s, Width: %d, Height: %d", pconfig->filename, pconfig->width, pconfig->height);
        }
        else if (strncmp(token, "-v", 2) == 0)
		{
            char* codec = strtok(NULL, " ");

			if((strcmp(codec, "h264") == 0) || (strcmp(codec, "avc") == 0))
			{
				pconfig->eCodec = VideoEncoder_Codec_H264;
			}
			else if((strcmp(codec, "hevc") == 0) || (strcmp(codec, "h265") == 0))
			{
				pconfig->eCodec = VideoEncoder_Codec_H265;
			}
			else if((strcmp(codec, "mjpg") == 0) || (strcmp(codec, "mjpeg") == 0))
			{
				pconfig->eCodec = VideoEncoder_Codec_JPEG;
			}

			DLOGI("Video Codec: %s, eCodec:%d", codec, pconfig->eCodec);
        }
		else if (strncmp(token, "-fps", 4) == 0)
		{
			char* fps = strtok(NULL, " ");

			pconfig->framerate = atoi(fps);
			DLOGI("fps: %d, %s", pconfig->framerate, fps);
		}
		else if (strncmp(token, "-key", 4) == 0)
		{
			char* keyinterval = strtok(NULL, " ");

			pconfig->key_interval = atoi(keyinterval);
			DLOGI("key_interval: %d, %s", pconfig->key_interval, keyinterval);
		}
		else if (strncmp(token, "-p", 2) == 0)
		{
            char* address = strtok(NULL, " ");
            char* size = strtok(NULL, " ");

            DLOGI("Physical Address: %s", address);
            DLOGI("Size: %s", size);

			pconfig->yuv_pmap_addr = strtoul(address, NULL, 16);
			pconfig->yuv_pmap_size = strtoul(size, NULL, 16);
			DLOGI("yuv input pmap:0x%x, size:0x%x(%d)", pconfig->yuv_pmap_addr, pconfig->yuv_pmap_size, pconfig->yuv_pmap_size);
        }
        else if (strncmp(token, "-b", 2) == 0)
		{
            char* bitrate = strtok(NULL, " ");

			pconfig->userBitrate = atoi(bitrate);
			DLOGI("Bitrate: %d", pconfig->userBitrate);
        }
		else if (strncmp(token, "-f", 2) == 0)
		{
            char* yuvFormat = strtok(NULL, " ");

			pconfig->yuv_format = atoi(yuvFormat);
            DLOGI("yuvFormat: %d", pconfig->yuv_format);
        }
		else if (strncmp(token, "-log", 4) == 0)
		{
            char* logLevel = strtok(NULL, " ");

            pconfig->logLevel = atoi(logLevel);
			DLOGI("logLevel: %d", pconfig->logLevel);
			encoder_log_level = pconfig->logLevel;
        }
        else if (strncmp(token, "-size", 5) == 0)
		{
            char* writeSize = strtok(NULL, " ");

            pconfig->writeWithSize = atoi(writeSize);
			DLOGI("writeSize: %d", pconfig->writeWithSize);
        }

        token = strtok(NULL, " ");
    }

	return 0;
}

/*
int argValidation(MainContext_t* pMainCtx)
{
    int ii;

    for(ii=0; ii<pMainCtx->nb_of_encoder; ii++)
    {
        EncoderConfig_t* pConfig = NULL;
        EncoderContext_t* pEnc = &pMainCtx->encoder[ii];
        pConfig = &pEnc->tConfig;

        if(strlen(pConfig->filename) == 0)
        {
            DBG("encoder id:%d, file_name is not set!!", pEnc);
            return -1;
        }

        if(pConfig->width <= 0 || pConfig->height <= 0)
        {
            DBG("Resolution is wrong - (%d x %d)", pConfig->width, pConfig->height);
            return -1;
        }

		if(pConfig->yuv_pmap_addr <= 0 || pConfig->yuv_pmap_size <= 0)
        {
            DBG("pmap addr:0x%x, size;%d", pConfig->yuv_pmap_addr, pConfig->yuv_pmap_size);
			DBG("To input YUV data, the appropriate size of pmap information matching the resolution and YUV format must be provided.");
            return -1;
        }

        if(pConfig->userBitrate == 0)
        {
            int b_idx;
            for(b_idx = 0; b_idx < MAX_BITRATE_INDEX; b_idx++)
            {
                if(pConfig->width * pConfig->height <= bitrateTable[b_idx].ulResolution)
                {
                    pConfig->userBitrate = bitrateTable[b_idx].ulBitrate;
                    break;
                }
            }

            if(b_idx >= MAX_BITRATE_INDEX)
            {
                DBG("Resolution is too big!!");
                return -1;
            }
        }
    }

    return 0;
}
*/

static void* EncoderThread(void* private)
{
    EncoderContext_t* pctx = (EncoderContext_t*)private;
    EncoderConfig_t* pConfig = &pctx->tConfig;
	VideoEncoder_Error_E venc_ret = VideoEncoder_Error_None;
	VideoEncoder_Set_Header_Result_T header_result;

    int ret = 0;
    int readbyte;
    int inBufferSize;
    int frameCount = 0;
    unsigned char *pucInBuffer = NULL;
    char strOutputFileName[1024];
    long long accumEncTime = 0;
    int i = 0;
	int yuv_alloc_ret = 0;
	int iOffsetUV;

    FILE *hInputFile = NULL;
    FILE *hOutputFile = NULL;

    DLOGI("[Enc %d] Encoding thread started, source %s", pctx->enc_id, pConfig->filename);

    hInputFile = fopen(pConfig->filename, "rb");
    if(hInputFile == NULL)
    {
        DLOGI("[Enc %d] file open failed!! - %s", pctx->enc_id, pConfig->filename);
        ret = -1;
        goto error_out;
    }

	yuv_alloc_ret = alloc_yuv_buffer(pctx);
	if(yuv_alloc_ret < 0)
	{
		DLOGI("[Enc %d] yuv buffer alloc failed", pctx->enc_id);
        ret = -1;
        goto error_out;
	}

	inBufferSize = (pConfig->width * pConfig->height * 3 / 2);
    //pucInBuffer = (unsigned char *)malloc(sizeof(unsigned char) * inBufferSize);

	if(inBufferSize > pctx->yuv_buffer_size)
	{
		DLOGI("[Enc %d] buffer size is not enough, inBuffer:%d, yuv buffer:%d", pctx->enc_id, inBufferSize, pctx->yuv_buffer_size);
        ret = -1;
        goto error_out;
	}

#ifdef SUPPORT_OUTPUT_FILE_WRITE
    if (pConfig->writeWithSize)
    {
        sprintf(strOutputFileName, "vpu_encode_id%02d_output_with_size.%s", pctx->enc_id, strEncCodecExt[pConfig->eCodec]);
    }
    else
    {
        sprintf(strOutputFileName, "vpu_encode_id%02d_output.%s", pctx->enc_id, strEncCodecExt[pConfig->eCodec]);
    }

    DLOGI("open output:%s", strOutputFileName);
    hOutputFile = fopen(strOutputFileName, "wb");
    if(hOutputFile == NULL)
    {
        DLOGI("[Enc %d] file open failed!! - %s", pctx->enc_id, strOutputFileName);
        ret = -1;
        goto error_out;
    }
#endif

	pctx->video_encoder = VideoEncoder_Create();
	if(pctx->video_encoder == NULL)
	{
		goto error_out;
	}

	memset(&pctx->init_config, 0x00, sizeof(VideoEncoder_InitConfig_T));

	pctx->init_config.eCodec = pConfig->eCodec;
	pctx->init_config.iImageWidth = pConfig->width;
	pctx->init_config.iImageHeight = pConfig->height;

	if(pConfig->framerate > 0)
	{
		pctx->init_config.iFrameRate = pConfig->framerate;
	}
	else
	{
		pctx->init_config.iFrameRate = 30;
	}

	if(pConfig->key_interval > 0)
	{
		pctx->init_config.iKeyFrameInterval = pConfig->key_interval;
	}
	else
	{
		pctx->init_config.iKeyFrameInterval = 30;
	}

	if(pConfig->userBitrate > 0)
	{
		pctx->init_config.iBitRate = pConfig->userBitrate;
	}
	else
	{
		int bitrate_idx = 0;
		int resolution = pctx->init_config.iImageWidth * pctx->init_config.iImageHeight;
		for(bitrate_idx = 0; bitrate_idx < MAX_BITRATE_INDEX; bitrate_idx++)
		{
			DLOGI("resolution:%d, bitrate table:%d, %llu", resolution, bitrate_idx, bitrateTable[bitrate_idx].ulResolution);
			if(resolution < bitrateTable[bitrate_idx].ulResolution)
			{
				break;
			}
		}

		pctx->init_config.iBitRate = bitrateTable[bitrate_idx].ulBitrate;
		DLOGI("set bitrate to  %d", pctx->init_config.iBitRate);
	}

	pctx->init_config.iSliceMode = 0;
	pctx->init_config.iSliceSizeUnit = 0; // 0:bits, 1:MBs
    pctx->init_config.iSliceSize = 4 * 1024;

	DLOGI("codec id:%d, iImageWidth:%d, iImageHeight:%d, FrameRat:%d, KeyFrameInterval:%d, BitRate:%d",
		pctx->init_config.eCodec,
		pctx->init_config.iImageWidth, pctx->init_config.iImageHeight,
		pctx->init_config.iFrameRate, pctx->init_config.iKeyFrameInterval,
		pctx->init_config.iBitRate);

    venc_ret = VideoEncoder_Init(pctx->video_encoder, &pctx->init_config);
	if(venc_ret != VideoEncoder_Error_None)
    {
        DLOGE("[Enc %d] initializing failed!!", pctx->enc_id);
        goto error_out;
    }

	venc_ret = VideoEncoder_Set_Header(pctx->video_encoder, &header_result);
	if(venc_ret != VideoEncoder_Error_None)
	{
		DLOGE("[Enc %d] set header failed!!", pctx->enc_id);
        goto error_out;
	}

#ifdef SUPPORT_OUTPUT_FILE_WRITE
    DLOGD("sequence header encoding done, size=%d, %d", header_result.iHeaderSize);
    if (header_result.iHeaderSize > 0)
    {
        if (pConfig->writeWithSize)
        {
            int size = header_result.iHeaderSize;
            fwrite(&size, 1, 4, hOutputFile);
        }

		{
			unsigned char* ptr = (unsigned char*)header_result.pucBuffer;
			DLOGI("[VENC-%d] write sequence header, iFrameSize:%d, %x %x %x %x %x %x %x %x", pctx->enc_id,
				header_result.iHeaderSize, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
		}

        fwrite(header_result.pucBuffer, 1, header_result.iHeaderSize, hOutputFile);
    }
#endif

    while(pctx->isRun == 1)
    {
        long long enc_start;
        long long enc_finish;

		VideoEncoder_Encode_Input_T enc_input;
		VideoEncoder_Encode_Result_T enc_output;

        if((readbyte = fread(pctx->yuv_buffer_va, inBufferSize, 1, hInputFile)) <= 0)
        {
            DLOGE("[Enc %d] End of file!!", pctx->enc_id);
            break;
        }

		enc_input.llTimeStamp = frameCount * 33000;
		enc_input.pucVideoBuffer[VE_IN_Y] = pctx->yuv_buffer_pa;

		iOffsetUV = (pConfig->width * pConfig->height) / 4;
		enc_input.pucVideoBuffer[VE_IN_U] = enc_input.pucVideoBuffer[VE_IN_Y] + (pConfig->width * pConfig->height);
        enc_input.pucVideoBuffer[VE_IN_V] = enc_input.pucVideoBuffer[VE_IN_U] + iOffsetUV;

		venc_ret = VideoEncoder_Encode(pctx->video_encoder, &enc_input, &enc_output);
		if(venc_ret != VideoEncoder_Error_None)
		{
			DLOGE("[Enc %d] set header failed!!", pctx->enc_id);
			goto error_out;
		}

       // enc_finish = TCC_GetTimeUs();
       // accumEncTime += (enc_finish - enc_start);

#ifdef SUPPORT_OUTPUT_FILE_WRITE
        if (enc_output.iFrameSize > 0)
        {
            if (pConfig->writeWithSize)
            {
                int size = enc_output.iFrameSize;
                fwrite(&size, 1, 4, hOutputFile);
            }

			{
				unsigned char* ptr = (unsigned char*)enc_output.pucBuffer;
				DLOGI("[VENC-%d]  picture type:%s, size:%d, %x %x %x %x %x %x %x %x", pctx->enc_id,
					(enc_output.ePictureType == VideoEncoder_Codec_Picture_I) ? "I" : "P",
					enc_output.iFrameSize, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
			}
            //DBG("[Enc %d] --- write, framecount:%d, size:%d", pctx->enc_id, frameCount, tEncode[i].tOutputParam.iFrameSize);
            fwrite(enc_output.pucBuffer, 1, enc_output.iFrameSize, hOutputFile);
            //DBG("[Enc %d] +++ write, framecount:%d", pctx->enc_id, frameCount);
        }
#endif

/*
#ifndef DEBUG
        if((frameCount == 0) || ((frameCount+1) % 30 == 0)) //every 1 sec
#endif
        {
            DBG_ONE_FRAME("[Enc %d] %4d frame encoded, size %5d, avg enc time:%7.2f us", pctx->enc_id, frameCount, tEncode[i].tOutputParam.iFrameSize, ((float)accumEncTime / (float)(frameCount+1)));
        }
*/
        frameCount++;

        usleep(1);
    }

    DLOGI("[Enc %d] ", pctx->enc_id);

encoding_fail:
    venc_ret = VideoEncoder_Deinit(pctx->video_encoder);
	if(venc_ret != VideoEncoder_Error_None)
    {
        DLOGE("[%s][%d] Encoder deinit fail", __func__, __LINE__);
    }

	venc_ret = VideoEncoder_Destroy(pctx->video_encoder);
	if(venc_ret != VideoEncoder_Error_None)
    {
        DLOGE("[%s][%d] Encoder destroy fail", __func__, __LINE__);
    }

    DLOGI("[Enc %d] ", pctx->enc_id);

error_out:
    if(hInputFile != NULL)
    {
        fclose(hInputFile);
        hInputFile = NULL;
    }

    DLOGI("[Enc %d] ", pctx->enc_id);
#ifdef SUPPORT_OUTPUT_FILE_WRITE
    if(hOutputFile != NULL)
    {
        fclose(hOutputFile);
        hOutputFile = NULL;
    }
#endif

	release_yuv_buffer(pctx);

    DLOGI("[Enc %d] ",pctx->enc_id);
    //if(pucInBuffer != NULL)
    //{
    //   free(pucInBuffer);
    //    pucInBuffer = NULL;
    //}

    DLOGI("[Enc %d] finished", pctx->enc_id);
    return NULL;
}

venc_test_h venc_test_create(char* parameter)
{
	venc_test_h handle = NULL;
	EncoderContext_t* enc_ctx = NULL;

	DLOGI("input parameter:%s", parameter);

	enc_ctx = malloc(sizeof(EncoderContext_t));
	if(enc_ctx != NULL)
	{
		(void)parseEncoderCommand(enc_ctx, parameter);
		enc_ctx->enc_id = 0;
		//invok argsValdate()
	}

	return (venc_test_h)enc_ctx;
}

int venc_test_start(venc_test_h handle)
{
	int ret = 0;
	EncoderContext_t* enc_ctx = (EncoderContext_t*)handle;

	enc_ctx->isRun = 1;
    ret = pthread_create(&enc_ctx->hThread, NULL, EncoderThread, enc_ctx);
	if(ret != 0)
	{
		ret = -1;
	}

	return ret;
}

int venc_test_suspend(venc_test_h handle)
{
	EncoderContext_t* enc_ctx = (EncoderContext_t*)handle;

	DLOGE("not implemented yet!");
	return -1;
}

int venc_test_resume(venc_test_h handle)
{
	EncoderContext_t* enc_ctx = (EncoderContext_t*)handle;

	DLOGE("not implemented yet!");
	return -1;
}

int venc_test_stop(venc_test_h handle)
{
	EncoderContext_t* enc_ctx = (EncoderContext_t*)handle;

	enc_ctx->isRun = 0;
	DLOGI("encoder:%d join wait --", enc_ctx->enc_id);
	pthread_join(enc_ctx->hThread, NULL);
	DLOGI("encoder:%d join done ++", enc_ctx->enc_id);
	return 0;
}

void venc_test_destroy(venc_test_h handle)
{
	EncoderContext_t* enc_ctx = (EncoderContext_t*)handle;

	if(enc_ctx != NULL)
	{
		DLOGI("destroy encoder:%d", enc_ctx->enc_id);

		free(enc_ctx);
		enc_ctx = NULL;
	}
}

