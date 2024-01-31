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

#ifndef ADEC_H_
#define ADEC_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "audio_common.h"

typedef audio_streaminfo_t adec_input_t;
typedef audio_pcminfo_t adec_output_t;

#define		TCAS_CODEC_NOLFEMIX			(0)
#define		TCAS_CODEC_LFEMIX			(1)

#define		TCAS_CODEC_NODOWNMIX		(0)
#define		TCAS_CODEC_CENTER			(1)
#define		TCAS_CODEC_LR				(2)
#define		TCAS_CODEC_LtRt				(3)
#define		TCAS_CODEC_CLR				(4)
#define		TCAS_CODEC_LRCs				(5)
#define		TCAS_CODEC_CLRCs			(6)
#define		TCAS_CODEC_LRLsRs			(7)
#define		TCAS_CODEC_CLRLsRs			(8)

//! Data structure for Audio decoder initializing on CDK platform
typedef struct adec_init_t
{
	unsigned char  *m_pucExtraData;		//!< extra data
	int 			m_iExtraDataLen;	//!< extra data length in bytes

	adec_input_t   *m_psAudiodecInput;
	adec_output_t  *m_psAudiodecOutput;

	int 			m_iDownMixMode;

	// Callback Func
	void* (*m_pfMalloc		 ) ( size_t size );								//!< malloc
	void  (*m_pfFree		 ) ( void* ptr );										//!< free
	void* (*m_pfMemcpy		 ) ( void* dest, const void* source, size_t num );			//!< memcpy
	void* (*m_pfMemset		 ) ( void* dest, int c, size_t count);					//!< memset
	void* (*m_pfRealloc		 ) ( void* ptr, size_t size );							//!< realloc
	void* (*m_pfMemmove		 ) ( void* dest, const void* source, size_t num );			//!< memmove

	void*		 (*m_pfFopen	) (const char *filename, const char *mode);					//!< fopen
	size_t (*m_pfFread	) (void* ptr, size_t size, size_t count, void* stream);	//!< fread
	int			 (*m_pfFseek	) (void* stream, long offset, int origin);							//!< fseek
	long		 (*m_pfFtell	) (void* stream);										//!< ftell
	int			 (*m_pfFclose   ) (void *stream);										//!< fclose

	char		   *m_pcOpenFileName;

	int m_unAudioCodecParams[32];

} adec_init_t;



#ifdef __cplusplus
}
#endif

#endif //ADEC_H_
