cmake_minimum_required(VERSION 3.17)
project(OS_2 C)

set(CMAKE_C_STANDARD 11)

add_executable(main shell.c myshell.c)
