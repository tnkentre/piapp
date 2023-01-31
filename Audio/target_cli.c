/******************************************************************************
 *
 * REVOLABS CONFIDENTIAL
 * __________________
 *
 *  [2005] - [2013] Revolabs Incorporated
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

#include "fileio.h"
#include "FixedHeap.h"
#include "target_cli.h"

const ctl_bit_t target_ctl_bit[] = {
  {"ctl_toneusbout",    CTL_TONEUSBOUT, 	"Sends a tone to the USB out"},
  {"ctl_toneusbin",     CTL_TONEUSBIN,  	"Replaces the USB In by a tone"},
  {"ctl_tonemic1",      CTL_TONEMIC1,   	"Replaces mic1 with a tone"},
  {"ctl_usb2fe",        CTL_USB2FE,     	"USB Input -> Far End"},
  {"ctl_bypass_miccal", CTL_BYPASS_MICCAL, 	"Bypass mic calibration"},

  {NULL, 0}
};

const cli_cmd_t target_cli_cmd[] = {
  {"tversion",          CLI_TVERSION,           "Product version"},
  {"heapinfo",          CLI_HEAP_INFO,          "Heap info"},
  {"file_read",         CLI_FILE_READ,          "File transfer command"},
  {"button",            CLI_BUTTON,             "Simulates the press of a button"},
  {"fedelay",           CLI_FEDELAY,            "Adjusts the far end delay"},
  {"programcap",        CLI_PROGRAM_CAP,        "Progam the CapTouch Flash"},
  {"bootloadmode",      CLI_USB_BOOTLOAD,       "Place USB chip in bootload mode"},
  {"usbreset",          CLI_USB_RESET,          "Reset the USB chip"},
  {"sversions",         CLI_GET_VERSIONS,       "DSP and USB Software versions"},
  {"showpress",         CLI_SET_CAPVIEW,        "Set PC and CapTouch Button Logging"},
  {"simbuttonpress",    CLI_SIM_BUTTON,         "Simulate a CapTouch Button press"},
  {"sendPCbutton",      CLI_SEND_PC_BUTTON,     "Send the button event to PC via USB"},
  {"setchannel",        CLI_SETCHANNEL,         "Select a mic channel (manufacturing mode) [0-6]"},
  {"resetcodec",        CLI_RESETCODEC,         "Reset A/D converters"},
  {"extmicled",         CLI_EXTMICLED,          "Sets the state of the ext mic LED"},
  {"tweeter_version",   CLI_TWEETER_VERSION,    "Set the version of tweeter used 0 for old, 1 for new"},
  {"dialervolumeupdate",CLI_DIALER_VOL_UPDATE,  "notify the DSP about dialer volume presses"},
  // Dialer speaker volume setting change.
  {"dialerspkvolumechange",CLI_DIALER_SPK_VOL_CHANGE,  "notify the DSP about dialer volume presses"},
  {"BundleRev",         CLI_SEND_BUNDLE_INFO,   "Send Bundle info to USB chip for HID retrieval"},
  {"ipinfo",            CLI_SEND_IP_ADDRS_INFO, "Send IP info to USB chip for HID retrieval"},
  {"savemems",          CLI_SAVE_MEMS,          "Save MEMs Data to mems.dat file"},
  {"memstousb",         CLI_MEMStoUSB,          "Send a given MEM to the USB"},
  {"spkcal",            CLI_SPKCAL,             "Speaker calibration (LW, LT, RT, RW)"},
  {"miccal",            CLI_MICCAL,             "Mic calibration"},

  {NULL, 0}
};

void cli_TargetStatus()
{
 
}


void cli_TargetStop()
{
}

void cli_target_init(void)
{
}

int cli_parseTargetCommand(const SymbolTableEntry *ste,
                           const char *command)
{
  char token[CMD_SIZE];

  switch (ste->u.i) {

  case CLI_TVERSION:
    {
      // extern const char  __dspfw_version[];
      // extern const char  __audiomodules_version[];
      // extern const char  __codecs_version[];
      //extern const char  __celt_version[];
      extern const char  __audiolib_version[];
      //extern const char  __opus0_9_14_version[];

      //PRINTF("dspfw:        %s", __dspfw_version);
      //PRINTF("audiomodules: %s", __audiomodules_version);
      //PRINTF("codecs:       %s", __codecs_version);
      //PRINTF("celt:         %s", __celt_version);
      PRINTF("audiolib:     %s", __audiolib_version);
      //PRINTF("opus0_9_14:   %s", __opus0_9_14_version);
      //PRINTF("Driver HwVer: %s", _Drivers_HwVersion);
      return RC_OK;
    }

  case CLI_HEAP_INFO:
    {
#if !defined(pclinux)
      if (heapAllocFile_ptr != NULL) {
        openFlashFile("heapinfo.txt");
        writeToFlashFile(heapAllocFile_ptr);
        closeFlashFile();
      } else {
        TRACE(LEVEL_INFO,"Heap info not enabled in this build!");
      }
      return RC_OK;
#else
      TRACE(LEVEL_INFO,"Heap info command not available for pclinux");
      return RC_UNIMPLEMENTED;
#endif
    }

  default:
    {
      TRACE(LEVEL_ERROR, "Unknown target specific command: %s", command);
      return RC_BAD_REQUEST;
    }
  }
}
