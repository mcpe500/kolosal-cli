#ifndef UNIFIED_MODEL_SELECTOR_H
#define UNIFIED_MODEL_SELECTOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations
class InteractiveList;
class HuggingFaceClient;
class OllamaClient;

/**
 * @brief Structure to represent a unified model from any source
 */
struct UnifiedModel {
    std::string id;              // Unique identifier
    std::string name;            // Display name
    std::string source;          // Source identifier ("huggingface", "ollama", "local", "direct")
    std::string sourceTag;       // Visual tag for display (HF/OL/LOCAL/URL)
    std::string description;     // Model description
    long long size;              // Model size in bytes (-1 if unknown)
    std::string quantization;    // Quantization level (if applicable)
    std::string parameterCount;  // Parameter count (7B, 8B, etc.)
    std::string url;             // Direct URL for models that have it
    std::string format;          // Model format (GGUF, etc.)
    std::vector<std::string> tags; // Model tags for search
};

/**
 * @brief Enhanced UI class for unified model selection with source filtering
 */
class UnifiedModelSelector {
private:
    std::unique_ptr<HuggingFaceClient> huggingFaceClient;
    std::unique_ptr<OllamaClient> ollamaClient;
    std::vector<UnifiedModel> allModels;
    std::vector<UnifiedModel> filteredModels;
    std::string currentFilter;
    std::string currentSourceFilter; // "all", "huggingface", "ollama", "local", "favorites"
    
public:
    /**
     * @brief Constructor
     */
    UnifiedModelSelector();
    
    /**
     * @brief Destructor
     */
    ~UnifiedModelSelector();
    
    /**
     * @brief Show unified model selection interface and get user selection
     * @param configModels Vector of model IDs available in config
     * @param downloadedModels Vector of model IDs downloaded on the server (used for fallback only)
     * @return Selected model ID with source prefix, or empty string if cancelled
     */
    std::string selectModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels);
    
private:
    /**
     * @brief Load models from all available sources
     * @param configModels Vector of model IDs available in config
     * @param downloadedModels Vector of model IDs downloaded on the server (used for fallback only)
     */
    void loadModels(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels);
    
    /**
     * @brief Load Hugging Face models
     */
    void loadHuggingFaceModels();
    
    /**
     * @brief Load Ollama models
     */
    void loadOllamaModels();
    
    /**
     * @brief Load local config models
     * @param configModels Vector of model IDs available in config
     */
    void loadLocalModels(const std::vector<std::string>& configModels);
    
    /**
     * @brief Apply current filters to the model list
     */
    void applyFilters();
    
    /**
     * @brief Convert unified models to display strings for the interactive list
     * @return Vector of strings for display in the interactive list
     */
    std::vector<std::string> convertToDisplayStrings() const;
    
    /**
     * @brief Format a model for display with consistent information
     * @param model The model to format
     * @return Formatted string for display
     */
    std::string formatModelForDisplay(const UnifiedModel& model) const;
    
    /**
     * @brief Show the interactive model selection interface
     * @return Index of selected model, or -1 if cancelled
     */
    int showInteractiveInterface();
};

#endif // UNIFIED_MODEL_SELECTOR_H