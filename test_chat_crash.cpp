// Simple test to verify LoadingAnimation fixes
#include "src/loading_animation.cpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Testing LoadingAnimation crash fixes..." << std::endl;
    
    // Test 1: Quick destruction
    for (int i = 0; i < 10; i++) {
        std::cout << "Test " << (i+1) << "/10: Quick start/stop..." << std::endl;
        LoadingAnimation anim("Testing");
        anim.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        anim.stop();
    }
    
    // Test 2: Destruction while running
    for (int i = 0; i < 5; i++) {
        std::cout << "Test " << (i+1) << "/5: Destructor while running..." << std::endl;
        {
            LoadingAnimation anim("Testing");
            anim.start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // Let destructor be called while animation is running
        }
    }
    
    // Test 3: Exception simulation
    try {
        std::cout << "Test: Exception handling..." << std::endl;
        LoadingAnimation anim("Testing");
        anim.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        throw std::runtime_error("Test exception");
    } catch (...) {
        std::cout << "Exception caught and handled properly" << std::endl;
    }
    
    std::cout << "All LoadingAnimation tests passed!" << std::endl;
    return 0;
}
