/******************************************************************************
 * 
 * REVOLABS CONFIDENTIAL
 * __________________
 * 
 *  [2005] - [2016] Revolabs Incorporated 
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of Revolabs Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Revolabs Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Revolabs Incorporated.
 * 
 *****************************************************************************/

/**
 * @file dsptop.h
 * @brief This file implements the top module of DSP
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#ifndef _DSPTOP_H_
#define _DSPTOP_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DSPCH_IN_TC0L = 0,
  DSPCH_IN_TC0R,
  DSPCH_IN_AUD0L,
  DSPCH_IN_AUD0R,
  DSPCH_IN_TC1L,
  DSPCH_IN_TC1R,
  DSPCH_IN_AUD1L,
  DSPCH_IN_AUD1R,
  DSPCH_IN_NUM
} DSPCH_IN;

typedef enum {
  DSPCH_OUT_AUD0L = 0,
  DSPCH_OUT_AUD0R,
  DSPCH_OUT_AUD1L,
  DSPCH_OUT_AUD1R,
  DSPCH_OUT_NUM
} DSPCH_OUT;


/* Declare configuration */
#define DSP_NAME        scratch
#define DSP_NAME_STR    "scratch"
#define DSP_FS          48000
#define DSP_FRAMESIZE   256
#define DSP_CHNUM_IN    DSPCH_IN_NUM
#define DSP_CHNUM_OUT   DSPCH_OUT_NUM
//#define DSP_CHNUM_IN    2
//#define DSP_CHNUM_OUT   2

struct DSPTOPState_;
typedef struct DSPTOPState_ DSPTOPState;

/* Declare the functions */
DSPTOPState* dsptop_init(void);
void dsptop_proc(DSPTOPState* st, float* out[], float* in[]);


#ifdef __cplusplus
}
#endif
#endif /* _DSPTOP_H_ */
