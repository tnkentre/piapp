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
 * @file MidiFwk.h
 * @brief Audio processing frame work
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#ifndef _MIDIFWK_H_
#define _MIDIFWK_H_

/** Internal Fir state. Should never be accessed directly. */
struct MidiFwkState_;

/** @class FirState
 * This holds the state of the Fir.
 */
typedef struct MidiFwkState_ MidiFwkState;

/** Declare type of callback function which will be called every frame. */
typedef void (*MIDIFWK_PROCESS_CALLBACK)(uint64_t time, uint8_t type, uint8_t channel, uint8_t param, uint8_t value);

/** @fn MidiFwkState* midifwk_init(const char * name, int nch_input, int nch_output, int frame_size, MIDIFWK_PROCESS_CALLBACK callback)
 * @brief This function initialize and start the audio processing for JACK
 * @param name Name of the component
 * @param nch_input Number of the input channels
 * @param nch_output Number of the output channels
 * @param frame_size Length of the frame
 * @param callbakc function pointer to be called every frame
 * @return Initialized state of MidiFwk
 */
MidiFwkState* midifwk_init(const char * name, MIDIFWK_PROCESS_CALLBACK callback);

/** @fn void midifwk_close(MidiFwkState* st)
 * @brief This function destroy the state of MidiFwk to free
 * @param st State of MidiFwk
 */
void midifwk_close(MidiFwkState* st);

#endif /* _MIDIFWK_H_ */
