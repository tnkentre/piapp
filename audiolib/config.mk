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

# version: $LastChangedRevision: 6177 $
# author: zkhan

# Prefix for this module
export MODULE_PREFIX = audiolib2

export AC := $(SVNROOT)/AudioComponents/trunk

# Type of output this module will produce: archive (.a), dynamic (.so), or exe
export MODULE_OUTPUT = archive

# Version of this module, this could be auto filled by SVN
export MODULE_VERSION = 1.0

# List dirs in order for making
export MODULE_SUB_DIRS		:= 	\
	. \

export MODULE_CFLAGS += \
	-I$(AC)/Include.$(PLATFORM) \
	-I$(AC)/audiolib/speex/include \
	-I$(AC)/audiolib/DSP/c674x/dsplib_v12/src \
	-DWITH_POSIX -Drestrict="" \

export MODULE_LDFLAGS = \

export MODULE_EXTERN_LIBS := \

export MODULE_PREPROCESS_FILE := 

export MODULE_ARCHIVES := \

# Cross compile flags, do not change
-include $(BASE_DIR)/Build/def-$(PLATFORM).mk
-include $(BASE_DIR)/Build/module-common.mk
