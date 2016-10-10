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

# version: $LastChangedRevision: 13335 $
# author: zkhan

# Host commands for aiding in make, serves as a host dependency too 
export SED=sed
export LEX=flex
export YACC=bison
export COPY=cp -ard
export REMOVE=rm -rf
export MOVE=mv -f
export PRINT=cat
export MAKE=make
export MKDIR=mkdir
export CHDIR=cd
export RMDIR=rmdir
export PRINTSCR=echo
export DEP=cpp -M
export DATE=date
export SVNVERS=svn info
