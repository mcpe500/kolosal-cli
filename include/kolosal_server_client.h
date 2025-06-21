#ifndef KOLOSAL_SERVER_CLIENT_H
#define KOLOSAL_SERVER_CLIENT_H

#include <string>
#include <memory>
#include <functional>

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
     * @brief Start the Kolosal server in the background
     * @param serverPath Path to kolosal-server executable
     * @param port Port to run the server on (default: 8080)
     * @return True if server started successfully, false otherwise
     */
    bool startServer(const std::string& serverPath = "", int port = 8080);
    
    /**
     * @brief Stop the Kolosal server
     * @return True if server stopped successfully, false otherwise
     */
    bool stopServer();
    
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
     * @param loadAtStartup Whether to load the model immediately after download
     * @return True if engine creation was initiated successfully, false otherwise
     */
    bool addEngine(const std::string& engineId, const std::string& modelUrl, 
                   const std::string& modelPath, bool loadAtStartup = true);
    
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
     * @brief Monitor download progress with callback
     * @param modelId Model ID to monitor
     * @param progressCallback Callback function called with progress updates (percentage, status, downloadedBytes, totalBytes)
     * @param checkIntervalMs Interval between progress checks in milliseconds (default: 1000)
     * @return True if download completed successfully, false otherwise
     */
    bool monitorDownloadProgress(const std::string& modelId, 
                               std::function<void(double, const std::string&, long long, long long)> progressCallback,
                               int checkIntervalMs = 1000);

private:
    std::string m_baseUrl;
    std::string m_apiKey;
    void* m_serverProcess; // HANDLE on Windows, will be cast as needed
    bool m_serverStartedByUs;
    
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
};

#endif // KOLOSAL_SERVER_CLIENT_H
