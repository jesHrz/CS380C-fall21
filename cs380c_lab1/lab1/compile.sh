#!/usr/bin/env bash

# Script to compile your source
# g++ -std=c++14 -g -o translator main.cpp misc.cpp
mkdir -p build
cd build
cmake ..
make
cp convert ..