// test_main.cpp - Simple test framework main
// SPDX-License-Identifier: MIT

#include <iostream>
#include <vector>
#include <functional>
#include <string>

struct TestCase {
    std::string name;
    std::function<bool()> test;
};

std::vector<TestCase>& GetTests() {
    static std::vector<TestCase> tests;
    return tests;
}

void RegisterTest(const std::string& name, std::function<bool()> test) {
    GetTests().push_back({name, test});
}

int main() {
    std::cout << "AES67 Virtual Soundcard - Unit Tests\n";
    std::cout << "=====================================\n\n";
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : GetTests()) {
        std::cout << "Running: " << test.name << "... ";
        std::cout.flush();
        
        try {
            if (test.test()) {
                std::cout << "✓ PASS\n";
                passed++;
            } else {
                std::cout << "✗ FAIL\n";
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ EXCEPTION: " << e.what() << "\n";
            failed++;
        }
    }
    
    std::cout << "\n=====================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    
    return failed > 0 ? 1 : 0;
}
