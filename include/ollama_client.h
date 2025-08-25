#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

/**
 * @brief Client for interacting with the Ollama API
 */
class OllamaClient {
public:
    /**
     * @brief Constructor
     */
    OllamaClient();
    
    /**
     * @brief Destructor
     */
    ~OllamaClient();
    
    /**
     * @brief Check if the Ollama server is running and healthy
     * @return true if server is healthy, false otherwise
     */
    bool isServerHealthy() const;
    
    /**
     * @brief Get list of available models from Ollama
     * @return Vector of model names, or empty vector if failed
     */
    std::vector<std::string> listModels() const;
    
    /**
     * @brief Get detailed information about a specific model
     * @param modelName Name of the model to get details for
     * @return JSON object with model details, or empty JSON if failed
     */
    nlohmann::json getModelDetails(const std::string& modelName) const;
    
    /**
     * @brief Pull a model from Ollama registry
     * @param modelName Name of the model to pull
     * @return true if successful, false otherwise
     */
    bool pullModel(const std::string& modelName) const;
    
    /**
     * @brief Check if a model exists locally
     * @param modelName Name of the model to check
     * @return true if model exists, false otherwise
     */
    bool modelExists(const std::string& modelName) const;
    
private:
    /**
     * @brief Base URL for the Ollama API
     */
    std::string baseUrl;
    
    /**
     * @brief Make an HTTP GET request to the Ollama API
     * @param endpoint API endpoint to call
     * @return Response body as string, or empty string if failed
     */
    std::string makeGetRequest(const std::string& endpoint) const;
    
    /**
     * @brief Make an HTTP POST request to the Ollama API
     * @param endpoint API endpoint to call
     * @param body Request body as JSON
     * @return Response body as string, or empty string if failed
     */
    std::string makePostRequest(const std::string& endpoint, const nlohmann::json& body) const;
};

#endif // OLLAMA_CLIENT_H