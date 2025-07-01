#ifndef KOLOSAL_CLI_H
#define KOLOSAL_CLI_H

#include <string>
#include <vector>
#include <memory>
#include "model_file.h"
#include "kolosal_server_client.h"
#include "command_manager.h"

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
     * @brief Handle direct GGUF file URL input
     * @param url Direct URL to a GGUF file
     * @return True if download completed successfully, false otherwise
     */
    bool handleDirectGGUFUrl(const std::string& url);
    
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
    std::unique_ptr<CommandManager> m_commandManager; // Command manager for chat commands
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
     * @brief Get user input with real-time command autocomplete
     * @param prompt The prompt to display
     * @return The complete user input string
     */
    std::string getInputWithRealTimeAutocomplete(const std::string& prompt);
    
    /**
     * @brief Update real-time command suggestions display
     * @param input Current input string
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Reference to row where suggestions start
     * @param prompt The input prompt
     */
    void updateRealTimeSuggestions(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt);
    
    /**
     * @brief Clear command suggestions from display
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Row where suggestions start
     * @param prompt The input prompt
     * @param input Current input string
     */
    void clearSuggestions(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input);
    
    /**
     * @brief Clear current input line for rewriting
     * @param input Current input string to clear
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Row where suggestions start
     * @param prompt The input prompt
     */
    void clearCurrentInput(const std::string& input, bool& showingSuggestions, int suggestionStartRow, const std::string& prompt);
    
    /**
     * @brief Force clear any lingering command suggestions from console
     */
    void forceClearSuggestions();
    
    /**
     * @brief Display hint text in the input area
     * @param hintText The hint text to display
     * @param showingHint Reference to bool tracking hint visibility
     */
    void displayHintText(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Clear hint text from the input area
     * @param hintText The hint text that was displayed
     * @param showingHint Reference to bool tracking hint visibility
     */
    void clearHintText(bool& showingHint);
    
    /**
     * @brief Display hint text in the input area (Linux implementation)
     * @param hintText The hint text to display
     * @param showingHint Reference to bool tracking hint visibility
     */
    void displayHintTextLinux(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Clear hint text from the input area (Linux implementation)
     * @param hintText The hint text that was displayed
     * @param showingHint Reference to bool tracking hint visibility
     */
    void clearHintTextLinux(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Update real-time suggestions for Linux
     * @param input Current input string
     * @param showingSuggestions Reference to bool tracking suggestion visibility
     * @param suggestionStartRow Starting row for suggestions
     * @param prompt Input prompt string
     */
    void updateRealTimeSuggestionsLinux(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt);
    
    /**
     * @brief Clear suggestions for Linux
     * @param showingSuggestions Reference to bool tracking suggestion visibility
     * @param suggestionStartRow Starting row for suggestions
     * @param prompt Input prompt string
     * @param input Current input string
     */
    void clearSuggestionsLinux(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input);
    
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
