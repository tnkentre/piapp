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
AC4_MODULE;

/* Turntable State */
struct TurntableState_ {
  AC4_HEADER;

  /* Configuration */
  int flags;
  int fs;
  int frame_size;
  int ntdvsb;
  int nfdvsb;

  /* Parameter */
  float *note_tdvsb;
  float *note_fdvsb;
  float gain_input;

  /* Audio Components */
  TCANALYSIS_State * tcana;
  VSB_State        ** tdvsb;
  FBVSB_State      ** fdvsb;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(flags,           CLI_ACXPTM,   TurntableState, "Control flags"),
};

TurntableState* Turntable_init(const char *name, int fs, int frame_size, int ntdvsb, int nfdvsb)
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
  st->ntdvsb     = ntdvsb;
  st->nfdvsb     = nfdvsb;

  sprintf(subname, "%s_tcana",name);
  st->tcana = tcanalysis_init(subname, st->fs, 1000.f);

  if (st->ntdvsb) {
    st->note_tdvsb = MEM_ALLOC(MEM_SDRAM, float,      st->ntdvsb, 4);
    st->tdvsb      = MEM_ALLOC(MEM_SDRAM, VSB_State*, st->ntdvsb, 4);
    for (i=0; i<st->ntdvsb; i++) {
      sprintf(subname, "%s_tdvsb%d",name, i);    
      st->tdvsb[i] = vsb_init(subname, (int)(st->fs * sec));
    }
  }

  if (st->nfdvsb) {
    st->note_fdvsb = MEM_ALLOC(MEM_SDRAM, float,       st->nfdvsb, 4);
    st->fdvsb      = MEM_ALLOC(MEM_SDRAM, FBVSB_State*, st->nfdvsb, 4);
    for (i=0; i<st->nfdvsb; i++) {
      sprintf(subname, "%s_fdvsb%d",name, i);    
      st->fdvsb[i] = FBvsb_init(subname, 2, st->fs, st->frame_size, sec);
    }
  }
  
  return st;
}

void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[])
{
  int i;
  int frame_size = st->frame_size;

  VARDECLR(float, speed);
  VARDECLR(float, note);
  VARDECLR(float*, in_adj);
  VARDECLR(float*, zeros);
  VARDECLR(float*, temp);
  SAVE_STACK;
  ALLOC(speed, frame_size, float);
  ALLOC(note,  frame_size, float);
  ALLOC(in_adj, 2, float*);
  ALLOC(zeros,  2, float*);
  ALLOC(temp,   2, float*);
  ALLOC(in_adj[0],  frame_size, float);
  ALLOC(in_adj[1],  frame_size, float);
  ALLOC(zeros[0],  frame_size, float);
  ALLOC(zeros[1],  frame_size, float);
  ALLOC(temp[0],  frame_size, float);
  ALLOC(temp[1],  frame_size, float);

  vec_muls(in_adj[0], in[0], st->gain_input, frame_size);
  vec_muls(in_adj[1], in[1], st->gain_input, frame_size);
  
  vec_set(zeros[0], 0.f, frame_size);
  vec_set(zeros[1], 0.f, frame_size);

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana, speed, tc[0], tc[1], frame_size);

  COPY(out[0], zeros[0], frame_size);
  COPY(out[1], zeros[1], frame_size);

  /* TDVSB */
  for (i=0; i<st->ntdvsb; i++){
    if (st->note_tdvsb[i])
      vec_set(note, 1.f, frame_size);
    else
      vec_set(note, 0.f, frame_size);
    vsb_process(st->tdvsb[i], temp, in_adj, speed, note, frame_size);
    vec_add1(out[0], temp[0], frame_size);
    vec_add1(out[1], temp[1], frame_size);
  }

  /* FDVSB */
  for (i=0; i<st->nfdvsb; i++){
    if (st->note_fdvsb[i])
      vec_set(note, 1.f, frame_size);
    else
      vec_set(note, 0.f, frame_size);
    FBvsb_process(st->fdvsb[i], temp, in_adj, speed, note);
    vec_add1(out[0], temp[0], frame_size);
    vec_add1(out[1], temp[1], frame_size);
  }

  vec_add1(out[0], in_adj[0], frame_size);
  vec_add1(out[1], in_adj[1], frame_size);

  RESTORE_STACK;
}

void Turntable_set_note_td(TurntableState* st, int id, float note)
{
  if (id < st->ntdvsb)
    st->note_tdvsb[id] = note;
}
void Turntable_set_note_fd(TurntableState* st, int id, float note)
{
  if (id < st->nfdvsb)
    st->note_fdvsb[id] = note;
}
void Turntable_set_gain_in(TurntableState* st, float gain)
{
  st->gain_input = gain;
}
