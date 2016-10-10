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

# version: $LastChangedRevision: 9642 $
# author: zkhan

MAKEFLAGS = --quiet


# Main Makefile
# Define the PLATFORM you are making for example make PLATFORM=omap1

ifndef PLATFORM 
$(error !! PLATFORM must be defined before make can proceed)
endif


export BASE_DIR=$(CURDIR)

# Location of various builds
export INCLUDE_DIR=$(BASE_DIR)/Include.$(PLATFORM)
export BUILD_DIR=$(BASE_DIR)/Build.$(PLATFORM)

# Include host OS stuff
-include Build/host.mk

# This file includes the dirs that will be built.
-include Build/modules-$(PLATFORM).mk

# This is common version of this software, module version will be prefixed
export COMMON_VERSION := @`$(SVNVERS)`+`$(DATE)`

# All modules to be built
.PHONY: all
all:
	echo $(PLATFORM)
	@if [ ! -d $(INCLUDE_DIR) ]; then $(MKDIR) $(INCLUDE_DIR); fi
	@if [ ! -d $(BUILD_DIR) ]; then $(MKDIR) $(BUILD_DIR); fi
	@for m in $(MODULES); \
	do \
		if (!(	$(CHDIR) $(BASE_DIR)/$$m && \
			MODULE=$$m MODULE_DIR=$(BASE_DIR)/$$m \
			$(MAKE) -f config.mk all output )) \
		then \
			exit -1; \
		fi; \
	done

# Clean all modules
.PHONY: clean
clean:
	@for m in $(MODULES); \
	do \
		if (!(	$(CHDIR) $(BASE_DIR)/$$m && \
			MODULE=$$m MODULE_DIR=$(BASE_DIR)/$$m \
			$(MAKE) -f config.mk clean )) \
		then \
			exit -1; \
		fi; \
	done
	@if [ -d $(INCLUDE_DIR) ]; then $(REMOVE) $(INCLUDE_DIR); fi
	@if [ -d $(BUILD_DIR) ]; then $(REMOVE) $(BUILD_DIR); fi

# Platform system make, this is where everything comes together to build images
.PHONY: platform
platform:
	$(MAKE) -f $(BASE_DIR)/Build/platform-$(PLATFORM).mk all

.PHONY: platform_clean
platform_clean:
	$(MAKE) -f $(BASE_DIR)/Build/platform-$(PLATFORM).mk clean
