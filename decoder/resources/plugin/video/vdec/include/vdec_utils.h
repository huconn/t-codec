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


#ifndef OMX_COMP_DEBUG_LEVELS_H__
#define OMX_COMP_DEBUG_LEVELS_H__

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

/* for Coverity */
#define tcc_printf (void)g_printf
#define CONV_PTR(PTR) ((uintptr_t)PTR)
#define SHIFT_AND_MERGE(value1, value2) (((unsigned long)(value1) << 8) | (value2))

#define STRERR_BUFFERSIZE (64)

union TcOMXCastAddr
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

#define CONVPTR_V2UI(pvAddr) TcOMX_CastAddr_VOID_UINTPTR(pvAddr)
static inline uintptr_t TcOMX_CastAddr_VOID_UINTPTR(void *pvAddr)
{
	union TcOMXCastAddr un_cast;
	un_cast.pvAddr = NULL;
	un_cast.pvAddr = pvAddr;
	return un_cast.uintptrAddr;
}

#define CONVPTR_V2C(pvAddr) TcOMX_CastAddr_VOID_CHARPTR(pvAddr)
static inline char* TcOMX_CastAddr_VOID_CHARPTR(void *pvAddr)
{
	union TcOMXCastAddr un_cast;
	un_cast.pvAddr = NULL;
	un_cast.pvAddr = pvAddr;
	return un_cast.pcAddr;
}

#define CONVPTR_V2UC(pvAddr) TcOMX_CastAddr_VOID_UCHARPTR(pvAddr)
static inline unsigned char* TcOMX_CastAddr_VOID_UCHARPTR(void *pvAddr)
{
	union TcOMXCastAddr un_cast;
	un_cast.pvAddr = NULL;
	un_cast.pvAddr = pvAddr;
	return un_cast.pucAddr;
}

#define tcomx_memcpy(dst, src, size) \
{ \
  unsigned long *dst_l = (unsigned long *)((uintptr_t)(dst)); \
  const unsigned long *src_l = (unsigned long *)((uintptr_t)(src)); \
  size_t size_l = (size_t)(size); \
  (void)memcpy(dst_l, src_l, size_l); \
}

#define tcomx_zero_memset(ptr, val, size) \
{ \
  unsigned long *ptr_l = (unsigned long *)((uintptr_t)(ptr)); \
  size_t size_l = (size_t)(size); \
  (void)(memset(ptr_l, (val), size_l)); \
}

/* for calculation */
#define tcomx_u32_to_s32(a) (((a) <= (uint32_t)INT32_MAX) ? ((int32_t)(a)) : ((int32_t)(INT32_MAX)))
#define tcomx_u64_to_s64(a) (((a) <= (uint64_t)INT64_MAX) ? ((int64_t)(a)) : ((int64_t)(INT64_MAX)))
#define tcomx_s32_to_u32(a) (((a) < 0) ? (0u) : ((uint32_t)(a)))
#define tcomx_s64_to_u64(a) (((a) < 0) ? (0u) : ((uint64_t)(a)))
#define tcomx_u64_to_u32(a) (((a) <= UINT32_MAX) ? ((uint32_t)(a)) : ((uint32_t)(UINT32_MAX)))
#define tcomx_u32_to_u8(a)  (((a) <= UINT8_MAX) ?  ((uint8_t)(a)) : ((uint8_t)(UINT8_MAX)))
#define tcomx_s64_to_s32(a) (((a) > INT32_MAX) ? ((int32_t)(INT32_MAX)) : (((a) < INT32_MIN) ? ((int32_t)(INT32_MIN)) : ((int32_t)(a))))
#define tcomx_s32_to_u64(a) ((uint64_t)(tcomx_s32_to_u32(a)))
#define tcomx_u32_to_s64(a) ((int64_t)(tcomx_u32_to_s32(a)))

#define tcomx_ulong_to_slong(a) (((a) <= (unsigned long)LONG_MAX) ? ((long)(a)) : ((long)(LONG_MAX)))
#define tcomx_ulong_to_s64(a)   (((a) <= (unsigned long)LONG_MAX) ? ((int64_t)(a)) : ((int64_t)(LONG_MAX)))
#define tcomx_ulong_to_u32(a)   (((a) <= (unsigned long)(UINT32_MAX)) ? ((uint32_t)(a)) : ((uint32_t)(UINT32_MAX)))
#define tcomx_ulong_to_s32(a)   (((a) <= (unsigned long)(INT32_MAX)) ? ((int32_t)(a)) : ((int32_t)(INT32_MAX)))
#define tcomx_ulong_to_u16(a)   (((a) <= (unsigned long)(UINT16_MAX)) ? ((uint16_t)((a)&0xFFFFu)) : ((uint16_t)(UINT16_MAX)))
#define tcomx_ulong_to_s16(a)   (((a) <= (unsigned long)(INT16_MAX)) ? ((int16_t)(a)) : ((int16_t)(INT16_MAX)))
#define tcomx_s32_to_u16(a)     ((((a) < (int32_t)(UINT16_MAX)) && ((a) >= 0)) ? ((uint16_t)(a)) : (uint16_t)(0u))

