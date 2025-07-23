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

    /**
     * @brief Display server logs in the CLI
     * @return True if logs were retrieved and displayed successfully, false otherwise
     */
    bool showServerLogs();

    /**
     * @brief Display available inference engines in the CLI
     * @return True if engines were retrieved and displayed successfully, false otherwise
     */
    bool showInferenceEngines();

    /**
     * @brief Download an engine file to the executable directory
     * @param engineName Name of the engine to download
     * @param filename The filename of the engine file on Hugging Face
     * @return True if download completed successfully, false otherwise
     */
    bool downloadEngineFile(const std::string& engineName, const std::string& filename);

    /**
     * @brief Get the directory where the kolosal-cli executable is located
     * @return Path to the executable directory
     */
    std::string getExecutableDirectory();

    /**
     * @brief Normalize engine name for cross-platform compatibility
     * @param filename The original filename from Hugging Face repository
     * @return Normalized engine name (removes lib prefix on Linux, removes extension and path)
     * 
     * Examples:
     * - Windows: "llama-cpu.dll" → "llama-cpu"
     * - Linux: "libllama-cpu.so" → "llama-cpu"
     * - Both: "path/to/libllama-vulkan.so" → "llama-vulkan" (on Linux)
     */
    static std::string normalizeEngineName(const std::string& filename);

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
     * @brief Get list of downloaded models from the server
     * @return Vector of model IDs that are available on the server
     */
    std::vector<std::string> getDownloadedModelsFromServer();
    
    /**
     * @brief Get server models that match a specific model pattern
     * @param modelId The model ID to match against (e.g., "microsoft/DialoGPT-medium")
     * @return Vector of ModelFile objects representing models available on the server
     */
    std::vector<ModelFile> getServerModelsForRepo(const std::string& modelId);
    
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
