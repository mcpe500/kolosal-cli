#ifndef KOLOSAL_CLI_H
#define KOLOSAL_CLI_H

#include <string>
#include <vector>
#include <memory>
#include "model_file.h"
#include "kolosal_server_client.h"
#include "command_manager.h"
#include "model_repo_selector.h"
#include "model_file_selector.h"
#include "chat_interface.h"

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
     * @brief Check if input is a direct URL to a GGUF file
     * @param input URL from command line
     * @return True if it's a direct GGUF file URL, false otherwise
     */
    bool isDirectGGUFUrl(const std::string& input);
    
    /**
     * @brief Check if input is a local path to a GGUF file
     * @param input Path from command line
     * @return True if it's a local GGUF file path, false otherwise
     */
    bool isLocalGGUFPath(const std::string& input);
    
    /**
     * @brief Handle direct GGUF file URL input
     * @param url Direct URL to a GGUF file
     * @return True if download completed successfully, false otherwise
     */
    bool handleDirectGGUFUrl(const std::string& url);
    
    /**
     * @brief Handle local GGUF file path input
     * @param path Local path to a GGUF file
     * @return True if model was loaded successfully, false otherwise
     */
    bool handleLocalGGUFPath(const std::string& path);
    
    /**
     * @brief Display welcome message and initialize HTTP client
     */
    void showWelcome();    /**
     * @brief Generate fallback sample files when API fails
     * @param modelId The model ID to generate sample files for
     * @return Vector of sample ModelFile objects
    /**
     * @brief Generate fallback sample files when API fails
     * @param modelId The model ID to generate sample files for
     * @return Vector of sample ModelFile objects
     */
    std::vector<ModelFile> generateSampleFiles(const std::string& modelId);

private:
    std::unique_ptr<KolosalServerClient> m_serverClient;
    std::vector<std::string> m_activeDownloads; // Track active download IDs
    std::unique_ptr<CommandManager> m_commandManager; // Command manager for chat commands
    std::unique_ptr<ModelRepoSelector> m_repoSelector; // Model repository selector UI
    std::unique_ptr<ModelFileSelector> m_fileSelector; // Model file selector UI
    std::unique_ptr<ChatInterface> m_chatInterface; // Chat interface UI
    static KolosalCLI* s_instance; // For signal handling
    
    /**
     * @brief Initialize and start the Kolosal server
     * @return True if server is ready, false otherwise
     */
    bool initializeServer();
    
    /**
     * @brief Ensure server is running and connected
     * @return True if server is ready, false otherwise
     */
    bool ensureServerConnection();
    
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
     * @brief Update config.yaml to add a new model entry
     * @param engineId The engine ID to add
     * @param modelPath The local path to the model file
     * @param loadImmediately Whether to load the model immediately on server start
     * @return True if config was updated successfully, false otherwise
     */
    bool updateConfigWithModel(const std::string& engineId, const std::string& modelPath, bool loadImmediately = false);
    
    /**
     * @brief Start an interactive chat session with the given model
     * @param engineId The engine ID to use for chat
     * @return True if chat session completed successfully, false if there was an error
     */
    bool startChatInterface(const std::string& engineId);
    
    /**
     * @brief Get list of available model IDs from config.yaml
     * @return Vector of model IDs found in the config
     */
    std::vector<std::string> getAvailableModelIds();
    
    /**
     * @brief Ensure console encoding is set to UTF-8
     */
    void ensureConsoleEncoding();
    
    /**
     * @brief Signal handler for graceful shutdown
     * @param signal Signal number
     */
    static void signalHandler(int signal);
};

#endif // KOLOSAL_CLI_H
