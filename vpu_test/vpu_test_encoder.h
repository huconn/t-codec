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


#ifndef VPU_TEST_ENCODER_H
#define VPU_TEST_ENCODER_H

typedef void* venc_test_h;

//parameter:"-e test_raw.yuv 1920 1080 -v h264  ..." until next -e
//if parsing and create ok, venc_test_h is valid
venc_test_h venc_test_create(char* parameter);

//thread run
int venc_test_start(venc_test_h handle);

int venc_test_suspend(venc_test_h handle);

int venc_test_resume(venc_test_h handle);

//thread stop
int venc_test_stop(venc_test_h handle);

//free resource
void venc_test_destroy(venc_test_h handle);

#endif //VPU_TEST_ENCODER_H