#!/usr/bin/env bash
CSC=../../cs380c_lab1/src/csc
THREE_ADDR_TO_C_TRANSLATOR=../../cs380c_lab1/lab1/convert
OPT=../lab2/main.py

[ $# -ne 1 ] && { echo "Usage $0 PROGRAM" >&2; exit 1; }


PROGRAM=$1
echo $PROGRAM
${CSC} ../../cs380c_lab1/examples/${PROGRAM}.c | ${OPT} -r | ${OPT} -l | ${THREE_ADDR_TO_C_TRANSLATOR} > build/${PROGRAM}.3addr.c
# ${CSC} ../../cs380c_lab1/examples/${PROGRAM}.c | ${OPT} -r | ${OPT} -l | ${THREE_ADDR_TO_C_TRANSLATOR}
gcc ../../cs380c_lab1/examples/${PROGRAM}.c -o build/${PROGRAM}.gcc.bin
gcc build/${PROGRAM}.3addr.c -o build/${PROGRAM}.3addr.bin
./build/${PROGRAM}.gcc.bin > build/${PROGRAM}.gcc.txt
./build/${PROGRAM}.3addr.bin > build/${PROGRAM}.3addr.txt
md5 build/${PROGRAM}.gcc.txt build/${PROGRAM}.3addr.txt