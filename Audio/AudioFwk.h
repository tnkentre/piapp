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
 * @file AudioFwk.h
 * @brief Audio processing frame work
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */

#ifndef _AUDIOFWK_H_
#define _AUDIOFWK_H_

/** Internal Fir state. Should never be accessed directly. */
struct AudioFwkState_;

/** @class FirState
 * This holds the state of the Fir.
 */
typedef struct AudioFwkState_ AudioFwkState;

/** Declare type of callback function which will be called every frame. */
typedef void (*AUDIOFWK_PROCESS_CALLBACK)(float* out[], float* in[]);

/** @fn AudioFwkState* audiofwk_init(const char * name, int nch_input, int nch_output, int frame_size, AUDIOFWK_PROCESS_CALLBACK callback)
 * @brief This function initialize and start the audio processing for JACK
 * @param name Name of the component
 * @param nch_input Number of the input channels
 * @param nch_output Number of the output channels
 * @param frame_size Length of the frame
 * @param callbakc function pointer to be called every frame
 * @return Initialized state of AudioFwk
 */
AudioFwkState* audiofwk_init(const char * name, int nch_input, int nch_output, int frame_size, AUDIOFWK_PROCESS_CALLBACK callback);

/** @fn void audiofwk_close(AudioFwkState* st)
 * @brief This function destroy the state of AudioFwk to free
 * @param st State of AudioFwk
 */
void audiofwk_close(AudioFwkState* st);

#endif /* _AUDIOFWK_H_ */
