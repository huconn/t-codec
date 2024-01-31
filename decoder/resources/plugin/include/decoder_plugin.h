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

#ifndef DECODER_PLUGIN_H__
#define DECODER_PLUGIN_H__

#include <stdio.h>
#include <stdint.h>

#define DEFAULT_VIDEO_MIN_OUTPUT_WIDTH	    (16)
#define DEFAULT_VIDEO_MIN_OUTPUT_HEIGHT	    (16)
#define DEFAULT_VIDEO_MAX_OUTPUT_WIDTH	    (1920)
#define DEFAULT_VIDEO_MAX_OUTPUT_HEIGHT	    (1088)
#define DEFAULT_IMAGE_MAX_OUTPUT_WIDTH	    (8196)
#define DEFAULT_IMAGE_MAX_OUTPUT_HEIGHT	    (8196)
#define DEFAULT_4K_VIDEO_MAX_OUTPUT_WIDTH	(4096)
#define DEFAULT_4K_VIDEO_MAX_OUTPUT_HEIGHT	(2160)
#define DEFAULT_VIDEO_DEFAULT_FRAMERATE	    (30000)


typedef void *DEC_PLUGIN_H;

union DecPluginCastAddr
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

static inline uintptr_t DecPlugin_CastAddr_VOID_UINTPTR(void *pvAddr)
{
    union DecPluginCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.uintptrAddr;
}

static inline unsigned char *DecPlugin_CastAddr_VOID_UCHARPTR(void *pvAddr)
{
    union DecPluginCastAddr un_cast;
    un_cast.pvAddr = NULL;
    un_cast.pvAddr = pvAddr;
    return un_cast.pucAddr;
}

typedef struct
{
	const char *name;

	DEC_PLUGIN_H (*open)(void);
	int (*close)(DEC_PLUGIN_H hPlugin);

	int (*init)(DEC_PLUGIN_H hPlugin, void *pvInitConfig);
	int (*deinit)(DEC_PLUGIN_H hPlugin);

	int (*decode)(DEC_PLUGIN_H hPlugin, unsigned int uiStreamLen, unsigned char *pucStream, void *pvDecodedStream);

	int (*set)(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
	int (*get)(DEC_PLUGIN_H hPlugin, unsigned int uiId, void *pvStructure);
} DecoderPluginDesc_T;


extern DecoderPluginDesc_T G_tDecoderPluginDesc;


#define DECODER_PLUGIN_DEFINE(name, open, close, init, deinit, decode, set, get)	\
	DecoderPluginDesc_T G_tDecoderPluginDesc = {									\
		name, open,	close, init, deinit, decode, set, get							\
	};

#endif	// DECODER_PLUGIN_H__
