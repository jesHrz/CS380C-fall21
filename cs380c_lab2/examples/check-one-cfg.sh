#!/usr/bin/env bash

# $Id: check-one.sh 818 2007-09-02 17:45:21Z suriya $

C_SUBSET_COMPILER=../../cs380c_lab1/src/csc
THREE_ADDR_TO_C_TRANSLATOR=../lab2/main.py

[ $# -ne 1 ] && { echo "Usage $0 PROGRAM" >&2; exit 1; }

# set -v

PROGRAM=$1
BASENAME=`basename $PROGRAM .c`
echo $PROGRAM
${C_SUBSET_COMPILER} $PROGRAM > ${BASENAME}.3addr
${THREE_ADDR_TO_C_TRANSLATOR} -c < ${BASENAME}.3addr > ${BASENAME}.cfg
md5 ${BASENAME}.cfg ${BASENAME}.ta.cfg
