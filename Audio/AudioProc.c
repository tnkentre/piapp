/*******************************************************************************
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
 ******************************************************************************/

/**
 * @file AudioProc.c
 * @brief Audio processing
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */
#define _GNU_SOURCE  
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include "RevoRT.h"
#include "FixedHeap.h"
#include "vectors.h"
#include "cli.h"
#include "AudioFwk.h"
#include "MidiFwk.h"
#include "FB.h"
#include "FB_filters.h"
#include "tcanalysis.h"
#include "vsb.h"
#include "AudioProc.h"

/*
 * AEC functions
 */
#define FS           (48000)
#define FRAME_SIZE   (FS/100)
#define NCH_INPUT    (6)
#define NCH_OUTPUT   (6)
#define NLAYER       (8)

/* AudioProc State */
typedef struct {
  /* Configuration */
  int frame_size;
  int nch_input;
  int nch_output;

  /* Parameters */
  int ActiveLayer_L;
  int ActiveLayer_R;

  /* AC states */
  TCANALYSIS_State * tcana_L;
  TCANALYSIS_State * tcana_R;
  VSB_State * vsb_L[NLAYER];
  VSB_State * vsb_R[NLAYER];

  /* Audio FWK (JACK interface) */
  AudioFwkState*   audfwk;

  /* MIDI FWK (JACK interface) */
  MidiFwkState*   midifwk;

} AudioProcState;

/* The instance */
static AudioProcState* audioproc_st = NULL;

static void AudioProc(float * out[], float* in[])
{
  AudioProcState* st = audioproc_st;
  int ch;
  int frame_size = st->frame_size;
  int nch_input  = st->nch_input;
  int nch_output = st->nch_output;
  float* tmp[2];
  float* blank[2];

  VARDECLR(float, speed_L);
  VARDECLR(float, speed_R);
  VARDECLR(float, tmpL);
  VARDECLR(float, tmpR);
  VARDECLR(float, zeros);
  SAVE_STACK;
  ALLOC(speed_L, frame_size, float);
  ALLOC(speed_R, frame_size, float);
  ALLOC(tmpL,    frame_size, float);
  ALLOC(tmpR,    frame_size, float);
  ALLOC(zeros,   frame_size, float);

  tmp[0]   = tmpL;
  tmp[1]   = tmpR;
  blank[0] = zeros;
  blank[1] = zeros;  
  vec_set(zeros, 0.f, frame_size);
  
  for (ch=0; ch<MIN(nch_input, nch_output); ch++) {
    COPY(out[ch], in[ch], frame_size);
  }
  for (; ch<nch_output; ch++) {
    vec_set(out[ch], 0.f, frame_size);
  }

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana_L, speed_L, in[2], in[3], frame_size);
  tcanalysis_process(st->tcana_R, speed_R, in[4], in[5], frame_size);

  /* Turntable on left side */
  vec_set(out[2], 0.f, frame_size);
  vec_set(out[3], 0.f, frame_size);
  for (ch=0; ch<NLAYER; ch++) {
    if (ch == st->ActiveLayer_L)
      vsb_process(st->vsb_L[ch], tmp, &in[0], speed_L, frame_size);
    else
      vsb_process(st->vsb_L[ch], tmp, blank,  speed_L, frame_size);
    vec_add1(out[2], tmp[0], frame_size);
    vec_add1(out[3], tmp[1], frame_size);
  }
    
  vec_set(out[4], 0.f, frame_size);
  vec_set(out[5], 0.f, frame_size);
  for (ch=0; ch<NLAYER; ch++) {
    /* Turntable on right side */
    if (ch == st->ActiveLayer_R)
      vsb_process(st->vsb_R[ch], tmp, &in[0], speed_R, frame_size);
    else
      vsb_process(st->vsb_R[ch], tmp, blank,  speed_R, frame_size);
    vec_add1(out[4], tmp[0], frame_size);
    vec_add1(out[5], tmp[1], frame_size);
  }

  RESTORE_STACK;
  
}

void MidiProc(uint64_t time, uint8_t type, uint8_t channel, uint8_t param, uint8_t value)
{
  printf("MIDI:%llu: type=0x%02X, channel=%d, param=%d, value=%d\n",
	 time, type, channel, param, value);
}

void AudioProc_init(void)
{
  int ch;
  AudioProcState* st;
  char name[32];
  st = MEM_ALLOC(MEM_SDRAM, AudioProcState, 1, 4);
  audioproc_st = st;

  st->frame_size = FRAME_SIZE;
  st->nch_input  = NCH_INPUT;
  st->nch_output = NCH_OUTPUT;

  st->tcana_L = tcanalysis_init("tcana_L", FS, 1000.f);
  st->tcana_R = tcanalysis_init("tcana_R", FS, 1000.f);
  for (ch=0; ch<NLAYER; ch++) {
    sprintf(name, "vsb_L%d", ch);
    st->vsb_L[ch] = vsb_init(name, (int)(FS * 60.f / 33.3f));
    sprintf(name, "vsb_R%d", ch);
    st->vsb_R[ch] = vsb_init(name, (int)(FS * 60.f / 33.3f));
  }
     
  /* Initialize and start AudioFwk */
  st->audfwk = audiofwk_init("AudioProc", st->nch_input, st->nch_output, st->frame_size, AudioProc);
  st->midifwk = midifwk_init("MidiProc", MidiProc);
}

void AudioProc_close(void)
{
  AudioProcState* st = audioproc_st;
  audiofwk_close(st->audfwk);
  midifwk_close(st->midifwk);
}
