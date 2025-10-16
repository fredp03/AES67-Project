// AES67_RingBuffer.cpp - Implementation (mostly header-only)
// SPDX-License-Identifier: MIT

#include "AES67_RingBuffer.h"

// Template instantiations if needed
namespace AES67 {
    // Explicit instantiation for int32_t (audio samples)
    template class RingBuffer<int32_t>;
}
