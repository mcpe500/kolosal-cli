#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <vector>

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

private:
    /**
     * @brief Callback function for libcurl to write response data
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // HTTP_CLIENT_H
