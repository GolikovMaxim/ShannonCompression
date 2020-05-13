cmake_minimum_required(VERSION 3.12)
project(Shannon_Compression)

set(CMAKE_CXX_STANDARD 14)

add_executable(Shannon_Compression main.cpp compression.cpp decompression.cpp)