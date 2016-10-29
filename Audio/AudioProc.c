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

/* AudioProc State */
typedef struct {
  /* Configuration */
  int frame_size;
  int nch_input;
  int nch_output;

  /* AC states */
  TCANALYSIS_State * tcana_L;
  TCANALYSIS_State * tcana_R;
  VSB_State * vsb_L;
  VSB_State * vsb_R;

  /* Task handler */
  char             tskName[32];
  MBX_Handle       mbx;
  SEM_Handle       sem;
  TSK_Handle       tsk;
  TaskDescriptor_t tskDescriptor;  
  
  /* Audio FWK (JACK interface) */
  AudioFwkState*   audfwk;

} AudioProcState;

/* command format of Mailbox */
typedef struct {
  float ** input_buffers;
  float ** output_buffers;
} cmd_desc_t;

/* The instance */
static AudioProcState* audioproc_st = NULL;

static void proc(AudioProcState* st, float * out[], float* in[])
{
  int ch;
  int frame_size = st->frame_size;
  int nch_input  = st->nch_input;
  int nch_output = st->nch_output;

  VARDECLR(float, speed_L);
  VARDECLR(float, speed_R);
  SAVE_STACK;
  ALLOC(speed_L, frame_size, float);
  ALLOC(speed_R, frame_size, float);
  
  for (ch=0; ch<MIN(nch_input, nch_output); ch++) {
    COPY(out[ch], in[ch], frame_size);
  }
  for (; ch<nch_output; ch++) {
    vec_set(out[ch], 0.f, frame_size);
  }

  tcanalysis_process(st->tcana_L, speed_L, in[2], in[3], frame_size);
  tcanalysis_process(st->tcana_R, speed_R, in[4], in[5], frame_size);
  vsb_process(st->vsb_L, &out[2], &in[0], speed_L, frame_size);  
  vsb_process(st->vsb_R, &out[4], &in[0], speed_R, frame_size);

  RESTORE_STACK;
  
}

/*----------------------------------
    Task of AudioProc
 -----------------------------------*/
static void TaskAudioProc(void* arg1)
{
  static Environment_t env;
  cmd_desc_t cmd;
  //  AudioProcState *st = (AudioProcState*)arg1;
  AudioProcState *st = audioproc_st;

  TRACE(LEVEL_INFO, "Task AudioProc started on CPU %d", sched_getcpu());
  
  InitEnvironment(&env, 128*1024, __func__);

  do {
    MBX_pend(st->mbx, &cmd, SYS_FOREVER);

    // Not needed because not a real time task
    Task_startNewCycle();

    proc(st, cmd.output_buffers, cmd.input_buffers);

  } while(1);
}

void AudioProc(float* out[], float* in[])
{
  AudioProcState* st = audioproc_st;
  cmd_desc_t cmd;
  cmd.input_buffers = in;
  cmd.output_buffers = out;
  MBX_post(st->mbx, &cmd, 0);
}

void AudioProc_init(void)
{
  AudioProcState* st;
  st = MEM_ALLOC(MEM_SDRAM, AudioProcState, 1, 4);
  audioproc_st = st;

  st->frame_size = FRAME_SIZE;
  st->nch_input  = NCH_INPUT;
  st->nch_output = NCH_OUTPUT;


  st->tcana_L = tcanalysis_init("tcana_L", FS, 1000.f);
  st->tcana_R = tcanalysis_init("tcana_R", FS, 1000.f);
  st->vsb_L = vsb_init("vsb_L", (int)(FS * 60.f / 33.3f));
  st->vsb_R = vsb_init("vsb_R", (int)(FS * 60.f / 33.3f));
  
  /* initialize mailbox */
  st->mbx = MBX_create(sizeof(cmd_desc_t), 1, NULL);
  /* initialize semapho */
  st->sem = SEM_create(1, NULL);
  /* initialize task */
  sprintf(st->tskName, "TaskAudioProc");
  st->tskDescriptor.h        = &st->tsk;
  st->tskDescriptor.f        = (Fxn)TaskAudioProc;
  st->tskDescriptor.name     = st->tskName;
  st->tskDescriptor.stack    = 5*1024;
  st->tskDescriptor.priority = 10;
  st->tskDescriptor.arg1     = st;
  Task_init(&st->tskDescriptor, 1);
   
  /* Initialize and start AudioFwk */
  st->audfwk = audiofwk_init("AudioProc", st->nch_input, st->nch_output, st->frame_size, AudioProc);
}

void AudioProc_close(void)
{
  AudioProcState* st = audioproc_st;
  audiofwk_close(st->audfwk);
}
