cmake_minimum_required(VERSION 3.20)
project(AES67VSC_Driver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for hard real-time performance
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -fno-exceptions
    -fno-rtti
    -ffast-math
    -O3
    -march=native
  )
endif()

# Find Core Audio frameworks
find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
find_library(AUDIOUNIT_FRAMEWORK AudioUnit REQUIRED)

# Driver sources
set(DRIVER_SOURCES
  src/AES67_PlugIn.cpp
  src/AES67_Device.cpp
  src/AES67_Stream.cpp
  src/AES67_Clock.cpp
  src/AES67_RingBuffer.cpp
  src/AES67_EngineInterface.cpp
)

set(DRIVER_HEADERS
  include/AES67_PlugIn.h
  include/AES67_Device.h
  include/AES67_Stream.h
  include/AES67_Clock.h
  include/AES67_RingBuffer.h
  include/AES67_EngineInterface.h
  include/AES67_Types.h
)

# Build as bundle (AudioServerPlugIn)
add_library(AES67VSC MODULE ${DRIVER_SOURCES} ${DRIVER_HEADERS})

target_include_directories(AES67VSC PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../engine/include
)

target_link_libraries(AES67VSC PRIVATE
  ${COREAUDIO_FRAMEWORK}
  ${COREFOUNDATION_FRAMEWORK}
  ${AUDIOUNIT_FRAMEWORK}
)

# Bundle properties
set_target_properties(AES67VSC PROPERTIES
  BUNDLE TRUE
  BUNDLE_EXTENSION "driver"
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  MACOSX_BUNDLE_BUNDLE_NAME "AES67 Virtual Soundcard"
  MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
  XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/Signing.entitlements"
)

# Install to HAL plug-ins directory
install(TARGETS AES67VSC
  BUNDLE DESTINATION /Library/Audio/Plug-Ins/HAL
)

# Post-install: restart coreaudiod
install(CODE "execute_process(COMMAND launchctl kickstart -k system/com.apple.audio.coreaudiod)")
