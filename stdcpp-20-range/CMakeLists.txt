cmake_minimum_required(VERSION 3.20)
project(CPP20_range)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # if compiler doesn't support C++11, stop
set(CMAKE_CXX_EXTENSIONS OFF) # -std=c++ instead of -std=gnu++
set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # compile_commands.json

set(CMAKE_C_COMPILER "/usr/bin/gcc-13")
set(CMAKE_CXX_COMPILER "/usr/bin/g++-13")

add_executable(
  ${PROJECT_NAME} 
  main.cpp
)
