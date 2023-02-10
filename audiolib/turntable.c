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

  /* Parameters */
  float ingain;
  float recgain;
  float overdub;
  float fdbal;
  float monitor;
  int   looplen;
  int   looplen_;

  /* for OSC */
  int vinyl_len;
  float* vinyl;

  /* Audio Components */
  TCANALYSIS_State * tcana;
  VSB_State        * tdvsb;
  FBVSB_State      * fdvsb;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(flags,   CLI_ACXPTM, TurntableState, "Control flags"),
  AC_REGDEF(ingain,  CLI_ACFPTM, TurntableState, "Input gain"),
  AC_REGDEF(recgain, CLI_ACFPTM, TurntableState, "Recording gain"),
  AC_REGDEF(overdub, CLI_ACFPTM, TurntableState, "overdub"),
  AC_REGDEF(fdbal,   CLI_ACFPTM, TurntableState, "Balance to FD"),
  AC_REGDEF(monitor, CLI_ACFPTM, TurntableState, "Input monitor level"),
  AC_REGDEF(looplen, CLI_ACIPTM, TurntableState, "Input monitor level"),
  AC_REGADEF(vinyl, vinyl_len, CLI_ACFPTMA,   TurntableState, "position"),
};

TurntableState* Turntable_init(const char *name, int fs, int frame_size)
{
  TurntableState* st;
  char subname[32];
  float bar = 2.0f;
  float sec = 60.f / 33.3f * bar;
  
  st = MEM_ALLOC(MEM_SDRAM, TurntableState, 1, 4);
  AC4_ADD(name, st, "Turntable");
  AC4_ADD_REG(rd);

  st->fs         = fs;
  st->frame_size = frame_size;

  st->ingain     = 1.0f;
  st->recgain    = 0.0f;
  st->overdub    = 0.0f;
  st->fdbal      = 0.0f;
  st->monitor    = 1.0f;
  st->looplen    = 4;
  st->looplen_   = 0;

  st->vinyl_len  = 2;
  st->vinyl = MEM_ALLOC(MEM_SDRAM, float, st->vinyl_len, 4);

  sprintf(subname, "%s_tcana",name);
  st->tcana = tcanalysis_init(subname, st->fs, 1000.f);

  sprintf(subname, "%s_fdvsb",name);    
  st->fdvsb = FBvsb_init(subname, 2, st->fs, st->frame_size, sec);

  sprintf(subname, "%s_tdvsb",name);    
  st->tdvsb = vsb_init(subname, FBvsb_get_buflen(st->fdvsb));

  
  return st;
}

void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[])
{
  int frame_size = st->frame_size;

  VARDECLR(float, speed);
  VARDECLR(float*, in_adj);
  VARDECLR(float*, in_rec);
  VARDECLR(float*, temp);
  SAVE_STACK;
  ALLOC(speed, frame_size, float);
  ALLOC(in_adj, 2, float*);
  ALLOC(in_rec, 2, float*);
  ALLOC(temp,   2, float*);
  ALLOC(in_adj[0],  frame_size, float);
  ALLOC(in_adj[1],  frame_size, float);
  ALLOC(in_rec[0],  frame_size, float);
  ALLOC(in_rec[1],  frame_size, float);
  ALLOC(temp[0],  frame_size, float);
  ALLOC(temp[1],  frame_size, float);

  /* loop length */
  if (st->looplen != st->looplen_) {
    float ratio;
    switch(st->looplen) {
    case 0:
      ratio = 1.f / 8.f;
      break;
    case 1:
      ratio = 2.f / 8.f;
      break;
    case 2:
      ratio = 3.f / 8.f;
      break;
    case 3:
      ratio = 4.f / 8.f;
      break;
    default:
    case 4:
      ratio = 1.f;
      break;
    }
    vsb_set_looplen(st->tdvsb, ratio);
    FBvsb_set_looplen(st->fdvsb, ratio);
    st->looplen_ = st->looplen;
  }

  /* overdub setting */
  if (st->overdub == 0.f && st->recgain > 0.f) {
    vsb_set_feedbackgain(st->tdvsb, 0.f);
    FBvsb_set_feedbackgain(st->fdvsb, 0.f);
  }
  else {
    vsb_set_feedbackgain(st->tdvsb, 1.f);
    FBvsb_set_feedbackgain(st->fdvsb, 1.f);
  }

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana, speed, tc[0], tc[1], frame_size);

  /* Adjust gain */
  vec_muls(in_adj[0], in[0], st->ingain, frame_size);
  vec_muls(in_adj[1], in[1], st->ingain, frame_size);
  vec_muls(in_rec[0], in_adj[0], st->recgain, frame_size);
  vec_muls(in_rec[1], in_adj[1], st->recgain, frame_size);

  /* TDVSB */
  vsb_process(st->tdvsb, out, in_rec, speed, frame_size);

  /* FDVSB */
  FBvsb_process(st->fdvsb, temp, in_rec, speed);

  /* Balance between TD vs FD */
  vec_wadd1(out[0], 1.0f - st->fdbal, st->fdbal, temp[0], frame_size);
  vec_wadd1(out[1], 1.0f - st->fdbal, st->fdbal, temp[1], frame_size);

  /* compute vinyl level */
  vec_mul(temp[0], out[0], out[0], frame_size);
  vec_mul(temp[1], out[1], out[1], frame_size);
  float vinyl_lev;
  vinyl_lev =                0.5f * MtodB(vec_sum(temp[0], frame_size) * RECIP((float)frame_size));
  vinyl_lev = MAX(vinyl_lev, 0.5f * MtodB(vec_sum(temp[1], frame_size) * RECIP((float)frame_size)));

  /* Mix input */
  vec_wadd1(out[0], 1.0f, st->monitor, in_adj[0], frame_size);
  vec_wadd1(out[1], 1.0f, st->monitor, in_adj[1], frame_size);

  /* Update vinyl */
  float base  = 10.f * RECIP(30.48f);
  float maxdb =   0.0f;
  float mindb =  -50.0f; 
  st->vinyl[0] = MAX(0.f, MIN(1.f, (vinyl_lev - mindb) * RECIP(maxdb - mindb))) * (1-base) + base;
  st->vinyl[1] = vsb_vinylpos(st->tdvsb);

  RESTORE_STACK;
}
