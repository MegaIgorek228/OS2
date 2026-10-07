cmake_minimum_required(VERSION 3.20)
project(Lab2 C)
set(CMAKE_C_STANDARD 11)

add_executable(parent src/parent.c)
add_executable(child  src/child.c)