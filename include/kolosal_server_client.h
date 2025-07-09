#ifndef KOLOSAL_SERVER_CLIENT_H
#define KOLOSAL_SERVER_CLIENT_H

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>

/**
 * @brief Client for communicating with Kolosal Server
 */
class KolosalServerClient {
public:
    /**
     * @brief Constructor
     * @param baseUrl Base URL of the Kolosal server (default: http://localhost:8080)
     * @param apiKey Optional API key for authentication
     */
    KolosalServerClient(const std::string& baseUrl = "http://localhost:8080", const std::string& apiKey = "");
    
    /**
     * @brief Destructor
     */
    ~KolosalServerClient();    /**
     * @brief Start the Kolosal server in the background if not already running
     * @param serverPath Path to kolosal-server executable
     * @param port Port to run the server on (default: 8080)
     * @return True if server started successfully or is already running, false otherwise
     */
    bool startServer(const std::string& serverPath = "", int port = 8080);
    
    /**
     * @brief Gracefully shutdown the server via API call
     * @return True if shutdown request was sent successfully, false otherwise
     */
    bool shutdownServer();
    
    /**
     * @brief Check if the server is running and healthy
     * @return True if server is healthy, false otherwise
     */
    bool isServerHealthy();
    
    /**
     * @brief Wait for the server to become healthy
     * @param timeoutSeconds Maximum time to wait in seconds (default: 30)
     * @return True if server became healthy within timeout, false otherwise
     */
    bool waitForServerReady(int timeoutSeconds = 30);
    
    /**
     * @brief Add an engine to the server and start downloading the model
     * @param engineId Unique identifier for the engine
     * @param modelUrl URL to download the model from
     * @param modelPath Local path where the model will be saved
     * @return True if engine creation was initiated successfully, false otherwise
     */
    bool addEngine(const std::string& engineId, const std::string& modelUrl, 
                   const std::string& modelPath);
    
    /**
     * @brief Add an engine to the server with specific inference engine
     * @param engineId Unique identifier for the engine
     * @param modelUrl URL to download the model from
     * @param modelPath Local path where the model will be saved
     * @param inferenceEngine Inference engine to use (llama-cpu, llama-cuda, llama-vulkan, etc.)
     * @return True if engine creation was initiated successfully, false otherwise
     */
    bool addEngine(const std::string& engineId, const std::string& modelUrl, 
                   const std::string& modelPath, const std::string& inferenceEngine);
    
    /**
     * @brief Get list of existing engines from the server
     * @param engines Output: vector of engine IDs that exist on the server
     * @return True if engines list was retrieved successfully, false otherwise
     */
    bool getEngines(std::vector<std::string>& engines);
    
    /**
     * @brief Get list of available inference engines from the server
     * @param engines Output: vector of inference engine info structures
     * @return True if inference engines list was retrieved successfully, false otherwise
     */
    bool getInferenceEngines(std::vector<std::tuple<std::string, std::string, std::string, std::string, bool>>& engines);
    
    /**
     * @brief Check if an engine with the given ID already exists on the server
     * @param engineId Engine ID to check
     * @return True if engine exists, false otherwise
     */
    bool engineExists(const std::string& engineId);
    
    /**
     * @brief Get download progress for a specific model
     * @param modelId Model ID to check progress for
     * @param downloadedBytes Output: bytes downloaded so far
     * @param totalBytes Output: total bytes to download
     * @param percentage Output: download percentage (0-100)
     * @param status Output: download status
     * @return True if progress was retrieved successfully, false otherwise
     */
    bool getDownloadProgress(const std::string& modelId, long long& downloadedBytes,
                           long long& totalBytes, double& percentage, std::string& status);
      /**
     * @brief Monitor download progress and provide updates
     * @param modelId Model ID to monitor
     * @param progressCallback Callback function called with progress updates (percentage, status, downloadedBytes, totalBytes)
     * @param checkIntervalMs Interval between progress checks in milliseconds (default: 1000)
     * @return True if download completed successfully, false otherwise
     */
    bool monitorDownloadProgress(const std::string& modelId, 
                               std::function<void(double, const std::string&, long long, long long)> progressCallback,
                               int checkIntervalMs = 1000);

    /**
     * @brief Cancel a specific download
     * @param modelId Model ID of the download to cancel
     * @return True if cancellation request was successful, false otherwise
     */
    bool cancelDownload(const std::string& modelId);

    /**
     * @brief Cancel all active downloads
     * @return True if cancellation request was successful, false otherwise
     */
    bool cancelAllDownloads();

    /**
     * @brief Send a chat completion request to the server
     * @param engineId Engine ID to use for chat completion
     * @param message User message to send
     * @param response Output: assistant's response
     * @return True if chat completion was successful, false otherwise
     */
    bool chatCompletion(const std::string& engineId, const std::string& message, std::string& response);

    /**
     * @brief Send a streaming chat completion request to the server
     * @param engineId Engine ID to use for chat completion
     * @param message User message to send
     * @param responseCallback Callback function called for each token/chunk received (text, tps, ttft)
     * @return True if chat completion was successful, false otherwise
     */
    bool streamingChatCompletion(const std::string& engineId, const std::string& message, 
                               std::function<void(const std::string&, double, double)> responseCallback);

    /**
     * @brief Get server logs
     * @param logs Output: vector of log entries with level, timestamp, and message
     * @return True if logs were retrieved successfully, false otherwise
     */
    bool getLogs(std::vector<std::tuple<std::string, std::string, std::string>>& logs);

private:
    std::string m_baseUrl;
    std::string m_apiKey;
    
    /**
     * @brief Make HTTP GET request to the server
     * @param endpoint API endpoint (e.g., "/v1/health")
     * @param response Output: response body
     * @return True if request was successful, false otherwise
     */
    bool makeGetRequest(const std::string& endpoint, std::string& response);
    
    /**
     * @brief Make HTTP POST request to the server
     * @param endpoint API endpoint
     * @param payload JSON payload to send
     * @param response Output: response body
     * @return True if request was successful, false otherwise
     */
    bool makePostRequest(const std::string& endpoint, const std::string& payload, std::string& response);
    
    /**
     * @brief Parse JSON response
     * @param jsonString JSON string to parse
     * @param key Key to extract from JSON
     * @param value Output: extracted value
     * @return True if parsing was successful, false otherwise
     */
    bool parseJsonValue(const std::string& jsonString, const std::string& key, std::string& value);
      /**
     * @brief Parse JSON number value
     * @param jsonString JSON string to parse
     * @param key Key to extract from JSON
     * @param value Output: extracted number value
     * @return True if parsing was successful, false otherwise
     */
    bool parseJsonNumber(const std::string& jsonString, const std::string& key, double& value);
    
    /**
     * @brief Update config.yaml to include a new model entry
     * @param engineId Model ID/name to add
     * @param modelPath Path to the model file
     * @return True if config was updated successfully, false otherwise
     */
    bool updateConfigWithNewModel(const std::string& engineId, const std::string& modelPath, const std::string& inferenceEngine = "llama-cpu");
};

#endif // KOLOSAL_SERVER_CLIENT_H
