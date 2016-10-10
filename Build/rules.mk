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

# version: $LastChangedRevision: 11365 $
# author: zkhan


# To compile these
export OBJS=\
	$(CSRC:.c=_$(PLATFORM)_c.o) \
	$(CXXSRC:.cpp=_$(PLATFORM)_cxx.o) \
	$(SSRC:.asm=_$(PLATFORM)_asm.o) \
	$(YSRC:.y=_$(PLATFORM).tab.o) \
	$(LSRC:.l=_$(PLATFORM).l.o) \
	$(QTSRC:.h=_$(PLATFORM).moc.o) \
   $(QRC:.qrc=_$(PLATFORM).qrc.o)

# Include dependencies
-include $(OBJS:.o=.d)

# Suffix rules
.SUFFIXES:
%_$(PLATFORM)_c.o:%.c
%_$(PLATFORM)_asm.o:%.asm
%_$(PLATFORM)_cxx.o:%.cpp
%_$(PLATFORM).tab.o:%.y
%_$(PLATFORM).l.o:%.l
%_$(PLATFORM).moc.o:%.h
%_$(PLATFORM).qrc.o:%.qrc

DEFAULT_INC = -I$(INCLUDE_DIR)

# special meta object compiler for some Qt classes
   
%_$(PLATFORM).moc.o:%.h
	@$(PRINTSCR) "MOC   [.cpp]  $*"
	$(MOC) $*.h > $*.moc.cpp
	@$(PRINTSCR) "C++   [.o]  $*.moc.cpp"
	@$(CXX) $(TOOLS_CXXFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CXXFLAGS) $(MODULE_CXXFLAGS) $(DEFAULT_INC) -c $*.moc.cpp
	@$(MOVE) $*.moc.o $*_$(PLATFORM).moc.o
	@$(PRINTSCR) "CPP   [.d]    $*.moc.cpp"
	@$(DEP) $(ALL_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -x c++ $*.moc.cpp $*.moc.d
	@$(MOVE) $*.moc.d $*.moc.d.tmp
	@$(SED) -e 's|.*:|$*_$(PLATFORM).moc.o:|' < $*.moc.d.tmp > $*.moc.d
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $*.moc.d.tmp | fmt -1 | \
		$(SED) -e 's/^ *//' -e 's/$$/:/' >> $*.moc.d
	@$(REMOVE) $*.moc.d.tmp
	@$(MOVE) $*.moc.d $*_$(PLATFORM).moc.d
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

%_$(PLATFORM).qrc.o:%.qrc
	@$(PRINTSCR) "RCC   [.cpp]  $*"
	$(RCC) $*.qrc > $*.qrc.cpp
	@$(PRINTSCR) "C++   [.o]  $*.qrc.cpp"
	@$(CXX) $(TOOLS_CXXFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CXXFLAGS) $(MODULE_CXXFLAGS) $(DEFAULT_INC) -c $*.qrc.cpp
	@$(MOVE) $*.qrc.o $*_$(PLATFORM).qrc.o
	@$(PRINTSCR) "CPP   [.d]    $*.qrc.cpp"
	@$(DEP) $(ALL_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -x c++ $*.qrc.cpp $*.qrc.d
	@$(MOVE) $*.qrc.d $*.qrc.d.tmp
	@$(SED) -e 's|.*:|$*_$(PLATFORM).qrc.o:|' < $*.qrc.d.tmp > $*.qrc.d
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $*.qrc.d.tmp | fmt -1 | \
		$(SED) -e 's/^ *//' -e 's/$$/:/' >> $*.qrc.d
	@$(REMOVE) $*.qrc.d.tmp
	@$(MOVE) $*.qrc.d $*_$(PLATFORM).qrc.d
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

# C Source Rules
# C dependencies
# --------

%_$(PLATFORM)_c.o:%.c
	@$(PRINTSCR) "CC    [.o]  $*.c"
	@$(CC) $(TOOLS_CFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -c $*.c
	@$(MOVE) $*.o $*_$(PLATFORM)_c.o
	@$(PRINTSCR) "CPP   [.d]  $*.c"
	@$(DEP) $(ALL_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -x c $*.c $*.d
	@$(MOVE) $*.d $*.d.tmp
	@$(SED) -e 's|.*:|$*_$(PLATFORM)_c.o:|' < $*.d.tmp > $*.d
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		$(SED) -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@$(REMOVE) $*.d.tmp
	@$(MOVE) $*.d $*_$(PLATFORM)_c.d
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

# CXX Source Rules
# CXX dependencies
# --------
#
%_$(PLATFORM)_cxx.o:%.cpp
	@$(PRINTSCR) "C++   [.o]  $*.cpp"
	@$(CXX) $(TOOLS_CXXFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CXXFLAGS) $(MODULE_CXXFLAGS) $(DEFAULT_INC) -c $*.cpp
	@$(MOVE) $*.o $*_$(PLATFORM)_cxx.o
	@$(PRINTSCR) "CPP   [.d]  $*.cpp"
	@$(DEP) $(ALL_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -x c++ $*.cpp $*.d
	@$(MOVE) $*.d $*.d.tmp
	@$(SED) -e 's|.*:|$*_$(PLATFORM)_cxx.o:|' < $*.d.tmp > $*.d
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		$(SED) -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@$(REMOVE) $*.d.tmp
	@$(MOVE) $*.d $*_$(PLATFORM)_cxx.d
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

# ASM Rules
# --------
#
%_$(PLATFORM)_asm.o:%.asm
	@$(PRINTSCR) "ASM   [.o]  $*.asm"
	@$(AS) $(TOOLS_ASFLAGS) $(ALL_ASFLAGS) $(MODULE_TOOLS_ASFLAGS) $(MODULE_ASFLAGS) $(DEFAULT_INC) $*.asm -o $*.o
	@$(MOVE) $*.o $*_$(PLATFORM)_asm.o
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

#
# YACC
#
%_$(PLATFORM).tab.o:%.y
	@$(PRINTSCR) "YACC  [.o]  $*.y"
	$(YACC) -d $*.y
	@$(CC) $(TOOLS_CFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -c $*.tab.c
	@$(MOVE) $*.tab.o $*_$(PLATFORM).tab.o
	@$(REMOVE) $(MODULE_OUTPUT_ALL)

#
# LEX
#
%_$(PLATFORM).l.o:%.l
	@$(PRINTSCR) "LEX   [.o]  $*.l"
	$(LEX) $*.l
	$(MOVE) lex.yy.c $*.l.c
	@$(CC) $(TOOLS_CFLAGS) $(ALL_CFLAGS) $(MODULE_TOOLS_CFLAGS) $(MODULE_CFLAGS) $(DEFAULT_INC) -c $*.l.c
	@$(MOVE) $*.l.o $*_$(PLATFORM).l.o
	@$(REMOVE) $(MODULE_OUTPUT_ALL)


# Headers 
# --------
#
.PHONY: headers
headers: $(addprefix $(INCLUDE_DIR)/,$(HEADERS))
$(INCLUDE_DIR)/%: %
	@$(PRINTSCR) "COPY  [.h]  $^"
	$(COPY) $^ $(INCLUDE_DIR)
vpath %.h .

# Make all outputs
all: $(OBJS)
	@$(PRINTSCR) $(addprefix $(MODULE_SUB_DIR)/,$(OBJS)) >> \
		$(MODULE_DIR)/__objs_$(PLATFORM)

# Cleanup
.PHONY: clean 
clean:
	$(REMOVE) $(OBJS:.o=.d)
	$(REMOVE) $(OBJS)
	$(REMOVE) $(YSRC:.y=.tab.c) $(YSRC:.y=.tab.h) $(LSRC:.l=.l.c) $(QTSRC:.h=.moc.cpp) $(QRC:.qrc=.qrc.cpp)
	$(REMOVE) $(addprefix $(INCLUDE_DIR)/,$(HEADERS))
