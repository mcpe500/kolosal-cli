#include "kolosal_server_client.h"
#include "http_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <regex>
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

KolosalServerClient::KolosalServerClient(const std::string& baseUrl, const std::string& apiKey)
    : m_baseUrl(baseUrl), m_apiKey(apiKey), m_serverProcess(nullptr), m_serverStartedByUs(false) {
}

KolosalServerClient::~KolosalServerClient() {
    if (m_serverStartedByUs) {
        stopServer();
    }
}

bool KolosalServerClient::startServer(const std::string& serverPath, int port) {
    // Check if server is already running
    if (isServerHealthy()) {
        return true;
    }
    
    // Determine server executable path
    std::string actualServerPath = serverPath;
    if (actualServerPath.empty()) {
        // Try to find the server in common locations relative to the CLI
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
        
        // Check for server-bin relative to CLI directory
        std::string parentDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
        parentDir = parentDir.substr(0, parentDir.find_last_of("\\/"));
        
        std::string serverBinPath = parentDir + "\\server-bin\\kolosal-server.exe";
        
        // Check if the file exists
        DWORD fileAttr = GetFileAttributesA(serverBinPath.c_str());
        if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            actualServerPath = serverBinPath;
        } else {
            // Fallback to current directory
            actualServerPath = "kolosal-server.exe";
        }
    }
    
    // Prepare command line
    std::string commandLine = "\"" + actualServerPath + "\" --port " + std::to_string(port) + " --host 0.0.0.0";
      // Set up working directory to be the same as the server executable
    std::string workingDir = actualServerPath.substr(0, actualServerPath.find_last_of("\\/"));
    if (workingDir == actualServerPath) {
        workingDir = "."; // If no path separator found, use current directory
    }
    
    // Windows process creation
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hide the server window
    ZeroMemory(&pi, sizeof(pi));
    
    // Create the process
    BOOL result = CreateProcessA(
        NULL,                           // No module name (use command line)
        const_cast<char*>(commandLine.c_str()), // Command line
        NULL,                           // Process handle not inheritable
        NULL,                           // Thread handle not inheritable
        FALSE,                          // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,             // Create new console
        NULL,                           // Use parent's environment block
        workingDir.c_str(),             // Working directory
        &si,                            // Pointer to STARTUPINFO structure
        &pi                             // Pointer to PROCESS_INFORMATION structure
    );
    
    if (!result) {
        DWORD error = GetLastError();
        std::cerr << "Failed to start Kolosal server. Error code: " << error << std::endl;
        
        // Provide more specific error messages
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
                std::cerr << "Server executable not found: " << actualServerPath << std::endl;
                break;
            case ERROR_ACCESS_DENIED:
                std::cerr << "Access denied when trying to start server." << std::endl;
                break;
            default:
                std::cerr << "Unknown error occurred." << std::endl;
                break;
        }
        return false;
    }
      // Store process handle and mark that we started it
    m_serverProcess = pi.hProcess;
    m_serverStartedByUs = true;
    CloseHandle(pi.hThread); // Close thread handle as we don't need it
    
    return true;
}

bool KolosalServerClient::stopServer() {
    if (!m_serverStartedByUs || !m_serverProcess) {
        return true;
    }
    
    HANDLE hProcess = static_cast<HANDLE>(m_serverProcess);
    
    // Try to terminate gracefully first
    if (!TerminateProcess(hProcess, 0)) {
        std::cerr << "Failed to terminate server process. Error code: " << GetLastError() << std::endl;
        return false;
    }
    
    // Wait for the process to exit
    WaitForSingleObject(hProcess, 5000); // Wait up to 5 seconds
    
    CloseHandle(hProcess);
    m_serverProcess = nullptr;
    m_serverStartedByUs = false;
    
    return true;
}

