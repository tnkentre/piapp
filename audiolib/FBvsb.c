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
 * @file FBvsb.c
 * @brief This file implements a variable speed buffer module.
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#include <float.h>
#include "RevoRT.h"
#include "RevoRT_complex.h"
#include "Registers.h"
#include "FixedHeap.h"
#include "vectors.h"
#include "vectors_cplx.h"
#include "cli.h"
#include "FB.h"
#include "FB_filters.h"
#include "RevoFFT.h"
#include "FBvsb.h"

/************************************
 *    Implementation of SimpleCB    *
 ************************************/
typedef struct {
  int len;
  int idx;
  float * restrict buf;
} SimpleCB;

static SimpleCB* simplecb_init(int len)
{
  SimpleCB* st;
  st = MEM_ALLOC(MEM_SDRAM, SimpleCB, 1, 4);
  st->len = len;
  st->idx = 0;
  st->buf = MEM_ALLOC(MEM_SDRAM, float, len, 8);
  return st;
}

static void simplecb_write(SimpleCB* restrict st, const float * buf, int len)
{
  int writelen;
  int srcidx = 0;
  while (len) {
    writelen = MIN(st->len - st->idx, len);
    COPY(&st->buf[st->idx], &buf[srcidx], writelen);
    len     -= writelen;
    srcidx  += writelen;
    st->idx += writelen;
    if (st->idx >= st->len) {
      st->idx -= st->len;
    }
  }
}

static void simplecb_read(SimpleCB* restrict st, float * restrict buf, int len, int delay)
{
  int readlen;
  int srcidx = ((1 + (len+delay) / st->len) * st->len + st->idx - (len+delay)) % st->len;
  int dstidx = 0;
  while (len) {
    readlen = MIN(st->len - srcidx, len);
    COPY(&buf[dstidx], &st->buf[srcidx], readlen);
    len     -= readlen;
    dstidx  += readlen;
    srcidx  += readlen;
    if (srcidx >= st->len) {
      srcidx -= st->len;
    }
  }
}

/************************************
 *    Implementation of FBVSB       *
 ************************************/
struct FBVSB_State_ {
  AC_HEADER(0,0);  
  /* configuration and history */
  int    fs;
  int    frame_size;
  int    fft_size;
  int    fbfilter_size;
  int    nband;
  int    nfbbuf;

  rv_complex ** fbbuf[2];
  rv_complex ** phbuf[2];
  SimpleCB   *  tdbuf[2];
  float      *  ha;
  float      *  hs;
  int    ipos;
  float  fpos;

  /* Audio Components */
  RevoFFTState * restrict fft;
  
  /* parameters */
  float loopgain;
  float feedbackgain;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(loopgain,     CLI_ACFPTM,   FBVSB_State, "Loopback gain"),
  AC_REGDEF(feedbackgain, CLI_ACFPTM,   FBVSB_State, "Feedback gain"),
};

