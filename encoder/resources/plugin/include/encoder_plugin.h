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

#ifndef TENCODER_PLUGIN_H__
#define TENCODER_PLUGIN_H__

#include <stdio.h>
#include <stdint.h>

#define DEFAULT_ENC_VIDEO_MIN_OUTPUT_WIDTH	    (16)
#define DEFAULT_ENC_VIDEO_MIN_OUTPUT_HEIGHT	    (16)
#define DEFAULT_ENC_VIDEO_MAX_OUTPUT_WIDTH	    (1920)
#define DEFAULT_ENC_VIDEO_MAX_OUTPUT_HEIGHT	    (1088)
#define DEFAULT_ENC_4K_VIDEO_MAX_OUTPUT_WIDTH	(4096)
#define DEFAULT_ENC_4K_VIDEO_MAX_OUTPUT_HEIGHT	(2160)
#define DEFAULT_ENC_VIDEO_DEFAULT_FRAMERATE	    (30000)


typedef void *TENC_PLUGIN_H;

typedef struct
{
	int iWidth;
	int iHeight;

    int iFrameRate;
    int iKeyFrameInterval;
    int iBitRate;

    int iSliceMode;
	int iSliceSizeUnit;
	int iSliceSize;
} TENC_PLUGIN_INIT_T;

union EncPluginCastAddr
{
    intptr_t intptrAddr;
    uintptr_t uintptrAddr;
    unsigned char *pucAddr;
    char *pcAddr;
    unsigned int *puiAddr;
    int *piAddr;
    void *pvAddr;
    unsigned long *pulAddr;
    long *plAddr;
};

static inline uintptr_t EncPlugin_CastAddr_VOID_UINTPTR(void *pvAddr)
{
    union EncPluginCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.uintptrAddr;
}

static inline unsigned char *EncPlugin_CastAddr_VOID_UCHARPTR(void *pvAddr)
{
    union EncPluginCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.pucAddr;
}

typedef struct
{
	const char *name;

	TENC_PLUGIN_H (*open)(void);
	int (*close)(TENC_PLUGIN_H hPlugin);

	int (*init)(TENC_PLUGIN_H hPlugin, TENC_PLUGIN_INIT_T *ptSetInit);
	int (*deinit)(TENC_PLUGIN_H hPlugin);
	int (*encode)(TENC_PLUGIN_H hPlugin, void *pvEncodeInput, void *pvEncodingResult);

	int (*set)(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
	int (*get)(TENC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
} EncoderPluginDesc_T;


extern EncoderPluginDesc_T G_tEncoderPluginDesc;

#define TENCODER_PLUGIN_DEFINE(name, open, close, init, deinit, encode, set, get)	\
	EncoderPluginDesc_T G_tEncoderPluginDesc = {									\
		name, open,	close, init, deinit, encode, set, get							\
	};


#endif	// TENCODER_PLUGIN_H__