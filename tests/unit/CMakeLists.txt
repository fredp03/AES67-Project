# AES67 Unit Tests
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)
project(AES67Tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add test executable
add_executable(aes67_tests
    test_ring_buffer.cpp
    test_rtp_codec.cpp
    test_ptp_time.cpp
    test_main.cpp
)

# Include directories
target_include_directories(aes67_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../../driver/include
    ${CMAKE_SOURCE_DIR}/../../engine/include
)

# Link with engine library
set(ENGINE_LIB_DIR ${CMAKE_SOURCE_DIR}/../../engine/build)
target_link_libraries(aes67_tests 
    ${ENGINE_LIB_DIR}/libaes67_engine.a
    pthread
)

# Enable testing
enable_testing()
add_test(NAME AES67UnitTests COMMAND aes67_tests)

message(STATUS "Configured unit tests")
