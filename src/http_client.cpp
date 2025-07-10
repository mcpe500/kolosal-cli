#include "http_client.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <sstream>

void HttpClient::initialize() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void HttpClient::cleanup() {
    curl_global_cleanup();
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    HttpResponse* response = static_cast<HttpResponse*>(userp);
    response->data.append(static_cast<char*>(contents), realsize);
    return realsize;
}

bool HttpClient::get(const std::string& url, HttpResponse& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    // Clear any previous response data
    response.data.clear();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Check HTTP status code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    if (response_code != 200) {
        return false;
    }
    
    return true;
}

size_t HttpClient::writeStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), realsize);
    return realsize;
}

bool HttpClient::get(const std::string& url, std::string& response, const std::vector<std::string>& headers) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    // Clear any previous response data
    response.clear();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    
    // Set custom headers if provided
    struct curl_slist* headerList = nullptr;
    if (!headers.empty()) {
        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Cleanup headers
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Check HTTP status code but don't clear response for error codes
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    // Accept 200-299 status codes as successful, but preserve response for error analysis
    if (response_code < 200 || response_code >= 300) {
        return false;
    }
    
    return true;
}

bool HttpClient::post(const std::string& url, const std::string& payload, std::string& response, const std::vector<std::string>& headers) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    // Clear any previous response data
    response.clear();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    
    // Set custom headers if provided
    struct curl_slist* headerList = nullptr;
    if (!headers.empty()) {
        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Cleanup headers
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Check HTTP status code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    // Accept 200-299 status codes as successful
    if (response_code < 200 || response_code >= 300) {
        return false;
    }
    
    return true;
}

bool HttpClient::deleteRequest(const std::string& url, const std::string& payload, std::string& response, const std::vector<std::string>& headers) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    // Clear any previous response data
    response.clear();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    
    // Set payload if provided
    if (!payload.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    }
    
    // Set custom headers if provided
    struct curl_slist* headerList = nullptr;
    if (!headers.empty()) {
        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Cleanup headers
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Check HTTP status code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    // Accept 200-299 status codes as successful
    if (response_code < 200 || response_code >= 300) {
        return false;
    }
    
    return true;
}

HttpClient& HttpClient::getInstance() {
    static HttpClient instance;
    return instance;
}

// Streaming callback structure
struct StreamingData {
    std::function<void(const std::string&)> callback;
    std::string buffer;
    bool done;
    
    StreamingData() : done(false) {}
};

// Streaming write callback that accumulates data and processes complete lines
size_t streamingWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    StreamingData* data = static_cast<StreamingData*>(userp);
    
    // Append new data to buffer
    data->buffer.append(static_cast<char*>(contents), realsize);
    
    // Process complete lines
    size_t pos = 0;
    while ((pos = data->buffer.find('\n')) != std::string::npos) {
        std::string line = data->buffer.substr(0, pos);
        data->buffer.erase(0, pos + 1);
        
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Process the line
        if (line == "data: [DONE]") {
            data->done = true;
            break;
        } else if (line.length() > 6 && line.substr(0, 6) == "data: ") {
            // Extract JSON from "data: {...}"
            std::string jsonData = line.substr(6);
            data->callback(jsonData);
        }
        // Skip other lines (like "event:" lines, empty lines, etc.)
    }
    
    return realsize;
}

bool HttpClient::makeStreamingRequest(const std::string& url, const std::string& payload, 
                                    const std::string& headers, std::function<void(const std::string&)> chunkCallback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    StreamingData streamData;
    streamData.callback = chunkCallback;

    // Set curl options for streaming
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.length());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streamingWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &streamData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    
    // Critical options for real streaming
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024L); // Small buffer for responsiveness
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); // Force HTTP/1.1
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L); // Disable Nagle's algorithm
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L); // Disable progress meter

    // Set headers
    struct curl_slist* headerList = nullptr;
    if (!headers.empty()) {
        // Parse headers string and add each line
        std::istringstream headerStream(headers);
        std::string line;
        while (std::getline(headerStream, line)) {
            if (!line.empty() && line != "\r") {
                // Remove trailing \r if present
                if (line.back() == '\r') {
                    line.pop_back();
                }
                headerList = curl_slist_append(headerList, line.c_str());
            }
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    // Use curl_multi for proper streaming control
    CURLM* multi_handle = curl_multi_init();
    curl_multi_add_handle(multi_handle, curl);
    
    int still_running = 0;
    bool success = true;
    
    // Perform initial call
    curl_multi_perform(multi_handle, &still_running);
    
    // Main streaming loop - continue until done or error
    while (still_running && !streamData.done && success) {
        // Wait for activity
        int numfds = 0;
        CURLMcode mc = curl_multi_wait(multi_handle, nullptr, 0, 1000, &numfds);
        
        if (mc != CURLM_OK) {
            success = false;
            break;
        }
        
        // Read available data
        curl_multi_perform(multi_handle, &still_running);
        
        // Check for completed transfers
        CURLMsg* msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result != CURLE_OK) {
                    success = false;
                }
                break;
            }
        }
    }
    
    // Check HTTP status code
    if (success) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        success = (response_code >= 200 && response_code < 300);
    }

    // Cleanup
    curl_multi_remove_handle(multi_handle, curl);
    curl_multi_cleanup(multi_handle);
    
    if (headerList) {
        curl_slist_free_all(headerList);
    }
    
    curl_easy_cleanup(curl);
    return success;
}
