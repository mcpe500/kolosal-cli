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
};

#endif // MODEL_REPO_SELECTOR_H
