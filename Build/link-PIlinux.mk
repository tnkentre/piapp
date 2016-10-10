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

# version: $LastChangedRevision: 8475 $
# author: zkhan


comma=,

# Where and how to find all libraries
COMMON_PATHS= \
	$(addprefix -L,$(BUILD_DIR)) \
	$(addprefix -L,$(OS_BUILD_DIRS)) \
	-Wl,--whole-archive \
	$(addprefix $(BUILD_DIR)/, $(MODULE_ARCHIVES)) \
	-Wl,--no-whole-archive \
	$(MODULE_OBJS) \
	__version_info_$(PLATFORM).o \
	$(MODULE_EXTERN_LIBS) -lrt -lc -lpthread

# This will generate .a archive files
$(MODULE_ARCHIVE): \
	$(addprefix $(BUILD_DIR)/,$(MODULE_ARCHIVES))
	@$(PRINTSCR) "AR    [$(MODULE_ARCHIVE)]"
	$(AR) r \
		$@ \
		$(MODULE_OBJS) \
		$(addprefix $(BUILD_DIR)/,$(MODULE_ARCHIVES)) \
		__version_info_$(PLATFORM).o \
		$(MODULE_EXTERN_LIBS)

# This will generate .so files
$(MODULE_DYNAMIC): \
	$(addprefix $(BUILD_DIR)/,$(MODULE_ARCHIVES)) \
	$(addprefix $(BUILD_DIR)/,$(MODULE_DYNAMICS)) \
	$(MODULE_EXTERN_LIBS)
	@$(PRINTSCR) "LD    [$(MODULE_DYNAMIC)]"
	$(CC) \
		-shared \
		$(ALL_CFLAGS) \
		$(addprefix -Wl$(comma),-Map=$(MODULE_MAP) $(ALL_LDFLAGS)) \
		$(addprefix -Wl$(comma),$(MODULE_LDFLAGS)) \
		$(COMMON_PATHS) \
		$(addprefix -l,$(MODULE_OS_LIBS)) \
		$(addprefix -l,$(MODULE_DYNAMICS)) \
		-o $@
	$(SRP) $@

# This will generate executable files
$(MODULE_EXE): \
	$(addprefix $(BUILD_DIR)/,$(MODULE_ARCHIVES)) \
	$(addprefix $(BUILD_DIR)/,$(MODULE_DYNAMICS)) \
	$(MODULE_EXTERN_LIBS)
	@$(PRINTSCR) "LD    [$(MODULE_EXE)]"
	$(CC) \
		$(ALL_CFLAGS) \
		$(addprefix -Wl$(comma),-Map=$(MODULE_MAP) $(ALL_LDFLAGS)) \
		$(addprefix -Wl$(comma),$(MODULE_LDFLAGS)) \
		$(COMMON_PATHS) \
		$(addprefix -l,$(MODULE_OS_LIBS)) \
		$(addprefix -l,$(MODULE_DYNAMICS)) \
		-o $@
	$(SRP) $@
		