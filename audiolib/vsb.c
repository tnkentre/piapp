/******************************************************************************
 * 
 * TANAKA ENTERPRISES CONFIDENTIAL
 * __________________
 * 
 *  [2015] - [2016] Tanaka Enterprises
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of Tanaka Enterprises and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Tanaka Enterprises
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Tanaka Enterprises.
 * 
 *****************************************************************************/

/**
 * @file vsb.c
 * @brief This file implements a variable speed buffer module.
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */
#include "RevoRT.h"
#include "RevoRT_complex.h"
#include "Registers.h"
#include "FixedHeap.h"
#include "vectors.h"
#include "cli.h"
#include "vsb.h"

AC4_MODULE;

struct VSB_State_ {
  AC4_HEADER;
  /* configuration and history */
  int    size;
  float* buf[2];
  float  fpos;
  float  fpos_prev;
  float  speed_prev;
  float  src_prev[2];

  /* parameters */
  float loopgain;
  float feedbackgain;
  float recgain;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(loopgain,     CLI_ACFPTM,   VSB_State, "Loopback gain"),
  AC_REGDEF(feedbackgain, CLI_ACFPTM,   VSB_State, "Feedback gain"),
  AC_REGDEF(recgain,      CLI_ACFPTM,   VSB_State, "Recording gain"),
};

VSB_State *vsb_init(const char * name, int size)
{
  VSB_State* st;
  st = MEM_ALLOC(MEM_SDRAM, VSB_State, 1, 8);

  AC4_ADD(name, st, "VSB");
  AC4_ADD_REG(rd);

  st->size = size;
  st->buf[0] = MEM_ALLOC(MEM_SDRAM, float, st->size, 8);
  st->buf[1] = MEM_ALLOC(MEM_SDRAM, float, st->size, 8);
  vec_set(st->buf[0], 0.0f, st->size);
  vec_set(st->buf[1], 0.0f, st->size);
  st->loopgain = 1.0f;
  st->feedbackgain = 1.0f;
  st->recgain = 1.0f;
	
  return st;
}

void vsb_process(VSB_State * restrict st, float* dst[], float* src[], float* speed, int len)
{
  int   i, j;
  int   size = st->size;
  int   ipos;
  int   ipos_prev;
  int   ipos_temp;
  float fpos;
  float fpos_prev;
  float src_prev[2];
  float speed_prev;
  float bal0, bal1;
  int   bufidx;

  for (i=0; i<len; i++) {
    // Get history
    fpos_prev = st->fpos_prev;
    fpos      = st->fpos;
    src_prev[0] = st->src_prev[0];
    src_prev[1] = st->src_prev[1];
    speed_prev  = st->speed_prev;

    // compute index
    ipos_prev = (int)(fpos_prev);
    ipos      = (int)(fpos);

    // get recorded data
    bal1 = fpos - (float)ipos;
    bal0 = 1.0 - bal1;
    bufidx = ipos+1 < size ? ipos+1 : 0;
    if (speed_prev==0.0f) {
      dst[0][i] = 0.f;
      dst[0][i] = 0.f;
    else {
      dst[0][i] = st->loopgain * st->feedbackgain * (bal0 * st->buf[0][ipos] + bal1 * st->buf[0][bufidx]);
      dst[1][i] = st->loopgain * st->feedbackgain * (bal0 * st->buf[1][ipos] + bal1 * st->buf[1][bufidx]);
    }

    // --- write data ---
    if (ipos != ipos_prev) {
      if (speed_prev > 0.f) {
        // writing during ipos_prev --> ipos
        ipos_temp = ipos > ipos_prev ? ipos : ipos + st->size;
        for (j=ipos_prev+1; j<=ipos_temp; j++) {
          bal1 = ((float)j - fpos_prev) / (fpos - fpos_prev);
          bal0 = 1.f - bal1;
          bufidx = j < size ? j : j-size;
          st->buf[0][bufidx] *= st->feedbackgain;
          st->buf[1][bufidx] *= st->feedbackgain;
          st->buf[0][bufidx] += st->recgain * (bal0 * src_prev[0] + bal1 * src[0][i]);
          st->buf[1][bufidx] += st->recgain * (bal0 * src_prev[1] + bal1 * src[0][i]);
        }
      }
      else if (speed_prev < 0.f) {
        // writing during ipos_prev --> ipos
        ipos_temp = ipos < ipos_prev ? ipos : ipos - st->size;
        for (j=ipos_prev; j>ipos_temp; j--) {
          bal1 = (fpos_prev - (float)j) / (fpos_prev - fpos);
          bal0 = 1.f - bal1;
          bufidx = j > 0 ? j : j+size;
          st->buf[0][bufidx] *= st->feedbackgain;
          st->buf[1][bufidx] *= st->feedbackgain;
          st->buf[0][bufidx] += st->recgain * (bal0 * src_prev[0] + bal1 * src[0][i]);
          st->buf[1][bufidx] += st->recgain * (bal0 * src_prev[1] + bal1 * src[0][i]);
        }
      }
    }

    // update history
    fpos += speed[i];
    while (fpos <  0.f)           fpos += (float)(size);
    while (fpos >= (float)(size)) fpos -= (float)(size);
    st->fpos_prev   = st->fpos;
    st->fpos        = fpos;
    st->src_prev[0] = src[0][i];
    st->src_prev[1] = src[1][i];
    st->speed_prev  = speed[i];
  }
}

void vsb_set_feedbackgain(VSB_State * restrict st, float feedbackgain)
{
  st->feedbackgain = feedbackgain;
}

float vsb_vinylpos(VSB_State * restrict st)
{
  return st->fpos / st->size;
//  return (st->ipos + st->fpos) / st->size;
}

