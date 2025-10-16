# AES67 CLI Tools
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)
project(AES67Tools CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link to engine library
link_directories(${CMAKE_SOURCE_DIR}/../engine/build)

# aes67-list: List discovered AES67 streams
add_executable(aes67-list aes67-list.cpp)
target_include_directories(aes67-list PRIVATE 
    ${CMAKE_SOURCE_DIR}/../engine/include
    ${CMAKE_SOURCE_DIR}/../driver/include)
target_link_libraries(aes67-list aes67_engine)

# aes67-subscribe: Subscribe to and monitor a specific stream
add_executable(aes67-subscribe aes67-subscribe.cpp)
target_include_directories(aes67-subscribe PRIVATE 
    ${CMAKE_SOURCE_DIR}/../engine/include
    ${CMAKE_SOURCE_DIR}/../driver/include)
target_link_libraries(aes67-subscribe aes67_engine)

# aes67-stream: Transmit audio files as AES67 streams
add_executable(aes67-stream aes67-stream.cpp)
target_include_directories(aes67-stream PRIVATE 
    ${CMAKE_SOURCE_DIR}/../engine/include
    ${CMAKE_SOURCE_DIR}/../driver/include)
target_link_libraries(aes67-stream aes67_engine)

# aes67-monitor: Web-based audio monitoring tool
add_executable(aes67-monitor aes67-monitor.cpp)
target_include_directories(aes67-monitor PRIVATE 
    ${CMAKE_SOURCE_DIR}/../engine/include
    ${CMAKE_SOURCE_DIR}/../driver/include)
target_link_libraries(aes67-monitor aes67_engine)

message(STATUS "Built aes67-list, aes67-subscribe, aes67-stream, aes67-monitor tools")

