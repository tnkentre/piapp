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
  /* Configuration */
  int fs;
  int frame_size;
  int nch_input;
  int nch_output;
  //  int note_state[NTURNTABLE][LENID_NUM][NVSB];
  
  /* AC states */
  TurntableState* turntable[NTURNTABLE];

  /* Audio FWK (JACK interface) */
  AudioFwkState* audfwk;

  /* OSCAC FWK */
  OSCAC_State* oscac;
  
} AudioProcState;

/* The instance */
static AudioProcState* audioproc_st = NULL;

static void AudioProc(float * out[], float* in[])
{
  int i;
  AudioProcState* st = audioproc_st;

  for (i=0; i<NTURNTABLE; i++) {
    Turntable_proc(st->turntable[i], &out[CH_OUT_AUD0L+2*i], &in[CH_IN_AUDL], &in[CH_IN_TC0L+2*i]);
  }

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

  st->fs         = FS;
  st->frame_size = FRAME_SIZE;
  st->nch_input  = CH_IN_NUM;
  st->nch_output = CH_OUT_NUM;

  for (i=0; i<NTURNTABLE; i++) {
    sprintf(name, "tt%d",i);
    st->turntable[i] = Turntable_init(name, FS, FRAME_SIZE);
  }

  /* OSC */
  st->oscac = oscac_init("OSCAC", OSCAC_INITMODE_FULLACCESS);
  oscac_start(st->oscac);
  oscac_add_acreg(st->oscac, "tt0_tdvsb0", "position");
  oscac_add_acreg(st->oscac, "tt1_tdvsb0", "position");
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
