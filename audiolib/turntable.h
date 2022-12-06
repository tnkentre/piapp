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
 * @file Turntable.h
 * @brief Audio processing
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#ifndef _TURNTABLE_H_
#define _TURNTABLE_H_

/** Internal Turntable state. Should never be accessed directly. */
struct TurntableState_;

/** @class TurntableState
 * This holds the state of the FBbasssynth.
 */
typedef struct TurntableState_ TurntableState;

/** @fn TurntableState* Turntable_init(const char *name, int fs, int frame_size)
 * @brief This function creates a Turntable component
 * @param name Name of the component
 * @param fs Sampling frequency
 * @param frame_size Number of frame size
 * @return Initialized state of the Turntable
 */
TurntableState* Turntable_init(const char *name, int fs, int frame_size, int ntdvsb, int nfdvsb);

/** @fn void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[])
 * @brief This function execute Turntable
 * @param st State of the Turntable
 * @param out Output buffer's pointer array
 * @param in  Input audio buffer's pointer array
 * @param tc  Input time code buffer's pointer array
 */
void Turntable_proc(TurntableState* st, float* out[], float* in[], float* tc[]);

void Turntable_set_note_td(TurntableState* st, int id, float note);
void Turntable_set_note_fd(TurntableState* st, int id, float note);
void Turntable_set_gain_in(TurntableState* st, float gain);
#endif /* _TURNTABLE_H_ */
