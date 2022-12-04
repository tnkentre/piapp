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

#if 0
typedef struct {
  uint8_t note;
  int turntableid;
  int lenid;
  int vsbid;
} NOTE_ALLOCATION;

const NOTE_ALLOCATION note_alloc[] = {
  /* note tt    lenid       vsbid*/
  //  {    48, 0, LENID_16BEAT,   0},
  //  {    49, 0, LENID_16BEAT,   1},
  //  {    50, 0, LENID_8BEAT,    0},
  //  {    51, 0, LENID_8BEAT,    1},
  //  {    52, 0, LENID_4BEAT,    0},
  //  {    53, 0, LENID_2BEAT,    0},
  //  {    54, 0, LENID_2BEAT,    1},
  {    55, 0, LENID_1BAR,     0},
  {    56, 0, LENID_1BAR,     1},
  {    57, 0, LENID_2BAR,     0},
  {    58, 0, LENID_2BAR,     1},
  //  {    59, 0, LENID_4BAR,     0},
  //  {    60, 0, LENID_16BEAT,   0},
  //  {    61, 0, LENID_16BEAT,   1},
  //  {    62, 0, LENID_8BEAT,    0},
  //  {    63, 0, LENID_8BEAT,    1},
  //  {    64, 0, LENID_4BEAT,    0},
  //  {    65, 0, LENID_2BEAT,    0},
  //  {    66, 0, LENID_2BEAT,    1},
  {    67, 0, LENID_1BAR,     0},
  {    68, 0, LENID_1BAR,     1},
  {    69, 0, LENID_2BAR,     0},
  {    70, 0, LENID_2BAR,     1},
  //  {    71, 0, LENID_4BAR,     0},
};

#endif

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

  /* MIDI FWK (JACK interface) */
  MidiFwkState* midifwk;

  // Task
  Environment_t env[NTURNTABLE];
  char  tskName[NTURNTABLE][32];
  MBX_Handle mbx[NTURNTABLE];
  SEM_Handle sem[NTURNTABLE];
  TSK_Handle tsk[NTURNTABLE];
  TaskDescriptor_t tskDescriptor[NTURNTABLE];
  
} AudioProcState;

/* The instance */
static AudioProcState* audioproc_st = NULL;

/* command format of Mailbox */
typedef struct {
  float** out;
  float** in;
  float** tc;
} cmd_desc_t;

/*----------------------------------
    Task of AudioFwk
 -----------------------------------*/
static void
process_task(void* arg1)
{
  cmd_desc_t cmd;
  AudioProcState *st = audioproc_st;
  int ch = (int)arg1;

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(ch, &cpu_set);
  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);
  
  TRACE(LEVEL_INFO, "Task AudioProc%d started on CPU %d", ch, sched_getcpu());
  
  InitEnvironment(&st->env[ch], 128*1024, __func__);

  do {
    MBX_pend(st->mbx[ch], &cmd, SYS_FOREVER);

    // Not needed because not a real time task
    Task_startNewCycle();

    Turntable_proc(st->turntable[ch], cmd.out, cmd.in, cmd.tc);

    SEM_postBinary(st->sem[ch]);
  } while(1);
}


static void AudioProc(float * out[], float* in[])
{
  int i;
  AudioProcState* st = audioproc_st;

  for (i=0; i<NTURNTABLE; i++) {
    cmd_desc_t cmd;
    if (i==0) {
      cmd.out = &out[CH_OUT_AUD0L];
      cmd.in  = &in[CH_IN_AUDL];
      cmd.tc  = &in[CH_IN_TC0L];
    }
    else {
      cmd.out = &out[CH_OUT_AUD1L];
      cmd.in  = &in[CH_IN_AUDL];
      cmd.tc  = &in[CH_IN_TC1L];
    }
    MBX_post(st->mbx[i], &cmd, 0);
  }
  for (i=0; i<NTURNTABLE; i++) {
    SEM_pendBinary(st->sem[i], SYS_FOREVER);
  }
  vec_add(out[CH_OUT_MIXL], out[CH_OUT_AUD0L], out[CH_OUT_AUD1L], st->frame_size);
  vec_add(out[CH_OUT_MIXR], out[CH_OUT_AUD0R], out[CH_OUT_AUD1R], st->frame_size);
}

void MidiProc(uint64_t time, uint8_t type, uint8_t channel, uint8_t param, uint8_t value)
{
#if 0
  int i;
  AudioProcState* st = audioproc_st;
  if (type==0x90 || type==0x80) {
    /* find ids */
    for (i=0; i<NBELEM(note_alloc); i++) {
      if (note_alloc[i].note == param) {
	if (type==0x90)
	  st->note_state[note_alloc[i].turntableid][note_alloc[i].lenid][note_alloc[i].vsbid] = 1;
	else
	  st->note_state[note_alloc[i].turntableid][note_alloc[i].lenid][note_alloc[i].vsbid] = 0;
	break;
      }
    }
  }
#endif
#if 1
  printf("MIDI:%llu: type=0x%02X, channel=%d, param=%d, value=%d\n",
	 time, type, channel, param, value);
#endif
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

  /* Create task to audio process */
  for (i=0; i<NTURNTABLE; i++) {
    st->mbx[i] = MBX_create(sizeof(cmd_desc_t), 1, NULL);
    st->sem[i] = SEM_create(1, NULL);
    sprintf(st->tskName[i], "TaskAudioProcess%d", i);
    st->tskDescriptor[i].h        = &st->tsk[i];
    st->tskDescriptor[i].f        = (Fxn)process_task;
    st->tskDescriptor[i].name     = st->tskName[i];
    st->tskDescriptor[i].stack    = 5*1024;
    st->tskDescriptor[i].priority = 10;
    st->tskDescriptor[i].arg1     = (void*)i;
    Task_init(&st->tskDescriptor[i], 1);
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
