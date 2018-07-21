cmake_minimum_required(VERSION 3.9)
project(altair8800)

set(CMAKE_CXX_STANDARD 17)

add_executable(altair8800 src/main.cpp src/i8008.cpp src/i8008.h src/interfaces/monitor.cpp src/interfaces/monitor.h)