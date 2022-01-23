cmake_minimum_required(VERSION 3.20)
project(cs380c_lab1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(convert main.cpp misc.cpp misc.h)