bool KolosalServerClient::isServerHealthy() {
    std::string response;
    if (!makeGetRequest("/v1/health", response)) {
        return false;
    }
    
    try {
        json healthJson = json::parse(response);        std::string status = healthJson.value("status", "");
        return status == "healthy";
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::waitForServerReady(int timeoutSeconds) {
    std::cout << "Starting server";
    
    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeoutSeconds);
    
    while (std::chrono::steady_clock::now() - startTime < timeout) {
        if (isServerHealthy()) {
            std::cout << " ✓" << std::endl;
            return true;
        }
        
        std::cout << ".";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    std::cout << " ✗" << std::endl;
    std::cout << "Timeout waiting for server." << std::endl;
    return false;
}

bool KolosalServerClient::addEngine(const std::string& engineId, const std::string& modelUrl, 
                                   const std::string& modelPath, bool loadAtStartup) {
    try {
        json payload;
        payload["engine_id"] = engineId;
        payload["model_path"] = modelUrl; // For download, we pass URL as model_path
        payload["n_ctx"] = 4096;
        payload["n_gpu_layers"] = 50;
        payload["main_gpu_id"] = 0;
        payload["load_at_startup"] = loadAtStartup;
        
        std::string response;
        if (!makePostRequest("/engines", payload.dump(), response)) {
            return false;
        }        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating engine: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::getDownloadProgress(const std::string& modelId, long long& downloadedBytes,
                                            long long& totalBytes, double& percentage, std::string& status) {
    std::string response;
    std::string endpoint = "/v1/download-progress/" + modelId;
    
    if (!makeGetRequest(endpoint, response)) {
        return false;
    }
    
    try {
        json progressJson = json::parse(response);
        
        status = progressJson.value("status", "unknown");
        
        if (progressJson.contains("progress")) {
            auto progress = progressJson["progress"];
            downloadedBytes = progress.value("downloaded_bytes", 0LL);
            totalBytes = progress.value("total_bytes", 0LL);
            percentage = progress.value("percentage", 0.0);
        } else {
            downloadedBytes = 0;
            totalBytes = 0;
            percentage = 0.0;
        }        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing download progress: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::monitorDownloadProgress(const std::string& modelId, 
                                                std::function<void(double, const std::string&, long long, long long)> progressCallback,
                                                int checkIntervalMs) {
    
    while (true) {
        long long downloadedBytes, totalBytes;
        double percentage;
        std::string status;
        
        if (!getDownloadProgress(modelId, downloadedBytes, totalBytes, percentage, status)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
            continue;
        }
        
        // Call the progress callback with all details
        progressCallback(percentage, status, downloadedBytes, totalBytes);
        
        // Check if download is complete
        if (status == "completed" || status == "creating_engine") {
            return true;
        } else if (status == "failed" || status == "cancelled") {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
    }
}

bool KolosalServerClient::makeGetRequest(const std::string& endpoint, std::string& response) {
    std::string url = m_baseUrl + endpoint;
    
    // Prepare headers
    std::vector<std::string> headers;
    if (!m_apiKey.empty()) {
        headers.push_back("X-API-Key: " + m_apiKey);
    }
    
    // Use existing HTTP client
    HttpClient client;
    return client.get(url, response, headers);
}

bool KolosalServerClient::makePostRequest(const std::string& endpoint, const std::string& payload, std::string& response) {
    std::string url = m_baseUrl + endpoint;
    
    // Prepare headers
    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    if (!m_apiKey.empty()) {
        headers.push_back("X-API-Key: " + m_apiKey);
    }
    
    // Use existing HTTP client
    HttpClient client;
    return client.post(url, payload, response, headers);
}

bool KolosalServerClient::parseJsonValue(const std::string& jsonString, const std::string& key, std::string& value) {
    try {
        json j = json::parse(jsonString);
        if (j.contains(key)) {            value = j[key].get<std::string>();
            return true;
        }
    } catch (const std::exception&) {
        // JSON parsing failed
    }
    return false;
}

bool KolosalServerClient::parseJsonNumber(const std::string& jsonString, const std::string& key, double& value) {
    try {
        json j = json::parse(jsonString);
        if (j.contains(key)) {            value = j[key].get<double>();
            return true;
        }
    } catch (const std::exception&) {
        // JSON parsing failed
    }
    return false;
}