FBVSB_State *FBvsb_init(const char * name, int fs, int frame_size, float length_sec)
{
  int ch, i;
  FBVSB_State* st;
  const FB_filters_info * fbinfo;

  int fft_size      = 2 * frame_size;
  int fbfilter_size = 2 * fft_size;
  int nband         = fft_size / 2;
  
  st = MEM_ALLOC(MEM_SDRAM, FBVSB_State, 1, 8);

  AC_ADD(name, CLI_RLSL_FE, st, "FBVSB");
  AC_ADD_REG(rd, CLI_RLSL_FE);

  st->fs            = fs;
  st->frame_size    = frame_size;
  st->fft_size      = fft_size;
  st->fbfilter_size = fbfilter_size;
  st->nband         = nband;
  st->nfbbuf        = (int)(length_sec * fs / frame_size);
  for (ch = 0; ch < 2; ch++) {
    st->fbbuf[ch] = MEM_ALLOC(MEM_SDRAM, rv_complex*, st->nfbbuf, 8);
    st->phbuf[ch] = MEM_ALLOC(MEM_SDRAM, rv_complex*, st->nfbbuf, 8);
    for (i = 0; i < st->nfbbuf; i++) {
      st->fbbuf[ch][i] = MEM_ALLOC(MEM_SDRAM, rv_complex, nband+1, 8);
      st->phbuf[ch][i] = MEM_ALLOC(MEM_SDRAM, rv_complex, nband+1, 8);
    }
    st->tdbuf[ch] = simplecb_init(fbfilter_size + frame_size);
  }

  fbinfo = FB_filters_getinfo(frame_size, fft_size, fbfilter_size);
  st->ha = MEM_ALLOC(MEM_SDRAM, float, fbfilter_size, 8);
  st->hs = MEM_ALLOC(MEM_SDRAM, float, fbfilter_size, 8);
  COPY(st->ha, fbinfo->filter_h, fbfilter_size);
  COPY(st->hs, fbinfo->filter_g, fbfilter_size);

  st->fft           = RevoFFT_initr(fft_size);
  
  st->ipos          = 0;
  st->fpos          = 0.0f;
  st->loopgain      = 1.0f;
  st->feedbackgain  = 1.0f;
	
  return st;
}

static void proc_put(FBVSB_State * restrict st, float* src[], float* speed)
{
  int i,m,ch;
  int frame_size    = st->frame_size;
  int fbfilter_size = st->fbfilter_size;
  int fft_size      = st->fft_size;
  int nband         = st->nband;
  int cur           = (int)(st->fpos / frame_size);
  int putlen        = 0;

  VARDECLR(float,      fftbuf);
  VARDECLR(rv_complex, fdtemp0);
  VARDECLR(rv_complex, fdtemp1);
  VARDECLR(float,      sctemp0);
  VARDECLR(float,      sctemp1);
  SAVE_STACK;
  ALLOC(fftbuf,  fbfilter_size, float);
  ALLOC(fdtemp0, nband+1,  rv_complex);
  ALLOC(fdtemp1, nband+1,  rv_complex);
  ALLOC(sctemp0, nband+1,  float);
  ALLOC(sctemp1, nband+1,  float);
  
  for (i=0; i<frame_size; i++) {
    st->fpos += speed[i];
    putlen++;
    if ((int)(st->fpos / frame_size) != cur) {
      for (ch=0; ch<2; ch++) {
	simplecb_write(st->tdbuf[ch], &src[ch][i-putlen+1], putlen);
	// FB analysis
	simplecb_read(st->tdbuf[ch], fftbuf, fbfilter_size, 0);
	vec_mul1(fftbuf, st->ha, fbfilter_size);
	for (m=0; m<fbfilter_size/fft_size-1; m++) {
	  vec_add1(fftbuf, &fftbuf[(m+1)*fft_size], fft_size);
	}
	RevoFFT_fftr(st->fft, (float*)st->fbbuf[ch][cur], fftbuf);
	// phase analysis
	simplecb_read(st->tdbuf[ch], fftbuf, fbfilter_size, frame_size);
	vec_mul1(fftbuf, st->ha, fbfilter_size);
	for (m=0; m<fbfilter_size/fft_size-1; m++) {
	  vec_add1(fftbuf, &fftbuf[(m+1)*fft_size], fft_size);
	}
	RevoFFT_fftr(st->fft, (float*)fdtemp0, fftbuf);
	vec_cplx_mul_conj(fdtemp1, fdtemp0, st->fbbuf[ch][cur], nband);
	vec_spectral_power(sctemp0, fdtemp1, nband);
	vec_sqrt(sctemp1, sctemp0, nband);
	vec_add1s(sctemp1, FLT_MIN, nband);
	vec_inv(sctemp0, sctemp1, nband);
	vec_cplx_mulr(st->phbuf[ch][cur], fdtemp1, sctemp0, nband);
	// !!todo!! adjust fractional delay
      }
      putlen = 0;
      cur = (int)(st->fpos / frame_size);
      if (cur >= st->nfbbuf) {
	st->fpos -= st->nfbbuf * frame_size;
	cur = (int)(st->fpos / frame_size);
      }
      if (cur < 0) {
	st->fpos += st->nfbbuf * frame_size;
	cur = (int)(st->fpos / frame_size);
      }
    }
  }
  for (ch=0; ch<2; ch++) {
    simplecb_write(st->tdbuf[ch], &src[ch][i-putlen+1], putlen);
  }

  RESTORE_STACK;
}

