#include "loading_animation.h"
#include <iostream>
#include <iomanip>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

const int LoadingAnimation::INTERVAL_MS = 30;

LoadingAnimation::LoadingAnimation(const std::string& message) 
    : message(message), running(false), currentFrame(0) {
    initializeFrames();
    
#ifdef _WIN32
    // Enable UTF-8 console output for Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Enable virtual terminal processing for better console support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

LoadingAnimation::~LoadingAnimation() {
    stop();
    // Ensure cursor is always visible when object is destroyed
    showCursor();
}

void LoadingAnimation::initializeFrames() {
    // Optimized frame sequence for smoother animation
    frames = {
        "⢀⠀", "⡀⠀", "⠄⠀", "⢂⠀", "⡂⠀", "⠅⠀", "⢃⠀", "⡃⠀", 
        "⠍⠀", "⢋⠀", "⡋⠀", "⠍⠁", "⢋⠁", "⡋⠁", "⠍⠉", "⠋⠉", 
        "⠉⠙", "⠉⠩", "⠈⢙", "⠈⡙", "⢈⠩", "⡀⢙", "⠄⡙", "⢂⠩",
        "⡂⢘", "⠅⡘", "⢃⠨", "⡃⢐", "⠍⡐", "⢋⠠", "⡋⢀", "⠍⡁",
        "⢋⠁", "⡋⠁", "⠍⠉", "⠋⠉", "⠉⠙", "⠉⠩", "⠈⢙", "⠈⡙",
        "⠈⠩", "⠀⢙", "⠀⡙", "⠀⠩", "⠀⢘", "⠀⡘", "⠀⠨", "⠀⢐",
        "⠀⡐", "⠀⠠", "⠀⢀", "⠀⡀"
    };
}

void LoadingAnimation::start() {
    if (running.load()) {
        return; // Already running
    }
    
    running.store(true);
    currentFrame = 0;
    
    // Hide cursor to prevent flickering
    hideCursor();
    
    // Small delay before starting animation for smoother UX
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    animationThread = std::thread(&LoadingAnimation::animationLoop, this);
}

void LoadingAnimation::stop() {
    if (!running.load()) {
        return; // Not running
    }
    
    running.store(false);
    
    if (animationThread.joinable()) {
        animationThread.join();
    }
    
    clearLine();
    // Show cursor again
    showCursor();
}

void LoadingAnimation::complete(const std::string& completionMessage) {
    stop();    // Brief pause before showing completion for smoother transition
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::cout << "Done: " << completionMessage << std::endl;
}

bool LoadingAnimation::isRunning() const {
    return running.load();
}

void LoadingAnimation::animationLoop() {
    auto nextFrame = std::chrono::steady_clock::now();
    
    while (running.load()) {
        displayFrame();
        
        // Calculate next frame time for consistent timing
        nextFrame += std::chrono::milliseconds(INTERVAL_MS);
        std::this_thread::sleep_until(nextFrame);
        
        currentFrame = (currentFrame + 1) % frames.size();
    }
}

void LoadingAnimation::clearLine() {
    std::cout << "\r" << std::string(100, ' ') << "\r" << std::flush;
}

void LoadingAnimation::displayFrame() {
    // Move cursor to beginning of line and display frame
    std::cout << "\r" << frames[currentFrame] << " " << message << "...   " << std::flush;
}

void LoadingAnimation::hideCursor() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hOut, &cursorInfo);
#else
    // ANSI escape sequence to hide cursor
    std::cout << "\033[?25l" << std::flush;
#endif
}

void LoadingAnimation::showCursor() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hOut, &cursorInfo);
#else
    // ANSI escape sequence to show cursor
    std::cout << "\033[?25h" << std::flush;
#endif
}
