/******************************************************************************
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
 *****************************************************************************/

/**
 * @file target_cli.h
 * @brief Target dependent part of the CLI
 * @version $LastChangedRevision:$
 * @author Pascal Cleve
 */


#if !defined(_target_cli_)
#define _target_cli_

#include "cli.h"
#include "Registers.h"

/** Target specific objects */
enum {
  /** Activate tone 1 on the daisy out line */
  CTL_TONEUSBOUT = CTL_TARGET_BIT,              /**< Send a tone to the USB out */
  CTL_TONEUSBIN,                                /**< Replace the USB In by a tone */
  CTL_TONE2OUT,                                 /**< Send tone2 to the output */
  CTL_TONEMIC1,                                 /**< Tone Mic 1 */
  CTL_USB2FE,                                   /**< Send the USB to the Far End */
  CTL_BYPASS_MICCAL,                            /**< Bypass mic calibration */
};

/** Target specific commands */
enum {
  CLI_TVERSION = CLI_TARGET_SPECIFIC_COMMAND,	/**< Product version */
  CLI_HEAP_INFO,                		/**< Dump heap info to a file */
  CLI_FILE_READ,                		/**< File transfer to DSP */
  CLI_BUTTON,                                   /**< Simulates the press of a button */
  CLI_FEDELAY,                                  /**< Adjusts the far end delay */
  CLI_PROGRAM_CAP,                              /**< Program the CapTouch Flash */
  CLI_USB_BOOTLOAD,                             /**< Place USB chip in Bootload Mode */
  CLI_USB_RESET,                                /**< Reset the USB chip */
  CLI_GET_VERSIONS,                             /**< Software versions: DSP and USB */
  CLI_SET_CAPVIEW,                              /**< Log the CapTouch button presses */
  CLI_SIM_BUTTON,                               /**< Simulate CapTouch button press */
  CLI_SEND_PC_BUTTON,                           /**< Send button evnt to the PC via USB */
  CLI_SETCHANNEL,                               /**< Select a mic channel (manufacturing mode) */
  CLI_RESETCODEC,                               /**< Reset A/D converters */
  CLI_EXTMICLED,                                /**< Sets the state of the external mic LED */
  CLI_TWEETER_VERSION,                          /**< Sets the tweeter version. 0 for Neodymium (old), 1 for Alnico (new) */
  CLI_DIALER_VOL_UPDATE,                        /**< Notifies DSP that the dialer volume buttons have been pressed (new) */
  CLI_DIALER_SPK_VOL_CHANGE,                    /**< Notifies DSP that the dialer speaker volume setting have been changed (new) */
  CLI_SEND_BUNDLE_INFO,                         /**< Used for Bundle Retrieval over the HID interface */
  CLI_SEND_IP_ADDRS_INFO,                       /**< Used for IP Address Retrieval over the HID interface */
  CLI_SAVE_MEMS,                                /**< Save MEMs Data to mems.dat */
  CLI_MEMStoUSB,                                /**< Send a given MEM to the USB */
  CLI_SPKCAL,                                   /**< Speaker calibration (LW, LT, RT, RW) */
  CLI_MICCAL,                                   /**< Mic calibration */
};

extern int MEMStoUSB;
extern void cli_target_init(void);
extern rv_uint32 CY_uiUpdatedCapSenseFlash;
extern void CY_SetUsbInBootLoadMode(void);
extern void CY_ResetTheUsbChip(void);
extern void CY_SendButtonMsgToPC(char button);
extern void CAP_ShowButtonResponses(int on_off);
extern void CAP_SimulateButtonPress(char button);
extern void AdjustSpeakerCallVolume(rv_uint8 uAction, rv_uint8 uiHostIndex);
extern void SendBundleInfoToHost(unsigned char len, char *pBundle);
extern void SendIpInfoToHost(char *pIp);


#endif
