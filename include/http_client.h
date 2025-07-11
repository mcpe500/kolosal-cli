#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <functional>

/**
 * @brief Structure to hold HTTP response data
 */
struct HttpResponse {
    std::string data;
};

/**
 * @brief HTTP client for making requests to Hugging Face API using libcurl
 */
class HttpClient {
public:
    /**
     * @brief Initialize the HTTP client (calls curl_global_init)
     */
    static void initialize();
    
    /**
     * @brief Cleanup the HTTP client (calls curl_global_cleanup)
     */
    static void cleanup();
      /**
     * @brief Make a GET request to the specified URL
     * @param url The URL to request
     * @param response Reference to HttpResponse object to store the response
     * @return true if the request was successful, false otherwise
     */
    static bool get(const std::string& url, HttpResponse& response);
    
    /**
     * @brief Make a GET request with custom headers
     * @param url The URL to request
     * @param response String to store the response body
     * @param headers Vector of custom headers (format: "Header: Value")
     * @return true if the request was successful, false otherwise
     */
    bool get(const std::string& url, std::string& response, const std::vector<std::string>& headers = {});
    
    /**
     * @brief Make a POST request with JSON payload
     * @param url The URL to request
     * @param payload JSON payload to send
     * @param response String to store the response body
     * @param headers Vector of custom headers (format: "Header: Value")
     * @return true if the request was successful, false otherwise
     */
    bool post(const std::string& url, const std::string& payload, std::string& response, const std::vector<std::string>& headers = {});

    /**
     * @brief Make a DELETE request with optional JSON payload
     * @param url The URL to request
     * @param payload JSON payload to send (optional)
     * @param response String to store the response body
     * @param headers Vector of custom headers (format: "Header: Value")
     * @return true if the request was successful, false otherwise
     */
    bool deleteRequest(const std::string& url, const std::string& payload, std::string& response, const std::vector<std::string>& headers = {});

    /**
     * @brief Make a PUT request with JSON payload
     * @param url The URL to request
     * @param payload JSON payload to send
     * @param response String to store the response body
     * @param headers Vector of custom headers (format: "Header: Value")
     * @return true if the request was successful, false otherwise
     */
    bool put(const std::string& url, const std::string& payload, std::string& response, const std::vector<std::string>& headers = {});

    /**
     * @brief Get singleton instance of HttpClient
     * @return Reference to the singleton instance
     */
    static HttpClient& getInstance();

    /**
     * @brief Make a streaming POST request with a callback for each chunk
     * @param url The URL to request
     * @param payload JSON payload to send
     * @param headers Custom headers string (format: "Header: Value\r\n")
     * @param chunkCallback Callback function called for each chunk received
     * @return true if the request was successful, false otherwise
     */
    bool makeStreamingRequest(const std::string& url, const std::string& payload, 
                            const std::string& headers, std::function<void(const std::string&)> chunkCallback);

    /**
     * @brief Download a file from a URL to a local path
     * @param url The URL to download from
     * @param filePath The local file path to save to
     * @param progressCallback Optional callback for progress updates (downloaded, total, percentage)
     * @return true if the download was successful, false otherwise
     */
    static bool downloadFile(const std::string& url, const std::string& filePath, 
                            std::function<void(size_t, size_t, double)> progressCallback = nullptr);

    /**
     * @brief Get the file size from a URL without downloading
     * @param url The URL to check
     * @return File size in bytes, or -1 if failed
     */
    static long long getFileSize(const std::string& url);

private:
    /**
     * @brief Callback function for libcurl to write response data
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    /**
     * @brief Instance-based callback for writing response data to std::string
     */
    static size_t writeStringCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // HTTP_CLIENT_H
