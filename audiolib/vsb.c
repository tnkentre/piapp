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
  int    ipos;
  float  fpos;
  float  rec[2];
  float  writestock[2];
  int    idxstock;
  float*  position;
  int     position_len;

  /* parameters */
  float loopgain;
  float feedbackgain;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(loopgain,     CLI_ACFPTM,   VSB_State, "Loopback gain"),
  AC_REGDEF(feedbackgain, CLI_ACFPTM,   VSB_State, "Feedback gain"),
  AC_REGADEF(position, position_len, CLI_ACFPTMA,   VSB_State, "position"),
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
  st->ipos   = 0;
  st->fpos   = 0.0f;
  st->loopgain = 1.0f;
  st->feedbackgain = 1.0f;
	
  st->writestock[0] = 0.0f;
  st->writestock[1] = 0.0f;
  st->idxstock      = 0;

  st->position_len = 2;
  st->position = MEM_ALLOC(MEM_SDRAM, float, st->position_len, 8);

  return st;
}

void vsb_process(VSB_State * restrict st, float* dst[], float* src[], float* speed, float* note, int len)
{
  int   i, j;
  int   ipos_next;
  float fpos_next;
  int   ipos_nexttmp;
  int   ipos_curtmp;
  float norm;
  float out[2];
  
  for (i=0; i<len; i++) {
    // compute buffer's position
    ipos_next = st->ipos;
    fpos_next = st->fpos + speed[i];
    ipos_next += (int)fpos_next;
    fpos_next -= (float)((int)fpos_next);
    if (fpos_next < 0.0f) {
      ipos_next -= 1;
      fpos_next += 1.0f;
    }
    if (ipos_next <  0)      ipos_next += st->size;
    if (ipos_next >= st->size) ipos_next -= st->size;

    // write rec data when buffer is free
    if (speed[i] == 0.0f) {
      st->writestock[0] = 0.0f;
      st->writestock[1] = 0.0f;
    }
    if (ipos_next != st->ipos) {
      st->buf[0][st->idxstock] = st->feedbackgain * st->buf[0][st->idxstock] + st->writestock[0];
      st->buf[1][st->idxstock] = st->feedbackgain * st->buf[1][st->idxstock] + st->writestock[1];
    }

    // get recorded data
#if 0
    out[0] = src[0][i] + st->loopgain * ((1.0f - fpos_next) * st->buf[0][ipos_next] + fpos_next * st->buf[0][ipos_next+1<st->size?ipos_next+1:0]);
    out[1] = src[1][i] + st->loopgain * ((1.0f - fpos_next) * st->buf[1][ipos_next] + fpos_next * st->buf[1][ipos_next+1<st->size?ipos_next+1:0]);
#else
    out[0] = st->loopgain * ((1.0f - fpos_next) * st->buf[0][ipos_next] + fpos_next * st->buf[0][ipos_next+1<st->size?ipos_next+1:0]);
    out[1] = st->loopgain * ((1.0f - fpos_next) * st->buf[1][ipos_next] + fpos_next * st->buf[1][ipos_next+1<st->size?ipos_next+1:0]);
#endif

    // write stock
    if (st->ipos != ipos_next) {
      if (speed[i] > 0.0f) {
	ipos_nexttmp = ipos_next > st->ipos ? ipos_next : ipos_next + st->size;
	if (note[i] > 0.0f) {
	  norm = 1.0f / (ipos_nexttmp - st->ipos + fpos_next - st->fpos);
	  for (j=st->ipos+1; j<ipos_nexttmp; j++) {
	    st->buf[0][j<st->size?j:j-st->size] *= st->feedbackgain;
	    st->buf[1][j<st->size?j:j-st->size] *= st->feedbackgain;
	    st->buf[0][j<st->size?j:j-st->size] += norm * ((j - st->ipos - st->fpos) * src[0][i] * note[i] + (ipos_nexttmp - j + fpos_next) * st->rec[0]);
	    st->buf[1][j<st->size?j:j-st->size] += norm * ((j - st->ipos - st->fpos) * src[1][i] * note[i] + (ipos_nexttmp - j + fpos_next) * st->rec[1]);
	  }
	  st->writestock[0] = norm * ((ipos_nexttmp - st->ipos - st->fpos) * src[0][i] * note[i] + fpos_next * st->rec[0]);
	  st->writestock[1] = norm * ((ipos_nexttmp - st->ipos - st->fpos) * src[1][i] * note[i] + fpos_next * st->rec[1]);
	}
	st->idxstock = ipos_nexttmp < st->size ? ipos_nexttmp : ipos_nexttmp - st->size;
      }
      else if (speed[i] < 0.0f) {
	ipos_curtmp = ipos_next < st->ipos ? st->ipos : st->ipos + st->size;
	if (note[i] > 0.0f) {
	  norm = 1.0f / (ipos_curtmp - ipos_next + st->fpos - fpos_next);
	  for (j=ipos_curtmp; j>ipos_next+1; j--) {
	    st->buf[0][j<st->size?j:j-st->size] *= st->feedbackgain;
	    st->buf[1][j<st->size?j:j-st->size] *= st->feedbackgain;
	    st->buf[0][j<st->size?j:j-st->size] += norm * ((ipos_curtmp - j + st->fpos) * src[0][i] * note[i] + (j - ipos_next - fpos_next) * st->rec[0]);
	    st->buf[1][j<st->size?j:j-st->size] += norm * ((ipos_curtmp - j + st->fpos) * src[1][i] * note[i] + (j - ipos_next - fpos_next) * st->rec[1]);
	  }
	  st->writestock[0] = norm * ((ipos_curtmp - ipos_next - 1 + st->fpos) * src[0][i] * note[i] + (1.0f - fpos_next) * st->rec[0]);
	  st->writestock[1] = norm * ((ipos_curtmp - ipos_next - 1 + st->fpos) * src[1][i] * note[i] + (1.0f - fpos_next) * st->rec[1]);
	}
	st->idxstock = ipos_next+1 < st->size ? ipos_next+1 : ipos_next+1-st->size;
      }
    }
    st->rec[0] = src[0][i] * note[i];
    st->rec[1] = src[1][i] * note[i];

    // prepare for next
    st->ipos = ipos_next;
    st->fpos = fpos_next;

    dst[0][i] = out[0];
    dst[1][i] = out[1];
  }
  st->position[0] = 10.f * RECIP(30.48f);
  st->position[1] = (st->ipos + st->fpos) / st->size;
}
