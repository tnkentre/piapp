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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include "RevoRT.h"
#include "FixedHeap.h"
#include "vectors.h"
#include "cli.h"
#include "AudioFwk.h"
#include "MidiFwk.h"
#include "turntable.h"
#include "oscac.h"
#include "AudioProc.h"

AC4_MODULE;

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

typedef struct {
  AC4_HEADER;

  /* Configuration */
  int fs;
  int frame_size;
  int nch_input;
  int nch_output;
  //  int note_state[NTURNTABLE][LENID_NUM][NVSB];
  
  /* parameters */
  float sync0;
  float sync1;
  int source0;
  int source1;

  /* AC states */
  TurntableState* turntable[NTURNTABLE];

  /* Audio FWK (JACK interface) */
  AudioFwkState* audfwk;

  /* OSCAC FWK */
  OSCAC_State* oscac;
  
} AudioProcState;

static const RegDef_t rd[] = {
  AC_REGDEF(sync0,     CLI_ACFPTM,   AudioProcState, "sync0"),
  AC_REGDEF(sync1,     CLI_ACFPTM,   AudioProcState, "sync1"),
  AC_REGDEF(source0,   CLI_ACIPTM,   AudioProcState, "source0"),
  AC_REGDEF(source1,   CLI_ACIPTM,   AudioProcState, "source1"),
};

/* The instance */
static AudioProcState* audioproc_st = NULL;

static void AudioProc(float * out[], float* in[])
{
  AudioProcState* st = audioproc_st;

  float ** src;
  float ** tc;

  if (!st->sync0)   tc = &in[CH_IN_TC0L];
  else              tc = &in[CH_IN_TC1L];
  if (!st->source0) src = &in[CH_IN_TC0L];
  else              src = &in[CH_IN_AUDL];
  Turntable_proc(st->turntable[0], &out[CH_OUT_AUD0L], src, tc);

  if (!st->sync1)   tc = &in[CH_IN_TC1L];
  else              tc = &in[CH_IN_TC0L];
  if (!st->source1) src = &in[CH_IN_TC1L];
  else              src = &in[CH_IN_AUDL];
  Turntable_proc(st->turntable[1], &out[CH_OUT_AUD1L], src, tc);

  vec_add(out[CH_OUT_MIXL], out[CH_OUT_AUD0L], out[CH_OUT_AUD1L], st->frame_size);
  vec_add(out[CH_OUT_MIXR], out[CH_OUT_AUD0R], out[CH_OUT_AUD1R], st->frame_size);
}

void AudioProc_init(void)
{
  int i;
  AudioProcState* st;
  char name[32];
  st = MEM_ALLOC(MEM_SDRAM, AudioProcState, 1, 4);
  audioproc_st = st;

  AC4_ADD("top", st, "AudioProc");
  AC4_ADD_REG(rd);

  st->fs         = FS;
  st->frame_size = FRAME_SIZE;
  st->nch_input  = CH_IN_NUM;
  st->nch_output = CH_OUT_NUM;

  st->sync0 = 0.f;
  st->sync1 = 0.f;
  st->source0 = 1;
  st->source1 = 1;

  for (i=0; i<NTURNTABLE; i++) {
    sprintf(name, "tt%d",i);
    st->turntable[i] = Turntable_init(name, FS, FRAME_SIZE);
  }

  /* OSC */
  st->oscac = oscac_init("OSCAC", OSCAC_INITMODE_FULLACCESS);
  oscac_start(st->oscac);

  for (i=0; i<NTURNTABLE; i++) {
    char str[32];
    sprintf(str, "sync%d",i);   oscac_add_acreg(st->oscac, "top", str);
    sprintf(str, "source%d",i); oscac_add_acreg(st->oscac, "top", str);
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "igain");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "recgain");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "overdub");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "fdbal");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "monitor");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "looplen");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "looppos");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "vinyl");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "loopstart");
    sprintf(str, "tt%d", i);    oscac_add_acreg(st->oscac, str, "loopend");
  }
  oscac_set_moninterval(st->oscac, 20);

  /* Initialize and start AudioFwk */
  st->audfwk = audiofwk_init("AudioProc", st->nch_input, st->nch_output, st->frame_size, AudioProc);
}

void AudioProc_close(void)
{
  AudioProcState* st = audioproc_st;
  audiofwk_close(st->audfwk);
  oscac_stop(st->oscac);
}
