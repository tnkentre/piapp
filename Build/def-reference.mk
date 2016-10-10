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

# version: $LastChangedRevision: 6078 $
# author: zkhan

# Location of cross tools 
export TOOL_PATH :=

# The prefix used in cross compilation
export TOOL_PREFIX :=

# Various tools
export CC	:= $(TOOL_PATH)/$(TOOL_PREFIX)gcc
export CXX	:= $(TOOL_PATH)/$(TOOL_PREFIX)g++
export AS	:= $(TOOL_PATH)/$(TOOL_PREFIX)as
export LD	:= $(TOOL_PATH)/$(TOOL_PREFIX)ld
export AR	:= $(TOOL_PATH)/$(TOOL_PREFIX)ar
export SRP	:= $(TOOL_PATH)/$(TOOL_PREFIX)strip

# Linker flags for all modules
export ALL_LDFLAGS 	:= \
	$(LDFLAGS)

# Assembler flags for all modules
export ALL_ASFLAGS	:= \
	$(ASFLAGS)

# C compiler flags for all modules
export ALL_CFLAGS	:= \
	$(CFLAGS) -I. -Wall -D$(PLATFORM) 

# C++ compiler flags for all modules
export ALL_CXXFLAGS	:= \
	$(CFLAGS) -I. -Wall -D$(PLATFORM)

# Special ASM flags for these tools
export TOOLS_ASFLAGS	:= \

# Special C flags for these tools
export TOOLS_CFLAGS	:= \

# Special C++ flags for these tools
export TOOLS_CXXFLAGS	:= \
