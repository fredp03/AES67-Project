// test_ring_buffer.cpp - Ring buffer tests
// SPDX-License-Identifier: MIT

#include "AES67_RingBuffer.h"
#include <functional>
#include <cstring>
#include <thread>

extern void RegisterTest(const std::string& name, std::function<bool()> test);

using namespace AES67;

// Test basic write and read
bool test_ring_buffer_basic() {
    AudioRingBuffer ring(1024);
    
    int32_t writeData[16];
    int32_t readData[16];
    
    // Fill with pattern
    for (int i = 0; i < 16; ++i) {
        writeData[i] = i * 1000;
    }
    
    // Write
    size_t written = ring.Write(writeData, 16);
    if (written != 16) return false;
    
    // Read back
    size_t read = ring.Read(readData, 16);
    if (read != 16) return false;
    
    // Verify
    return std::memcmp(writeData, readData, 16 * sizeof(int32_t)) == 0;
}

// Test available space tracking
bool test_ring_buffer_available() {
    AudioRingBuffer ring(128);  // Will be 128 (power of 2)
    
    int32_t data[50];
    std::memset(data, 0, sizeof(data));
    
    // Initially empty
    if (ring.ReadAvailable() != 0) return false;
    if (ring.WriteAvailable() != 127) return false;  // capacity - 1
    
    // Write 50
    ring.Write(data, 50);
    if (ring.ReadAvailable() != 50) return false;
    if (ring.WriteAvailable() != 77) return false;  // 127 - 50
    
    // Read 30
    ring.Read(data, 30);
    if (ring.ReadAvailable() != 20) return false;
    if (ring.WriteAvailable() != 107) return false;  // 127 - 20
    
    return true;
}

// Test wrap-around
bool test_ring_buffer_wraparound() {
    AudioRingBuffer ring(100);
    
    int32_t writeData[80];
    int32_t readData[80];
    
    for (int i = 0; i < 80; ++i) {
        writeData[i] = i;
    }
    
    // Write 80, read 60, write 80 (should wrap)
    ring.Write(writeData, 80);
    ring.Read(readData, 60);
    
    // This write should wrap around
    ring.Write(writeData, 80);
    
    // Read remaining 20 from first write
    size_t read1 = ring.Read(readData, 20);
    if (read1 != 20) return false;
    
    // Read 80 from second write
    size_t read2 = ring.Read(readData, 80);
    if (read2 != 80) return false;
    
    // Verify last read data
    return std::memcmp(writeData, readData, 80 * sizeof(int32_t)) == 0;
}

// Test overrun protection (can't write more than capacity)
bool test_ring_buffer_overrun() {
    AudioRingBuffer ring(128);  // Will be 128 (power of 2)
    
    int32_t data[200];
    std::memset(data, 0, sizeof(data));
    
    // Try to write more than capacity - 1 (ring always leaves one slot empty)
    size_t written = ring.Write(data, 150);
    
    // Should only write up to capacity - 1
    return written == 127;
}

// Test underrun (reading when empty)
bool test_ring_buffer_underrun() {
    AudioRingBuffer ring(128);
    
    int32_t data[50];
    
    // Try to read from empty buffer
    size_t read = ring.Read(data, 50);
    
    // Should read nothing
    return read == 0;
}

// Test reset
bool test_ring_buffer_reset() {
    AudioRingBuffer ring(128);  // Will be 128 (power of 2)
    
    int32_t data[50];
    std::memset(data, 0, sizeof(data));
    
    ring.Write(data, 50);
    ring.Reset();
    
    return ring.ReadAvailable() == 0 && ring.WriteAvailable() == 127;
}

// Simple producer-consumer test (single producer, single consumer)
bool test_ring_buffer_spsc() {
    AudioRingBuffer ring(1000);
    std::atomic<bool> running{true};
    std::atomic<int> totalWritten{0};
    std::atomic<int> totalRead{0};
    
    // Producer thread
    std::thread producer([&]() {
        int32_t data[10];
        for (int i = 0; i < 100; ++i) {
            for (int j = 0; j < 10; ++j) {
                data[j] = i * 10 + j;
            }
            size_t written = ring.Write(data, 10);
            totalWritten += written;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        int32_t data[10];
        while (totalRead < 1000) {
            size_t read = ring.Read(data, 10);
            totalRead += read;
            if (read == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    return totalWritten == 1000 && totalRead == 1000;
}

// Register all ring buffer tests
static struct RingBufferTestRegistrar {
    RingBufferTestRegistrar() {
        RegisterTest("RingBuffer: Basic write/read", test_ring_buffer_basic);
        RegisterTest("RingBuffer: Available space tracking", test_ring_buffer_available);
        RegisterTest("RingBuffer: Wrap-around", test_ring_buffer_wraparound);
        RegisterTest("RingBuffer: Overrun protection", test_ring_buffer_overrun);
        RegisterTest("RingBuffer: Underrun handling", test_ring_buffer_underrun);
        RegisterTest("RingBuffer: Reset", test_ring_buffer_reset);
        RegisterTest("RingBuffer: SPSC threading", test_ring_buffer_spsc);
    }
} ringBufferTestRegistrar;
