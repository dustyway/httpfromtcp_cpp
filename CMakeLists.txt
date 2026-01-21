cmake_minimum_required(VERSION 4.1)
project(httpfromtcp_cpp)

set(CMAKE_CXX_STANDARD 98)

add_executable(httpfromtcp_cpp main.cpp)
target_include_directories(httpfromtcp_cpp PUBLIC .)
