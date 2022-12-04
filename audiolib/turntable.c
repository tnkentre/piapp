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
 * @file turntable.c
 * @brief Turntable processing
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */
#include "RevoRT.h"
#include "FixedHeap.h"
#include "vectors.h"
#include "cli.h"
#include "tcanalysis.h"
#include "vsb.h"
#include "FBvsb.h"
#include "turntable.h"

/*
 * TurnTable functions
 */
#define NTDVSB       (2)
#define NFDVSB       (2)

AC4_MODULE;

/* Turntable State */
struct TurntableState_ {
  AC4_HEADER;

  /* Configuration */
  int flags;
  int fs;
  int frame_size;

  /* Parameter */
  int note_tdvsb[NTDVSB];
  int note_fdvsb[NFDVSB];

  /* Audio Components */
  TCANALYSIS_State * tcana;
  VSB_State        * tdvsb[NTDVSB];
  FBVSB_State      * fbvsb[NFDVSB];
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(flags,           CLI_ACXPTM,   TurntableState, "Control flags"),
};

TurntableState* Turntable_init(const char *name, int fs, int frame_size)
{
  int i;
  TurntableState* st;
  char subname[32];
  float bar = 2.0f;
  float sec = 60.f / 33.3f * bar;
  
  st = MEM_ALLOC(MEM_SDRAM, TurntableState, 1, 4);
  AC4_ADD(name, st, "Turntable ");
  AC4_ADD_REG(rd);

  st->fs         = fs;
  st->frame_size = frame_size;

  sprintf(subname, "%s_tcana",name);
  st->tcana = tcanalysis_init(subname, st->fs, 1000.f);

  for (i=0; i<NTDVSB; i++) {
    sprintf(subname, "%s_tdvsb%d",name, i);    
    st->tdvsb[i] = vsb_init(subname, (int)(st->fs * sec));
  }
  for (i=0; i<NFDVSB; i++) {
    sprintf(subname, "%s_fdvsb%d",name, i);    
    st->fbvsb[i] = FBvsb_init(subname, 2, st->fs, st->frame_size, sec);
  }

  st->note_tdvsb[0] = 1;
  //  note_fdvsb[NFDVSB];
  
  return st;
}

void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[])
{
  int i;
  int frame_size = st->frame_size;

  VARDECLR(float, speed);
  VARDECLR(float*, zeros);
  VARDECLR(float*, temp);
  SAVE_STACK;
  ALLOC(speed, frame_size, float);
  ALLOC(zeros, 2, float*);
  ALLOC(temp,  2, float*);
  ALLOC(zeros[0],  frame_size, float);
  ALLOC(zeros[1],  frame_size, float);
  ALLOC(temp[0],  frame_size, float);
  ALLOC(temp[1],  frame_size, float);

  vec_set(zeros[0], 0.f, frame_size);
  vec_set(zeros[1], 0.f, frame_size);

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana, speed, tc[0], tc[1], frame_size);

  COPY(out[0], zeros[0], frame_size);
  COPY(out[1], zeros[1], frame_size);

  /* TDVSB */
  for (i=0; i<NTDVSB; i++){
    if (st->note_tdvsb[i])
      vsb_process(st->tdvsb[i], temp, in, speed, frame_size);
    else
      vsb_process(st->tdvsb[i], temp, zeros, speed, frame_size);
    vec_add1(out[0], temp[0], frame_size);
    vec_add1(out[1], temp[1], frame_size);
  }

  /* FDVSB */
  for (i=0; i<NFDVSB; i++){
    if (st->note_fdvsb[i])
      FBvsb_process(st->fbvsb[i], temp, in, speed);
    else
      FBvsb_process(st->fbvsb[i], temp, zeros, speed);
    vec_add1(out[0], temp[0], frame_size);
    vec_add1(out[1], temp[1], frame_size);
  }
  RESTORE_STACK;
}