cmake_minimum_required(VERSION 3.16)
project(nova_compiler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(novac
    src/main.cpp
    src/lexer.cpp
    src/dfa.cpp
)
target_include_directories(novac PRIVATE src)
