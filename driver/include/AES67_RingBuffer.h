// AES67_RingBuffer.h - Lock-free SPSC ring buffer for audio transfer
// SPDX-License-Identifier: MIT

#pragma once

#include "AES67_Types.h"
#include <atomic>
#include <cstring>
#include <memory>

namespace AES67 {

/// Lock-free single-producer single-consumer ring buffer
/// Designed for real-time audio: no allocations, no locks in hot path
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(NextPowerOfTwo(capacity))
        , mask_(capacity_ - 1)
        , buffer_(new T[capacity_])
        , readIndex_(0)
        , writeIndex_(0)
    {
        // Zero-initialize buffer
        std::memset(buffer_.get(), 0, capacity_ * sizeof(T));
    }

    ~RingBuffer() = default;

    // Disable copy/move (contains atomics)
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /// Write samples (producer side, e.g., network engine → driver input)
    /// Returns number of frames actually written
    size_t Write(const T* data, size_t frames) noexcept {
        const size_t available = WriteAvailable();
        const size_t toWrite = std::min(frames, available);
        
        if (toWrite == 0) return 0;

        const size_t writeIdx = writeIndex_.load(std::memory_order_relaxed);
        const size_t firstChunk = std::min(toWrite, capacity_ - writeIdx);
        const size_t secondChunk = toWrite - firstChunk;

        // Copy first chunk (up to end of buffer)
        std::memcpy(&buffer_[writeIdx], data, firstChunk * sizeof(T));
        
        // Wrap around if needed
        if (secondChunk > 0) {
            std::memcpy(&buffer_[0], data + firstChunk, secondChunk * sizeof(T));
        }

        // Publish write (release semantics for reader)
        writeIndex_.store((writeIdx + toWrite) & mask_, std::memory_order_release);
        
        return toWrite;
    }

    /// Read samples (consumer side, e.g., driver output → network engine)
    /// Returns number of frames actually read
    size_t Read(T* data, size_t frames) noexcept {
        const size_t available = ReadAvailable();
        const size_t toRead = std::min(frames, available);
        
        if (toRead == 0) return 0;

        const size_t readIdx = readIndex_.load(std::memory_order_relaxed);
        const size_t firstChunk = std::min(toRead, capacity_ - readIdx);
        const size_t secondChunk = toRead - firstChunk;

        // Copy first chunk
        std::memcpy(data, &buffer_[readIdx], firstChunk * sizeof(T));
        
        // Wrap around if needed
        if (secondChunk > 0) {
            std::memcpy(data + firstChunk, &buffer_[0], secondChunk * sizeof(T));
        }

        // Publish read (release semantics for writer)
        readIndex_.store((readIdx + toRead) & mask_, std::memory_order_release);
        
        return toRead;
    }

    /// Peek without consuming (useful for jitter buffer lookahead)
    size_t Peek(T* data, size_t frames) const noexcept {
        const size_t available = ReadAvailable();
        const size_t toPeek = std::min(frames, available);
        
        if (toPeek == 0) return 0;

        const size_t readIdx = readIndex_.load(std::memory_order_acquire);
        const size_t firstChunk = std::min(toPeek, capacity_ - readIdx);
        const size_t secondChunk = toPeek - firstChunk;

        std::memcpy(data, &buffer_[readIdx], firstChunk * sizeof(T));
        if (secondChunk > 0) {
            std::memcpy(data + firstChunk, &buffer_[0], secondChunk * sizeof(T));
        }
        
        return toPeek;
    }

    /// Skip frames without reading (advance read pointer)
    size_t Skip(size_t frames) noexcept {
        const size_t available = ReadAvailable();
        const size_t toSkip = std::min(frames, available);
        
        if (toSkip == 0) return 0;

        const size_t readIdx = readIndex_.load(std::memory_order_relaxed);
        readIndex_.store((readIdx + toSkip) & mask_, std::memory_order_release);
        
        return toSkip;
    }

    /// Write silence (for underrun recovery)
    size_t WriteSilence(size_t frames) noexcept {
        const size_t available = WriteAvailable();
        const size_t toWrite = std::min(frames, available);
        
        if (toWrite == 0) return 0;

        const size_t writeIdx = writeIndex_.load(std::memory_order_relaxed);
        const size_t firstChunk = std::min(toWrite, capacity_ - writeIdx);
        const size_t secondChunk = toWrite - firstChunk;

        std::memset(&buffer_[writeIdx], 0, firstChunk * sizeof(T));
        if (secondChunk > 0) {
            std::memset(&buffer_[0], 0, secondChunk * sizeof(T));
        }

        writeIndex_.store((writeIdx + toWrite) & mask_, std::memory_order_release);
        return toWrite;
    }

    /// Available space for writing
    size_t WriteAvailable() const noexcept {
        const size_t w = writeIndex_.load(std::memory_order_relaxed);
        const size_t r = readIndex_.load(std::memory_order_acquire);
        return capacity_ - 1 - ((w - r) & mask_);
    }

    /// Available data for reading
    size_t ReadAvailable() const noexcept {
        const size_t w = writeIndex_.load(std::memory_order_acquire);
        const size_t r = readIndex_.load(std::memory_order_relaxed);
        return (w - r) & mask_;
    }

    /// Reset buffer (not thread-safe, call when I/O stopped)
    void Reset() noexcept {
        readIndex_.store(0, std::memory_order_relaxed);
        writeIndex_.store(0, std::memory_order_relaxed);
        std::memset(buffer_.get(), 0, capacity_ * sizeof(T));
    }

    size_t Capacity() const noexcept { return capacity_; }

private:
    static size_t NextPowerOfTwo(size_t n) {
        if (n == 0) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    const size_t capacity_;
    const size_t mask_;
    std::unique_ptr<T[]> buffer_;
    
    // Cache-line aligned atomics (avoid false sharing)
    alignas(64) std::atomic<size_t> readIndex_;
    alignas(64) std::atomic<size_t> writeIndex_;
};

/// Convenience typedef for audio frames (interleaved 32-bit samples)
using AudioRingBuffer = RingBuffer<int32_t>;

} // namespace AES67