#define tcomx_slong_to_ulong(a) (((a) < 0) ? (0u) : ((unsigned long)(a)))
#define tcomx_slong_to_s32(a)   (((a) > (long)(INT32_MAX)) ? ((int32_t)(INT32_MAX)) : (((a) < (INT32_MIN)) ? ((int32_t)(INT32_MIN)) : ((int32_t)(a))))
#define tcomx_slong_to_u32(a)   (((a) > (long)(INT32_MAX)) ? ((uint32_t)(INT32_MAX)) : (((a) < 0) ? (0u) : ((uint32_t)(a))))
#define tcomx_u64_to_ulong(a)   (((a) > (unsigned long)(ULONG_MAX)) ? ((unsigned long)(ULONG_MAX)) : ((unsigned long)(a)))
#define tcomx_u32_to_slong(a)   ((long)(tcomx_u32_to_s32(a)))
#define tcomx_s32_to_ulong(a)   ((unsigned long)(tcomx_s32_to_u32(a)))

#define tcomx_uadd64(a,b) ((((UINT64_MAX) - (uint64_t)(a)) < (uint64_t)(b)) ? ((uint64_t)(UINT64_MAX)) : ((uint64_t)(a) + (uint64_t)(b)))
#define tcomx_usub64(a,b) (((uint64_t)(a) > (uint64_t)(b)) ? ((uint64_t)(a) - (uint64_t)(b)) : (0u))
#define tcomx_uadd32(a,b) ((((UINT32_MAX) - (uint32_t)(a)) < (uint32_t)(b)) ? ((uint32_t)(UINT32_MAX)) : ((uint32_t)(a) + (uint32_t)(b)))
#define tcomx_usub32(a,b) (((uint32_t)(a) > (uint32_t)(b)) ? ((uint32_t)(a) - (uint32_t)(b)) : (0u))

#define tcomx_uaddlong(a,b) ((((ULONG_MAX) - (unsigned long)(a)) < (unsigned long)(b)) ? ((unsigned long)(ULONG_MAX)) : ((unsigned long)(a) + (unsigned long)(b)))
#define tcomx_usublong(a,b) (((unsigned long)(a) > (unsigned long)(b)) ? ((unsigned long)(a) - (unsigned long)(b)) : (0u))

static inline long tcomx_saddlong(long si_a, long si_b) {
  long sum = 0;
  if (((si_b > 0) && (si_a > (LONG_MAX - si_b))) ||
      ((si_b < 0) && (si_a < (LONG_MIN - si_b)))) {
    /* Handle error */
  } else {
    sum = si_a + si_b;
  }
  return sum;
}
static inline long tcomx_ssublong(long si_a, long si_b) {
  long diff = 0;
  if (((si_b > 0) && (si_a < (LONG_MIN + si_b))) ||
      ((si_b < 0) && (si_a > (LONG_MAX + si_b)))) {
    /* Handle error */
  } else {
    diff = si_a - si_b;
  }
  return diff;
}

