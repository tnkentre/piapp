/******************************************************************************
 * 
 * TANAKA ENTERPRISES CONFIDENTIAL
 * __________________
 * 
 *  [2015] - [2016] Tanaka Enterprises
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of Tanaka Enterprises and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Tanaka Enterprises
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Tanaka Enterprises.
 * 
 *****************************************************************************/

/**
 * @file vsb.h
 * @brief This file implements a variable speed buffer module.
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#ifndef _VSB_H_
#define _VSB_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Internal SBNI state. Should never be accessed directly. */
struct VSB_State_;

/** @class VSB_State,  VSB_State
 * This holds the state of the VSB.
 */
typedef struct VSB_State_ VSB_State;

/** @fn VSB_State *vsb_init(const char * name, float fs, int frame_size, int nband)
 * @brief This function creates a VSB_State component
 * @param name Name of the component
 * @param fs Sampling frequency
 * @param freq_tc Reference frequency of TC signal
 * @return Initialized state of the VSB
 */
VSB_State *vsb_init(const char * name, int size);

/** @fn void vsb_process(VSB_State * restrict st, const float * E2, int nband)
 * @brief This function execute vsb in frequency domain
 * @param st    State of the VSB
 * @param speed output speed buffer's pointer array
 * @param tcL   input Lch tc signal buffer's pointer array
 * @param tcR   input Rch tc signal buffer's pointer array
 * @param len   Length of the buffer
 */
void vsb_process(VSB_State * restrict st, float* dst[], float* src[], float* speed, int len);

#ifdef __cplusplus
} 
#endif

#endif /* _VSB_H_ */
