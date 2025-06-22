#ifndef KOLOSAL_CLI_H
#define KOLOSAL_CLI_H

#include <string>
#include <vector>
#include <memory>
#include "model_file.h"
#include "kolosal_server_client.h"

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
    
    /**
     * @brief Stop the background Kolosal server
     * @return True if server was stopped successfully, false otherwise
     */
    bool stopBackgroundServer();

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
    std::vector<std::string> generateSampleModels();    /**
     * @brief Generate fallback sample files when API fails
     * @param modelId The model ID to generate sample files for
     * @return Vector of sample ModelFile objects
     */
    std::vector<ModelFile> generateSampleFiles(const std::string& modelId);

private:
    std::unique_ptr<KolosalServerClient> m_serverClient;
    std::vector<std::string> m_activeDownloads; // Track active download IDs
    static KolosalCLI* s_instance; // For signal handling
    
    /**
     * @brief Initialize and start the Kolosal server
     * @return True if server is ready, false otherwise
     */
    bool initializeServer();
    
    /**
     * @brief Process model selection and send to server for download
     * @param modelId The selected model ID
     * @param modelFile The selected model file
     * @return True if download completed successfully, false otherwise
     */
    bool processModelDownload(const std::string& modelId, const ModelFile& modelFile);
    
    /**
     * @brief Cancel all active downloads
     */
    void cancelActiveDownloads();
    
    /**
     * @brief Signal handler for graceful shutdown
     * @param signal Signal number
     */
    static void signalHandler(int signal);
};

#endif // KOLOSAL_CLI_H
