#!/bin/sh
cp def-reference.mk def-$1.mk
cp link-reference.mk link-$1.mk
cp modules-reference.mk modules-$1.mk
cp platform-reference.mk platform-$1.mk
cp pre-reference.mk pre-$1.mk
sed "s|export TOOL_PATH.*|export TOOL_PATH := $2|" def-$1.mk > tmp
sed "s|export TOOL_PREFIX.*|export TOOL_PREFIX := $3|" tmp > def-$1.mk ; rm tmp


