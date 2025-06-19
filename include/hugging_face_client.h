#ifndef HUGGING_FACE_CLIENT_H
#define HUGGING_FACE_CLIENT_H

#include <string>
#include <vector>
#include "model_file.h"

/**
 * @brief Client for interacting with Hugging Face API
 */
class HuggingFaceClient {
public:
    /**
     * @brief Fetch all Kolosal models from Hugging Face
     * @return Vector of model IDs from the kolosal organization
     */
    static std::vector<std::string> fetchKolosalModels();
    
    /**
     * @brief Fetch .gguf files for a specific model with quantization info
     * @param modelId The model ID (e.g., "kolosal/model-name")
     * @return Vector of ModelFile objects with quantization information
     */
    static std::vector<ModelFile> fetchModelFiles(const std::string& modelId);

private:
    static const std::string API_BASE_URL;
    static const std::string USER_AGENT;
    static const int REQUEST_TIMEOUT;
};

#endif // HUGGING_FACE_CLIENT_H
