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

# TODO: Implement other CLI tools
# - aes67-subscribe: Subscribe to a stream
# - aes67-stream: Send test audio stream
# - aes67-ptp-status: Show PTP synchronization status

message(STATUS "Built aes67-list tool")
