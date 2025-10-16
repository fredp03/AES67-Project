cmake_minimum_required(VERSION 3.20)
project(AES67_Engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -Wall -Wextra -Wpedantic
    -O3
    -march=native
  )
endif()

# Engine sources
set(ENGINE_SOURCES
  src/NetworkEngine.cpp
  src/RTPPacketizer.cpp
  src/PTPClient.cpp
  src/JitterBuffer.cpp
  src/SAPAnnouncer.cpp
  src/SDPParser.cpp
)

set(ENGINE_HEADERS
  include/NetworkEngine.h
  include/RTPPacketizer.h
  include/PTPClient.h
  include/JitterBuffer.h
  include/SAPAnnouncer.h
  include/SDPParser.h
  include/RTPTypes.h
  include/PTPTypes.h
)

# Build as static library (linked into driver and UI)
add_library(aes67_engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(aes67_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../driver/include
)

# Install
install(TARGETS aes67_engine
  ARCHIVE DESTINATION lib
)

install(FILES ${ENGINE_HEADERS}
  DESTINATION include/aes67
)
