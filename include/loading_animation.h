#ifndef LOADING_ANIMATION_H
#define LOADING_ANIMATION_H

#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

/**
 * @brief Loading animation class for displaying animated progress indicators
 */
class LoadingAnimation {
public:
    /**
     * @brief Constructor with custom message
     * @param message The message to display alongside the animation
     */
    LoadingAnimation(const std::string& message = "Loading");
    
    /**
     * @brief Destructor - stops animation if running
     */
    ~LoadingAnimation();
    
    /**
     * @brief Start the loading animation
     */
    void start();
    
    /**
     * @brief Stop the loading animation and clear the line
     */
    void stop();
    
    /**
     * @brief Stop the animation and show completion message
     * @param completionMessage Message to show when completed
     */
    void complete(const std::string& completionMessage = "Done");
    
    /**
     * @brief Check if animation is currently running
     * @return true if animation is active
     */
    bool isRunning() const;

private:
    std::string message;
    std::vector<std::string> frames;
    std::atomic<bool> running;
    std::thread animationThread;
    int currentFrame;
    static const int INTERVAL_MS;
    
    /**
     * @brief Initialize the animation frames
     */
    void initializeFrames();
    
    /**
     * @brief Animation loop function
     */
    void animationLoop();
      /**
     * @brief Clear the current line in console
     */
    void clearLine();
    
    /**
     * @brief Display current frame with message
     */
    void displayFrame();
    
    /**
     * @brief Hide the console cursor
     */
    void hideCursor();
    
    /**
     * @brief Show the console cursor
     */
    void showCursor();
};

#endif // LOADING_ANIMATION_H
