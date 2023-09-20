cmake_minimum_required(VERSION 3.20)
project(MultiClientServer1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(
  ${PROJECT_NAME} 
  main.cpp
  server.cpp
  server-base.cpp
)

target_link_libraries(${PROJECT_NAME} pthread)