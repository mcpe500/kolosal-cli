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
     * @param repoId Optional Hugging Face repository URL or ID
     * @return Exit code (0 for success, non-zero for error)
     */
    int run(const std::string& repoId = "");
    
    /**
     * @brief Cleanup resources before exit
     */
    void cleanup();

private:
    /**
     * @brief Parse Hugging Face repository URL or ID to extract model ID
     * @param input URL or ID from command line
     * @return Normalized model ID (e.g., "owner/model-name") or empty string if invalid
     */
    std::string parseRepositoryInput(const std::string& input);
    
    /**
     * @brief Validate if a model ID has the correct format
     * @param modelId The model ID to validate
     * @return True if valid, false otherwise
     */
    bool isValidModelId(const std::string& modelId);
    
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
    std::vector<ModelFile> generateSampleFiles(const std::string& modelId);
};

#endif // KOLOSAL_CLI_H
