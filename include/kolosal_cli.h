#ifndef KOLOSAL_CLI_H
#define KOLOSAL_CLI_H

#include <string>
#include <vector>
#include "model_file.h"

/**
 * @brief Main application class for Kolosal CLI
 */
class KolosalCLI {
public:
    /**
     * @brief Initialize the application
     */
    void initialize();
    
    /**
     * @brief Run the main application loop
     * @return Exit code (0 for success, non-zero for error)
     */
    int run();
    
    /**
     * @brief Cleanup resources before exit
     */
    void cleanup();

private:
    /**
     * @brief Display welcome message and initialize HTTP client
     */
    void showWelcome();
    
    /**
     * @brief Show model selection menu
     * @return Selected model ID, or empty string if cancelled
     */
    std::string selectModel();
    
    /**
     * @brief Show file selection menu for a specific model
     * @param modelId The model ID to fetch files for
     * @return Selected ModelFile, or empty ModelFile if cancelled
     */
    ModelFile selectModelFile(const std::string& modelId);
    
    /**
     * @brief Display final selection information
     * @param modelId The selected model ID
     * @param modelFile The selected model file
     */
    void showSelectionResult(const std::string& modelId, const ModelFile& modelFile);
    
    /**
     * @brief Generate fallback sample models when API fails
     * @return Vector of sample model IDs
     */
    std::vector<std::string> generateSampleModels();
    
    /**
     * @brief Generate fallback sample files when API fails
     * @param modelId The model ID to generate sample files for
     * @return Vector of sample ModelFile objects
     */
    std::vector<ModelFile> generateSampleFiles(const std::string& modelId);    /**
     * @brief Wait for user to press any key
     */
    void waitForKeyPress();
};

#endif // KOLOSAL_CLI_H
