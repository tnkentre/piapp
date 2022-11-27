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
#include "FBvsb.h"
#include "AudioProc.h"

/*
 * AEC functions
 */
#define FS           (48000)
//#define FRAME_SIZE   (FS/100)
#define FRAME_SIZE   (512)
#define NTURNTABLE   (2)

typedef enum {
  CH_IN_AUDL = 0,
  CH_IN_AUDR,
  CH_IN_TC0L,
  CH_IN_TC0R,
  CH_IN_TC1L,
  CH_IN_TC1R,
  CH_IN_NUM
} CH_IN;

typedef enum {
  CH_OUT_MIXL = 0,
  CH_OUT_MIXR,
  CH_OUT_AUD0L,
  CH_OUT_AUD0R,
  CH_OUT_AUD1L,
  CH_OUT_AUD1R,
  CH_OUT_NUM
} CH_OUT;

/* AudioProc State */
typedef struct {
  /* Configuration */
  int fs;
  int frame_size;
  int nch_input;
  int nch_output;

  /* AC states */
  TCANALYSIS_State * tcana[NTURNTABLE];
  VSB_State        * vsb[NTURNTABLE];
  FBVSB_State      * fbvsb[NTURNTABLE];

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
  int frame_size = st->frame_size;

  VARDECLR(float, speed_L);
  VARDECLR(float, speed_R);
  SAVE_STACK;
  ALLOC(speed_L, frame_size, float);
  ALLOC(speed_R, frame_size, float);

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana[0], speed_L, in[CH_IN_TC0L], in[CH_IN_TC0R], frame_size);
  tcanalysis_process(st->tcana[1], speed_R, in[CH_IN_TC1L], in[CH_IN_TC1R], frame_size);

  /* Turntable on left side */
#if 0
  vsb_process(st->vsb[0], &out[CH_OUT_AUD0L], &in[CH_IN_AUDL], speed_L, frame_size);
  vsb_process(st->vsb[1], &out[CH_OUT_AUD1L], &in[CH_IN_AUDL], speed_R, frame_size);
#else
  FBvsb_process(st->fbvsb[0], &out[CH_OUT_AUD0L], &in[CH_IN_AUDL], speed_L);
  FBvsb_process(st->fbvsb[1], &out[CH_OUT_AUD1L], &in[CH_IN_AUDL], speed_R);
#endif

  vec_add(out[CH_OUT_MIXL], out[CH_OUT_AUD0L], out[CH_OUT_AUD0L], frame_size);
  vec_add(out[CH_OUT_MIXR], out[CH_OUT_AUD0R], out[CH_OUT_AUD0R], frame_size);
  
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

  st->fs         = FS;
  st->frame_size = FRAME_SIZE;
  st->nch_input  = CH_IN_NUM;
  st->nch_output = CH_OUT_NUM;

  for (ch=0; ch<NTURNTABLE; ch++) {
    sprintf(name, "tcana%d",ch);
    st->tcana[ch] = tcanalysis_init(name, FS, 1000.f);
    sprintf(name, "vsb%d",ch);
    st->vsb[ch] = vsb_init(name, (int)(FS * 60.f / 33.3f));
    sprintf(name, "fbvsb%d",ch);
    st->fbvsb[ch] = FBvsb_init(name, 2, st->fs, st->frame_size, 60.f / 33.3f);
  }

  /* Initialize and start AudioFwk */
  st->audfwk = audiofwk_init("AudioProc", st->nch_input, st->nch_output, st->frame_size, AudioProc);
  st->midifwk = midifwk_init("MidiProc", MidiProc);
  midifwk_connect_midiin(st->midifwk, "Nocturn Keyboard");
}

void AudioProc_close(void)
{
  AudioProcState* st = audioproc_st;
  audiofwk_close(st->audfwk);
  midifwk_close(st->midifwk);
}
