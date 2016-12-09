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
 * @file dsptop.c
 * @brief This file implements the top module of DSP
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#include "RevoRT.h"
#include "Registers.h"
#include "FixedHeap.h"
#include "cli.h"
#include "vectors.h"
#include "vectors_cplx.h"
#include "FB.h"
#include "FB_filters.h"
#include "tcanalysis.h"
#include "vsb.h"
#include "dsptop.h"

//----------------------------------------------------------
// DSP State
//----------------------------------------------------------
struct DSPTOPState_ {
  int fs;
  int frame_size;
  int nband;
  float gain;
  rv_complex * X[DSP_CHNUM_IN];
  rv_complex * Y[DSP_CHNUM_OUT];
  FBanalysisState  * ana[DSP_CHNUM_IN];
  FBsynthesisState * syn[DSP_CHNUM_OUT];

  TCANALYSIS_State * tcana[2];
  VSB_State        * vsb[2];
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(gain, CLI_ACFPTM, DSPTOPState, "output gain"),
};

//----------------------------------------------------------
// DSP initializing
//----------------------------------------------------------
DSPTOPState* dsptop_init(void)
{
  int i;
  DSPTOPState* st;
  const FB_filters_info * fbinfo;

  initRevoRT();

  st = MEM_ALLOC(MEM_SDRAM, DSPTOPState, 1, 4);
  AC_ADD("dsptop", CLI_DSPTOP, st, "DSPTOP");
  AC_ADD_REG(rd, CLI_DSPTOP);

  st->fs         = DSP_FS;
  st->frame_size = DSP_FRAMESIZE;
  st->nband      = DSP_FRAMESIZE;

  fbinfo = FB_filters_getinfo(st->frame_size, 2 * st->nband, 4 * st->frame_size);
  for (i = 0; i < DSP_CHNUM_IN; i++) {
    st->X[i] = MEM_ALLOC(MEM_SDRAM, rv_complex, st->nband + 1, 8);
    st->ana[i] = FBanalysis_init(fbinfo->fftsize, fbinfo->decimation, fbinfo->lh, fbinfo->filter_h, 1);
  }
  for (i = 0; i < DSP_CHNUM_OUT; i++) {
    st->Y[i] = MEM_ALLOC(MEM_SDRAM, rv_complex, st->nband + 1, 8);
    st->syn[i] = FBsynthesis_init(fbinfo->fftsize, fbinfo->decimation, fbinfo->lg, fbinfo->filter_g, 1);
  }

  st->gain = 1.f;

  st->tcana[0] = tcanalysis_init("TC0", st->fs, 1000);
  st->tcana[1] = tcanalysis_init("TC1", st->fs, 1000);
  st->vsb[0] = vsb_init("VSB0", (int)(st->fs * 60.f / 33.3f));
  st->vsb[1] = vsb_init("VSB0", (int)(st->fs * 60.f / 33.3f));

  return st;
}

//----------------------------------------------------------
// DSP processing
//----------------------------------------------------------
void dsptop_proc(DSPTOPState* st, float* out[], float* in[])
{
#if 0
  int i;
  for (i = 0; i < DSP_CHNUM_IN; i++) {
    FBanalysis_process(st->ana[i], st->X[i], in[i]);
  }

  for (i = 0; i < MIN(DSP_CHNUM_IN, DSP_CHNUM_OUT); i++) {
    vec_cplx_muls(st->Y[i], st->X[i], st->gain, st->nband);
  }
  for (; i < DSP_CHNUM_OUT; i++) {
    vec_set((float*)st->Y[i], 0.0f, 2 * st->nband);
  }

  for (i = 0; i < DSP_CHNUM_OUT; i++) {
    CPX_ZERO(st->Y[i][st->nband]);
    FBsynthesis_process(st->syn[i], out[i], st->Y[i]);
  }
#else
  int frame_size = st->frame_size;

  VARDECLR(float, speed_L);
  VARDECLR(float, speed_R);
  SAVE_STACK;
  ALLOC(speed_L, frame_size, float);
  ALLOC(speed_R, frame_size, float);

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana[0], speed_L, in[DSPCH_IN_TC0L], in[DSPCH_IN_TC0R], frame_size);
  tcanalysis_process(st->tcana[1], speed_R, in[DSPCH_IN_TC1L], in[DSPCH_IN_TC1R], frame_size);

  /* Turntable on left side */
  vsb_process(st->vsb[0], &out[DSPCH_OUT_AUD0L], &in[DSPCH_IN_AUD0L], speed_L, frame_size);
  vsb_process(st->vsb[1], &out[DSPCH_OUT_AUD1L], &in[DSPCH_IN_AUD1L], speed_L, frame_size);

  RESTORE_STACK;
#endif
}

