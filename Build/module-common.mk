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

# version: $LastChangedRevision: 6291 $
# author: zkhan


-include $(BASE_DIR)/Build/pre-$(PLATFORM).mk

export MODULE_DIR := $(CURDIR)

ifndef MODULE_PREFIX
$(error !! MODULE_PREFIX must be defined in $(MODULE_DIR)/config.mk)
endif

ifndef MODULE_VERSION
$(error !! MODULE_VERSION must be defined in $(MODULE_DIR)/config.mk)
endif

ifndef MODULE_OUTPUT
$(error !! MODULE_OUTPUT must be defined in $(MODULE_DIR)/config.mk)
endif

export MODULE_ARCHIVE		:= $(BUILD_DIR)/$(strip $(MODULE_PREFIX)).a
export MODULE_EXE		:= $(BUILD_DIR)/$(strip $(MODULE_PREFIX))
export MODULE_DYNAMIC		:= $(BUILD_DIR)/lib$(strip $(MODULE_PREFIX)).so
export MODULE_MAP		:= $(BUILD_DIR)/$(strip $(MODULE_PREFIX)).map
export MODULE_OUTPUT_ALL	:= \
					$(MODULE_ARCHIVE) \
					$(MODULE_EXE) \
					$(MODULE_DYNAMIC) \
					$(MODULE_MAP)


# These are the final outputs
.PHONY: exe
exe:		$(MODULE_EXE)

.PHONY: dynamic
dynamic:	$(MODULE_DYNAMIC)

.PHONY: archive
archive:	$(MODULE_ARCHIVE)

# Deal with individual folders in the module
# Generate revinfo 
.PHONY: all
all: preprocess
	@$(PRINTSCR) ---- Building module $(MODULE) ----
	@$(REMOVE) __objs_$(PLATFORM)
	@for s in $(MODULE_SUB_DIRS); \
	do \
		if (!(	$(PRINTSCR) ---- Building module subfolder $$s ---- && \
			$(CHDIR) $(MODULE_DIR)/$$s && \
			MODULE_SUB_DIR=$(MODULE_DIR)/$$s \
			$(MAKE) -f build.mk all && $(MAKE) -f build.mk headers)) \
		then \
			exit -1; \
		fi; \
	done

.PHONY: clean
clean: preprocess_clean
	@$(PRINTSCR) ---- Cleaning module $(MODULE) ----
	@$(REMOVE) __objs_$(PLATFORM)
	@for s in $(MODULE_SUB_DIRS); \
	do \
		if (!(	$(PRINTSCR) ---- Cleaning module subfolder $$s ---- && \
			$(CHDIR) $(MODULE_DIR)/$$s && \
			MODULE_SUB_DIR=$(MODULE_DIR)/$$s \
			$(MAKE) -f build.mk clean )) \
		then \
			exit -1; \
		fi; \
	done
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

# What to link
export MODULE_OBJS=`$(PRINT) __objs_$(PLATFORM)`

# Final link stage.  Link all built objects
-include $(BASE_DIR)/Build/link-$(PLATFORM).mk
.PHONY: output
output: $(strip $(MODULE_OUTPUT))

