cmake_minimum_required(VERSION 3.20)
project(cs380c_lab3)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(ssa
        ssa.cpp
        Instr.cpp Instr.h misc.cpp misc.h)

add_executable(nonssa
        nonssa.cpp
        Instr.cpp Instr.h misc.cpp misc.h)