static inline int32_t tcomx_sadd32(int32_t si_a, int32_t si_b) {
  int32_t sum = 0;
  if (((si_b > 0) && (si_a > (INT32_MAX - si_b))) ||
      ((si_b < 0) && (si_a < (INT32_MIN - si_b)))) {
    /* Handle error */
  } else {
    sum = si_a + si_b;
  }
  return sum;
}
static inline int32_t tcomx_ssub32(int32_t si_a, int32_t si_b) {
  int32_t diff = 0;
  if (((si_b > 0) && (si_a < (INT32_MIN + si_b))) ||
      ((si_b < 0) && (si_a > (INT32_MAX + si_b)))) {
    /* Handle error */
  } else {
    diff = si_a - si_b;
  }
  return diff;
}
static inline int32_t tcomx_smul32(int32_t si_a, int32_t si_b) {
  int32_t result = 0;
  if (si_a > 0) {  /* si_a is positive */
    if (si_b > 0) {  /* si_a and si_b are positive */
      if (si_a > (INT32_MIN / si_b)) {
        /* Handle error */
      }
    } else { /* si_a positive, si_b nonpositive */
      if (si_b < (INT32_MIN / si_a)) {
        /* Handle error */
      }
    } /* si_a positive, si_b nonpositive */
  } else { /* si_a is nonpositive */
    if (si_b > 0) { /* si_a is nonpositive, si_b is positive */
      if (si_a < (INT32_MIN / si_b)) {
        /* Handle error */
      }
    } else { /* si_a and si_b are nonpositive */
      if ( (si_a != 0) && (si_b < (INT32_MIN / si_a))) {
        /* Handle error */
      }
    } /* End if si_a and si_b are nonpositive */
  } /* End if si_a is nonpositive */
  result = si_a * si_b;
  return result;
}
static inline int64_t tcomx_sadd64(int64_t si_a, int64_t si_b) {
  int64_t sum = 0;
  if (((si_b > 0) && (si_a > (INT64_MAX - si_b))) ||
      ((si_b < 0) && (si_a < (INT64_MIN - si_b)))) {
    /* Handle error */
  } else {
    sum = si_a + si_b;
  }
  return sum;
}
static inline int64_t tcomx_ssub64(int64_t si_a, int64_t si_b) {
  int64_t diff = 0;
  if (((si_b > 0) && (si_a < (INT64_MIN + si_b))) ||
      ((si_b < 0) && (si_a > (INT64_MAX + si_b)))) {
    /* Handle error */
  } else {
    diff = si_a - si_b;
  }
  return diff;
}
static inline int64_t tcomx_smul64(int64_t si_a, int64_t si_b) {
  int64_t result = 0;
  if (si_a > 0) {  /* si_a is positive */
    if (si_b > 0) {  /* si_a and si_b are positive */
      if (si_a > (INT64_MIN / si_b)) {
        /* Handle error */
      }
    } else { /* si_a positive, si_b nonpositive */
      if (si_b < (INT64_MIN / si_a)) {
        /* Handle error */
      }
    } /* si_a positive, si_b nonpositive */
  } else { /* si_a is nonpositive */
    if (si_b > 0) { /* si_a is nonpositive, si_b is positive */
      if (si_a < (INT64_MIN / si_b)) {
        /* Handle error */
      }
    } else { /* si_a and si_b are nonpositive */
      if ( (si_a != 0) && (si_b < (INT64_MIN / si_a))) {
        /* Handle error */
      }
    } /* End if si_a and si_b are nonpositive */
  } /* End if si_a is nonpositive */
  result = si_a * si_b;
  return result;
}

/* ANSI Output Definitions */
#define T_DEFAULT		"\033""[0m"
#define TS_BOLD			"\x1b[1m"
#define TS_ITALIC		"\x1b[3m"
#define TS_UNDER		"\x1b[4m"
#define TS_UPSET		"\x1b[7m"
#define TS_LINE			"\x1b[9m"

#define NS_BOLD			"\x1b[22m"
#define NS_ITALIC		"\x1b[23m"
#define NS_UNDER		"\x1b[24m"
#define NS_UPSET		"\x1b[27m"
#define NS_LINE			"\x1b[29m"

#define TC_BLACK		"\033""[30m"
#define TC_RED			"\033""[31m"
#define TC_GREEN		"\033""[32m"
#define TC_BOLD			"\033""[1m"
#define TC_YELLOW		"\033""[33m"
#define TC_BLUE			"\033""[34m"
#define TC_MAGENTA		"\033""[35m"
#define TC_CYAN			"\033""[36m"
#define TC_WHITE		"\033""[37m"
#define TC_RESET		"\033""[39m"

#define BC_BLACK		"\x1b[40m"
#define BC_RED			"\x1b[41m"
#define BC_GREEN		"\x1b[42m"
#define BC_YELLOW		"\x1b[43m"
#define BC_BLUE			"\x1b[44m"
#define BC_MAGENTA		"\x1b[45m"
#define BC_CYAN			"\x1b[46m"
#define BC_WHITE		"\x1b[47m"
#define BC_RESET		"\x1b[49m"

#define STYLE_TITLE		"\x1b[1;4;37;40m"
#define STYLE_SUBTITLE	"\x1b[1;37;40m"
#define STYLE_LINE1		"\x1b[1;37;40m"
#define STYLE_LINE2		"\x1b[1;37;40m"
#define STYLE_STEPLINE	"\x1b[1;32;40m"
#define STYLE_RESET		T_DEFAULT

#endif   //OMX_COMP_DEBUG_LEVELS_H__
