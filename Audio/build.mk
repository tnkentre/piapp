#!/bin/make
#*******************************************************************************
#
# REVOLABS CONFIDENTIAL
# __________________
#
#  [2005] - [2011] Revolabs Incorporated
#  All Rights Reserved.
#
# NOTICE:  All information contained herein is, and remains
# the property of Revolabs Incorporated and its suppliers,
# if any.  The intellectual and technical concepts contained
# herein are proprietary to Revolabs Incorporated
# and its suppliers and may be covered by U.S. and Foreign Patents,
# patents in process, and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this material
# is strictly forbidden unless prior written permission is obtained
# from Revolabs Incorporated.
#
#*******************************************************************************

# version: $LastChangedRevision: 6317 $
# author: zkhan

# C++ Source files
CXXSRC	= \

# C Source files
CSRC	= \
	AudioServer.c	\
	target_cli.c	\
	AudioFwk.c	\
	MidiFwk.c	\
	AudioProc.c	\


# Assembly files
SSRC	= \
	$(MODULE_PREPROCESS_FILE:.tcf=.asm) \


# Files to export to INCLUDE_DIR
HEADERS	=  \

-include $(BASE_DIR)/Build/rules.mk
