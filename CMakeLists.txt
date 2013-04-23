
cmake_minimum_required(VERSION 2.8)

project(nsl)

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

subdirs(NetStalkerLibrary examples)
