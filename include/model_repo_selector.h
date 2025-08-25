#ifndef MODEL_REPO_SELECTOR_H
#define MODEL_REPO_SELECTOR_H

#include <string>
#include <vector>

/**
 * @brief UI class for handling model repository selection
 */
class ModelRepoSelector {
public:
    /**
     * @brief Constructor
     */
    ModelRepoSelector();
    
    /**
     * @brief Destructor
     */
    ~ModelRepoSelector();
    
    /**
     * @brief Show model selection menu and get user selection
     * @return Selected model ID, or empty string if cancelled
     */
    std::string selectModel();
    
    /**
     * @brief Show model selection menu with available config models at the top
     * @param availableModels Vector of model IDs available in config
     * @return Selected model ID, or empty string if cancelled
     */
    std::string selectModel(const std::vector<std::string>& availableModels);
    
    /**
     * @brief Show model selection menu with config models and fallback for offline mode
     * @param configModels Vector of model IDs available in config
     * @param downloadedModels Vector of model IDs downloaded on the server (used for fallback only)
     * @return Selected model ID, or empty string if cancelled
     */
    std::string selectModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels);
    
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
     * @brief Show Ollama model selection menu
     * @return Selected model ID, or empty string if cancelled
     */
    std::string selectOllamaModel();
};

#endif // MODEL_REPO_SELECTOR_H