void FBvsb_process(FBVSB_State * restrict st, float* dst[], float* src[], float* speed, int len)
{
#if 0
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
    out[0] = src[0][i] + st->loopgain * ((1.0f - fpos_next) * st->buf[0][ipos_next] + fpos_next * st->buf[0][ipos_next+1<st->size?ipos_next+1:0]);
    out[1] = src[1][i] + st->loopgain * ((1.0f - fpos_next) * st->buf[1][ipos_next] + fpos_next * st->buf[1][ipos_next+1<st->size?ipos_next+1:0]);

    // write stock
    if (st->ipos != ipos_next) {
      if (speed[i] > 0.0f) {
	ipos_nexttmp = ipos_next > st->ipos ? ipos_next : ipos_next + st->size;
	norm = 1.0f / (ipos_nexttmp - st->ipos + fpos_next - st->fpos);
	for (j=st->ipos+1; j<ipos_nexttmp; j++) {
	  st->buf[0][j<st->size?j:j-st->size] *= st->feedbackgain;
	  st->buf[1][j<st->size?j:j-st->size] *= st->feedbackgain;
	  st->buf[0][j<st->size?j:j-st->size] += norm * ((j - st->ipos - st->fpos) * src[0][i] + (ipos_nexttmp - j + fpos_next) * st->rec[0]);
	  st->buf[1][j<st->size?j:j-st->size] += norm * ((j - st->ipos - st->fpos) * src[1][i] + (ipos_nexttmp - j + fpos_next) * st->rec[1]);
	}
	st->writestock[0] = norm * ((ipos_nexttmp - st->ipos - st->fpos) * src[0][i] + fpos_next * st->rec[0]);
	st->writestock[1] = norm * ((ipos_nexttmp - st->ipos - st->fpos) * src[1][i] + fpos_next * st->rec[1]);
	st->idxstock = ipos_nexttmp < st->size ? ipos_nexttmp : ipos_nexttmp - st->size;
      }
      else if (speed[i] < 0.0f) {
	ipos_curtmp = ipos_next < st->ipos ? st->ipos : st->ipos + st->size;
	norm = 1.0f / (ipos_curtmp - ipos_next + st->fpos - fpos_next);
	for (j=ipos_curtmp; j>ipos_next+1; j--) {
	  st->buf[0][j<st->size?j:j-st->size] *= st->feedbackgain;
	  st->buf[1][j<st->size?j:j-st->size] *= st->feedbackgain;
	  st->buf[0][j<st->size?j:j-st->size] += norm * ((ipos_curtmp - j + st->fpos) * src[0][i] + (j - ipos_next - fpos_next) * st->rec[0]);
	  st->buf[1][j<st->size?j:j-st->size] += norm * ((ipos_curtmp - j + st->fpos) * src[1][i] + (j - ipos_next - fpos_next) * st->rec[1]);
	}
	st->writestock[0] = norm * ((ipos_curtmp - ipos_next - 1 + st->fpos) * src[0][i] + (1.0f - fpos_next) * st->rec[0]);
	st->writestock[1] = norm * ((ipos_curtmp - ipos_next - 1 + st->fpos) * src[1][i] + (1.0f - fpos_next) * st->rec[1]);
	st->idxstock = ipos_next+1 < st->size ? ipos_next+1 : ipos_next+1-st->size;
      }
    }
    st->rec[0] = src[0][i];
    st->rec[1] = src[1][i];

    // prepare for next
    st->ipos = ipos_next;
    st->fpos = fpos_next;

    dst[0][i] = out[0];
    dst[1][i] = out[1];
  }
#endif
}
