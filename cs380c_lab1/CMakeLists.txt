cmake_minimum_required(VERSION 3.20)
project(cs380c_lab1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(lab1)

add_executable(cs380c_lab1
        lab1/main.cpp
        lab1/misc.cpp
        lab1/misc.h)
