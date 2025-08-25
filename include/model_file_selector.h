#ifndef MODEL_FILE_SELECTOR_H
#define MODEL_FILE_SELECTOR_H

#include <string>
#include <vector>
#include "model_file.h"

/**
 * @brief UI class for handling model file/quantization selection
 */
class ModelFileSelector {
public:
    /**
     * @brief Constructor
     */
    ModelFileSelector();
    
    /**
     * @brief Destructor
     */
    ~ModelFileSelector();
    
    /**
     * @brief Show file selection menu for a specific model
     * @param modelId The model ID to fetch files for
     * @return Selected ModelFile, or empty ModelFile if cancelled
     */
    ModelFile selectModelFile(const std::string& modelId);
    
    /**
     * @brief Show file selection menu for a specific model (with header info)
     * @param modelId The model ID to fetch files for
     * @param headerInfo Additional header information to display
     * @return Selected ModelFile, or empty ModelFile if cancelled
     */
    ModelFile selectModelFile(const std::string& modelId, const std::string& headerInfo);
    
    /**
     * @brief Show file selection menu with fallback to server models
     * @param modelId The model ID to fetch files for
     * @param headerInfo Additional header information to display
     * @param serverFallbackModels Server models to use as fallback when HF fails
     * @return Selected ModelFile, or empty ModelFile if cancelled
     */
    ModelFile selectModelFile(const std::string& modelId, const std::string& headerInfo, const std::vector<ModelFile>& serverFallbackModels);
    
    /**
     * @brief Handle direct GGUF file URL input
     * @param url Direct URL to a GGUF file
     * @return ModelFile object with URL and metadata
     */
    ModelFile handleDirectGGUFUrl(const std::string& url);
    
    /**
     * @brief Display final selection information
     * @param modelId The selected model ID
     * @param modelFile The selected model file
     */
    void showSelectionResult(const std::string& modelId, const ModelFile& modelFile);
    
    /**
     * @brief Handle Ollama model selection
     * @param modelName The Ollama model name
     * @return ModelFile object representing the Ollama model
     */
    ModelFile handleOllamaModel(const std::string& modelName);

private:
    /**
     * @brief Generate fallback sample files when API fails
     * @param modelId The model ID to generate sample files for
     * @return Vector of sample ModelFile objects
     */
    std::vector<ModelFile> generateSampleFiles(const std::string& modelId);
};

#endif // MODEL_FILE_SELECTOR_H
