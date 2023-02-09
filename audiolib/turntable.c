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
  float fdbal;
  float monitor;

  /* Audio Components */
  TCANALYSIS_State * tcana;
  VSB_State        * tdvsb;
  FBVSB_State      * fdvsb;
} ;

static const RegDef_t rd[] = {
  AC_REGDEF(flags,   CLI_ACXPTM, TurntableState, "Control flags"),
  AC_REGDEF(fdbal,   CLI_ACFPTM, TurntableState, "Balance to FD"),
  AC_REGDEF(monitor, CLI_ACFPTM, TurntableState, "Input monitor level"),
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

  st->fdbal      = 1.0f;
  st->monitor    = 1.0f;

  sprintf(subname, "%s_tcana",name);
  st->tcana = tcanalysis_init(subname, st->fs, 1000.f);

  sprintf(subname, "%s_tdvsb",name);    
  st->tdvsb = vsb_init(subname, (int)(st->fs * sec));

  sprintf(subname, "%s_fdvsb",name);    
  st->fdvsb = FBvsb_init(subname, 2, st->fs, st->frame_size, sec);
  
  return st;
}

void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[])
{
  int frame_size = st->frame_size;

  VARDECLR(float, speed);
  VARDECLR(float*, temp);
  SAVE_STACK;
  ALLOC(speed, frame_size, float);
  ALLOC(temp,  2, float*);
  ALLOC(temp[0],  frame_size, float);
  ALLOC(temp[1],  frame_size, float);

  /* Compute speed from TC sound */
  tcanalysis_process(st->tcana, speed, tc[0], tc[1], frame_size);

  /* TDVSB */
  vsb_process(st->tdvsb, out, in, speed, frame_size);

  /* FDVSB */
  FBvsb_process(st->fdvsb, temp, in, speed);

  /* Balance between TD vs FD */
  vec_wadd1(out[0], 1.0f - st->fdbal, st->fdbal, temp[0], frame_size);
  vec_wadd1(out[1], 1.0f - st->fdbal, st->fdbal, temp[1], frame_size);

  /* Mix input */
  vec_wadd1(out[0], 1.0f, st->monitor, in[0], frame_size);
  vec_wadd1(out[1], 1.0f, st->monitor, in[1], frame_size);

  RESTORE_STACK;
}
