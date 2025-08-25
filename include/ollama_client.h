#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <string>
#include <vector>
#include "model_file.h"

/**
 * @brief Structure to represent an Ollama model with its metadata
 */
struct OllamaModel {
    std::string name;           // Model name with tag (e.g., "llama3:8b")
    std::string displayName;    // User-friendly display name
    std::string modifiedAt;     // Last modified timestamp
    long long size;             // Model size in bytes
    std::string digest;         // Model digest
    std::string format;         // Model format (e.g., "gguf")
    std::string family;         // Model family (e.g., "llama")
    std::string parameterSize;  // Parameter size (e.g., "8B")
    std::string quantization;   // Quantization level

    /**
     * @brief Default constructor
     */
    OllamaModel();

    /**
     * @brief Constructor with all parameters
     */
    OllamaModel(const std::string& name,
                const std::string& displayName,
                const std::string& modifiedAt,
                long long size,
                const std::string& digest,
                const std::string& format,
                const std::string& family,
                const std::string& parameterSize,
                const std::string& quantization);

    /**
     * @brief Get a formatted string representation of the model size
     * @return Formatted size string (e.g., "4.2 GB")
     */
    std::string getFormattedSize() const;

    /**
     * @brief Check if this model is the same as another (based on name and digest)
     * @param other The other model to compare with
     * @return True if models are the same, false otherwise
     */
    bool isSameAs(const OllamaModel& other) const;
};

/**
 * @brief Client for interacting with Ollama API
 */
class OllamaClient {
public:
    /**
     * @brief Check if Ollama server is running
     * @return True if server is running, false otherwise
     */
    static bool isServerRunning();

    /**
     * @brief List locally available models
     * @return Vector of OllamaModel objects representing locally available models
     */
    static std::vector<OllamaModel> listLocalModels();

    /**
     * @brief Pull a model from Ollama registry
     * @param modelName The name of the model to pull (e.g., "llama3:8b")
     * @return True if pull was successful, false otherwise
     */
    static bool pullModel(const std::string& modelName);

    /**
     * @brief Search models by name pattern
     * @param query The search query string
     * @return Vector of OllamaModel objects matching the query
     */
    static std::vector<OllamaModel> searchModels(const std::string& query);

    /**
     * @brief Parse model name and tag from a full model name
     * @param fullName The full model name (e.g., "llama3:8b")
     * @return Pair containing model name and tag
     */
    static std::pair<std::string, std::string> parseModelName(const std::string& fullName);

    /**
     * @brief Convert OllamaModel to ModelFile for compatibility with existing system
     * @param ollamaModel The Ollama model to convert
     * @return ModelFile object compatible with existing system
     */
    static ModelFile convertToModelFile(const OllamaModel& ollamaModel);

private:
    static const std::string API_BASE_URL;
    static const std::string USER_AGENT;
    static const int REQUEST_TIMEOUT;
    static const int MAX_RETRIES;
    static const int RETRY_DELAY_MS;
    static const int CACHE_EXPIRATION_TIME;

    /**
     * @brief Show progress during model pull operation
     * @param modelName The name of the model being pulled
     * @param status Current status message
     * @param completed Number of bytes completed
     * @param total Total number of bytes
     */
    static void showPullProgress(const std::string& modelName,
                                const std::string& status,
                                long long completed,
                                long long total);

    /**
     * @brief Parse JSON response from Ollama API
     * @param jsonData The JSON data to parse
     * @return Vector of OllamaModel objects
     */
    static std::vector<OllamaModel> parseModelList(const std::string& jsonData);

    /**
     * @brief Parse a single model from JSON data
     * @param modelJson The JSON data for a single model
     * @return OllamaModel object
     */
    static OllamaModel parseModel(const std::string& modelJson);

    /**
     * @brief Handle streaming response from pull operation
     * @param responseStream The streaming response from HttpClient
     * @param modelName The name of the model being pulled
     * @return True if pull completed successfully, false otherwise
     */
    static bool handlePullStream(const std::string& responseStream, const std::string& modelName);

    /**
     * @brief Validate model name format
     * @param modelName The model name to validate
     * @return True if valid, false otherwise
     */
    static bool validateModelName(const std::string& modelName);

    /**
     * @brief Format model size in human readable format
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "4.2 GB")
     */
    static std::string formatSize(long long bytes);

    /**
     * @brief Extract model family from model name
     * @param modelName The model name
     * @return Model family string
     */
    static std::string extractFamily(const std::string& modelName);
};

#endif // OLLAMA_CLIENT_H