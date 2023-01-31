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

# version: $LastChangedRevision: 9007 $
# author: zkhan


.PHONY: preprocess
preprocess:
	@$(PRINTSCR) ---- Generating version info for module $(MODULE) ---- 
	@$(PRINTSCR) char __$(strip $(MODULE_PREFIX))_version[]= \
		\"Revolabs $(MODULE_PREFIX) $(MODULE_VERSION) $(COMMON_VERSION)\"\;\
		> __version_info_$(PLATFORM).c
	@$(CC) $(TOOLS_CFLAGS) $(ALL_CFLAGS) -c __version_info_$(PLATFORM).c

.PHONY: preprocess_clean
preprocess_clean:
	@$(REMOVE) __version_info_$(PLATFORM).c __version_info_$(PLATFORM).o
