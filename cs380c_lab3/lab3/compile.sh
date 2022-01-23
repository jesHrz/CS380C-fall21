#!/usr/bin/env bash

# A script that builds your compiler.
cd format
mkdir -p build
cmake . -Bbuild
make -C build
cp build/ssa ..
cp build/nonssa ..
cd ..