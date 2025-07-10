#include "kolosal_server_client.h"
#include "http_client.h"
#include "loading_animation.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <regex>
#include <fstream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <cstdlib>  // for _dupenv_s
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#endif

using json = nlohmann::json;

// Cross-platform helper functions
#ifdef _WIN32
    #define PATH_SEPARATOR "\\"
    #define EXECUTABLE_EXTENSION ".exe"
#else
    #define PATH_SEPARATOR "/"
    #define EXECUTABLE_EXTENSION ""
#endif

// Cross-platform process ID type
#ifdef _WIN32
typedef DWORD ProcessId;
#else
typedef pid_t ProcessId;
#endif

// Helper function to get executable path
std::string getExecutablePath() {
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    return std::string(exePath);
#else
    char exePath[1024];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len != -1) {
        exePath[len] = '\0';
        return std::string(exePath);
    }
    return "";
#endif
}

// Helper function to check if file exists
bool fileExists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

// Helper function to find server process ID
ProcessId findServerProcess() {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }

    ProcessId serverPid = 0;
    do {
        if (_wcsicmp(pe32.szExeFile, L"kolosal-server.exe") == 0) {
            serverPid = pe32.th32ProcessID;
            break;
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return serverPid;
#else
    // On Linux, search through /proc for the process
    DIR* procDir = opendir("/proc");
    if (!procDir) return 0;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";
            std::ifstream cmdlineFile(cmdlinePath);
            if (cmdlineFile.is_open()) {
                std::string cmdline;
                std::getline(cmdlineFile, cmdline);
                cmdlineFile.close();
                
                // Check if this process is kolosal-server
                if (cmdline.find("kolosal-server") != std::string::npos) {
                    closedir(procDir);
                    return std::stoi(entry->d_name);
                }
            }
        }
    }
    closedir(procDir);
    return 0;
#endif
}

// Helper function to terminate process
bool terminateProcess(ProcessId pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess != NULL) {
        bool success = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return success;
    }
    return false;
#else
    return kill(pid, SIGTERM) == 0;
#endif
}

KolosalServerClient::KolosalServerClient(const std::string &baseUrl, const std::string &apiKey)
    : m_baseUrl(baseUrl), m_apiKey(apiKey)
{
}

KolosalServerClient::~KolosalServerClient()
{
}

bool KolosalServerClient::startServer(const std::string &serverPath, int port)
{
    if (isServerHealthy())
    {
        return true;
    }

    std::string actualServerPath = serverPath;
    if (actualServerPath.empty())
    {
        std::string exePath = getExecutablePath();
        if (exePath.empty()) {
            std::cerr << "Error: Could not determine executable path" << std::endl;
            return false;
        }
        
        std::string exeDir = exePath.substr(0, exePath.find_last_of(PATH_SEPARATOR));
        std::cout << "Looking for kolosal-server from CLI directory: " << exeDir << std::endl;

        // Priority 1: Try same directory as CLI (highest priority for Windows)
        std::string sameDirPath = exeDir + PATH_SEPARATOR + "kolosal-server" + EXECUTABLE_EXTENSION;
        std::cout << "Checking same directory: " << sameDirPath << std::endl;
        if (fileExists(sameDirPath))
        {
            actualServerPath = sameDirPath;
            std::cout << "Found server at: " << actualServerPath << std::endl;
        }
        else
        {
            // Priority 2: Try kolosal-server subdirectory (common for extracted packages)
            std::string subDirPath = exeDir + PATH_SEPARATOR + "kolosal-server" + PATH_SEPARATOR + "kolosal-server" + EXECUTABLE_EXTENSION;
            std::cout << "Checking subdirectory: " << subDirPath << std::endl;
            if (fileExists(subDirPath))
            {
                actualServerPath = subDirPath;
                std::cout << "Found server at: " << actualServerPath << std::endl;
            }
            else
            {
                // Priority 3: Try parent/server-bin directory (Windows package structure)
                std::string parentDir = exeDir.substr(0, exeDir.find_last_of(PATH_SEPARATOR));
                std::string serverBinPath = parentDir + PATH_SEPARATOR + "server-bin" + PATH_SEPARATOR + "kolosal-server" + EXECUTABLE_EXTENSION;
                std::cout << "Checking parent/server-bin: " << serverBinPath << std::endl;
                if (fileExists(serverBinPath))
                {
                    actualServerPath = serverBinPath;
                    std::cout << "Found server at: " << actualServerPath << std::endl;
                }
                else
                {
                    // Priority 4: Try build directory (development environment)
                    std::string buildPath = parentDir + PATH_SEPARATOR + "build" + PATH_SEPARATOR + "kolosal-server" + PATH_SEPARATOR + "kolosal-server" + EXECUTABLE_EXTENSION;
                    std::cout << "Checking build directory: " << buildPath << std::endl;
                    if (fileExists(buildPath))
                    {
                        actualServerPath = buildPath;
                        std::cout << "Found server at: " << actualServerPath << std::endl;
                    }
                    else
                    {
                        // Last resort: Fall back to system PATH
                        actualServerPath = "kolosal-server" + std::string(EXECUTABLE_EXTENSION);
                        std::cout << "Using system PATH to find server: " << actualServerPath << std::endl;
                    }
                }
            }
        }
    } 
    else 
    {
        std::cout << "Using provided server path: " << actualServerPath << std::endl;
    }
    
    // Verify the server path exists before attempting to start
    if (!actualServerPath.empty() && actualServerPath != "kolosal-server" + std::string(EXECUTABLE_EXTENSION))
    {
        if (!fileExists(actualServerPath))
        {
            std::cerr << "Error: Server executable not found at: " << actualServerPath << std::endl;
            return false;
        }
    } 
    
#ifdef _WIN32
    // Build command line using the actual server path
    std::string commandLine;
    if (actualServerPath == "kolosal-server.exe") {
        commandLine = "kolosal-server.exe";
    } else {
        commandLine = "\"" + actualServerPath + "\"";
    }
    
    // Use the same directory as the CLI executable for config.yaml access
    std::string workingDir;
    
    // Priority 1: Use the same directory as the CLI executable (for config.yaml access)
    std::string exePath = getExecutablePath();
    if (!exePath.empty()) {
        std::string exeDir = exePath.substr(0, exePath.find_last_of(PATH_SEPARATOR));
        // Check if we can write to the exe directory (for logs, temp files, etc.)
        std::string testFile = exeDir + PATH_SEPARATOR + "test_write.tmp";
        std::ofstream testWrite(testFile);
        if (testWrite.is_open()) {
            testWrite.close();
            std::filesystem::remove(testFile);
            workingDir = exeDir;
            std::cout << "Using CLI directory as working directory: " << workingDir << std::endl;
        }
    }
    
    // Fallback: Use user profile or temp if exe directory is not writable
    if (workingDir.empty()) {
        char* userProfile = nullptr;
        size_t len = 0;
        if (_dupenv_s(&userProfile, &len, "USERPROFILE") == 0 && userProfile != nullptr) {
            workingDir = std::string(userProfile);
            free(userProfile);
            std::cout << "Using user profile as working directory: " << workingDir << std::endl;
        } else {
            // Final fallback to temp directory
            char* tempDir = nullptr;
            if (_dupenv_s(&tempDir, &len, "TEMP") == 0 && tempDir != nullptr) {
                workingDir = std::string(tempDir);
                free(tempDir);
            } else {
                workingDir = "C:\\temp";
            }
            std::cout << "Using temp directory as working directory: " << workingDir << std::endl;
        }
    }
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    
    BOOL result = CreateProcessA(
        (actualServerPath == "kolosal-server.exe") ? NULL : actualServerPath.c_str(),
        const_cast<char *>(commandLine.c_str()),
        NULL,
        NULL,
        FALSE,
        DETACHED_PROCESS,
        NULL,
        workingDir.c_str(),
        &si,
        &pi
    );

    if (!result)
    {
        DWORD error = GetLastError();
        switch (error)
        {
        case ERROR_FILE_NOT_FOUND:
            std::cerr << "Error: Server executable not found. Please ensure kolosal-server.exe is available." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cerr << "Error: Access denied. Please run as administrator if necessary." << std::endl;
            break;
        default:
            std::cerr << "Error: Failed to start server (code: " << error << ")" << std::endl;
            break;
        }
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    // Unix/Linux process creation
    std::cout << "Starting server: " << actualServerPath << std::endl;
    
    // Check if the file is executable
    if (access(actualServerPath.c_str(), X_OK) != 0) {
        std::cerr << "Error: Server binary is not executable: " << actualServerPath << std::endl;
        std::cerr << "Try running: chmod +x " << actualServerPath << std::endl;
        return false;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        // Change working directory to prioritize config.yaml access
        std::string workingDir;
        
        // Priority 1: Use the same directory as the CLI executable (for config.yaml access)
        std::string exePath = getExecutablePath();
        if (!exePath.empty()) {
            std::string exeDir = exePath.substr(0, exePath.find_last_of(PATH_SEPARATOR));
            // Check if we can write to the exe directory
            if (access(exeDir.c_str(), W_OK) == 0) {
                workingDir = exeDir;
                std::cerr << "Using CLI directory as working directory: " << workingDir << std::endl;
            }
        }
        
        // Fallback: Use system directories if exe directory is not writable
        if (workingDir.empty()) {
            // Try to use /var/lib/kolosal if it exists and is writable
            if (access("/var/lib/kolosal", W_OK) == 0) {
                workingDir = "/var/lib/kolosal";
                std::cerr << "Using /var/lib/kolosal as working directory: " << workingDir << std::endl;
            }
            // Otherwise use user's home directory
            else {
                const char* homeDir = getenv("HOME");
                if (homeDir) {
                    workingDir = std::string(homeDir);
                    std::cerr << "Using home directory as working directory: " << workingDir << std::endl;
                } else {
                    workingDir = "/tmp";  // Last resort
                    std::cerr << "Using /tmp as working directory: " << workingDir << std::endl;
                }
            }
        }
        
        if (chdir(workingDir.c_str()) != 0) {
            std::cerr << "Warning: Could not change to chosen directory: " << workingDir << std::endl;
            // Try /tmp as absolute last resort
            if (chdir("/tmp") != 0) {
                std::cerr << "Error: Could not change to any writable directory" << std::endl;
            } else {
                std::cerr << "Using /tmp as fallback working directory" << std::endl;
            }
        }
        
        // Execute the server in background (detached)
        setsid(); // Create new session
        
        // For debugging, redirect to log files instead of /dev/null temporarily
        // This will help us see what's happening with the server startup
        std::string logPath = "/tmp/kolosal-server.log";
        freopen(logPath.c_str(), "w", stdout);
        freopen(logPath.c_str(), "w", stderr);
        
        execl(actualServerPath.c_str(), "kolosal-server", nullptr);
        
        // If execl fails, log the error and exit child process
        std::cerr << "Failed to execute server: " << actualServerPath << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
        _exit(1);
    } else if (pid < 0) {
        // Fork failed
        std::cerr << "Error: Failed to start server process: " << strerror(errno) << std::endl;
        return false;
    }
    // Parent process continues - server is now running in background
    std::cout << "Server process started with PID: " << pid << std::endl;
    std::cout << "Server logs available at: /tmp/kolosal-server.log" << std::endl;
    
    // Give the server a moment to start before returning
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
#endif

    return true;
}

bool KolosalServerClient::isServerHealthy()
{
    std::string response;
    if (!makeGetRequest("/v1/health", response))
    {
        return false;
    }

    try
    {
        json healthJson = json::parse(response);
        std::string status = healthJson.value("status", "");
        return status == "healthy";
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool KolosalServerClient::waitForServerReady(int timeoutSeconds)
{
    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeoutSeconds);

    LoadingAnimation loading("Waiting for server to start");
    loading.start();

    while (std::chrono::steady_clock::now() - startTime < timeout)
    {
        if (isServerHealthy())
        {
            loading.complete("Server started successfully");
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    loading.stop();
    return false;
}

bool KolosalServerClient::getEngines(std::vector<std::string>& engines)
{
    std::string response;
    
    // Use the unified models endpoint
    if (!makeGetRequest("/models", response))
    {
        return false;
    }

    try
    {
        json modelsJson = json::parse(response);
        engines.clear();
        
        // Handle the unified models response format
        if (modelsJson.contains("models") && modelsJson["models"].is_array())
        {
            for (const auto& model : modelsJson["models"])
            {
                if (model.contains("model_id"))
                {
                    engines.push_back(model["model_id"].get<std::string>());
                }
            }
        }
        
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool KolosalServerClient::engineExists(const std::string& engineId)
{
    std::vector<std::string> engines;
    if (!getEngines(engines))
    {
        return false;
    }
    
    for (const auto& engine : engines)
    {
        if (engine == engineId)
        {
            return true;
        }
    }
    
    return false;
}

bool KolosalServerClient::addEngine(const std::string &engineId, const std::string &modelUrl,
                                    const std::string &modelPath)
{
    return addEngine(engineId, modelUrl, modelPath, "llama-cpu");
}

bool KolosalServerClient::addEngine(const std::string &engineId, const std::string &modelUrl,
                                    const std::string &modelPath, const std::string &inferenceEngine)
{
    try
    {
        // Note: Removed the early return for existing engines to handle cases where
        // downloads were cancelled or models failed to load properly
        
        // No loading animation for cleaner output - just show final result
        json payload;
        payload["model_id"] = engineId;
        payload["model_path"] = modelUrl; // For download, we pass URL as model_path
        payload["load_immediately"] = false;
        payload["main_gpu_id"] = 0;
        payload["inference_engine"] = inferenceEngine; // Use specified inference engine
        
        // Set comprehensive loading parameters
        json loadParams;
        loadParams["n_ctx"] = 4096;
        loadParams["n_keep"] = 2048;
        loadParams["use_mmap"] = true;
        loadParams["use_mlock"] = true;
        loadParams["n_parallel"] = 4;
        loadParams["cont_batching"] = true;
        loadParams["warmup"] = false;
        loadParams["n_gpu_layers"] = 50;
        loadParams["n_batch"] = 2048;
        loadParams["n_ubatch"] = 512;
        
        payload["loading_parameters"] = loadParams;

        std::string response;
        if (!makePostRequest("/models", payload.dump(), response))  // Using new model-based API endpoint
        {
            return false;
        }
        
        // Parse response to check if it was successful
        try {
            json responseJson = json::parse(response);
            
            // Check for error responses first
            if (responseJson.contains("error")) {
                auto error = responseJson["error"];
                std::string errorMessage = error.value("message", "Unknown error");
                std::string errorCode = error.value("code", "unknown");
                
                // Only treat "model_already_loaded" as a success case
                if (errorCode == "model_already_loaded") {
                    return true;
                }
                
                // For other errors, don't update config and return false
                std::cerr << "Server error: " << errorMessage << std::endl;
                return false;
            }
            
            // Check for success status codes
            if (responseJson.contains("status")) {
                std::string status = responseJson["status"];
                if (status == "loaded" || status == "created" || status == "downloading") {
                    // Only update config if the add operation was successful
                    updateConfigWithNewModel(engineId, modelUrl, inferenceEngine);
                    return true;
                }
            }
            
            // If no clear status, assume success for backward compatibility
            updateConfigWithNewModel(engineId, modelUrl, inferenceEngine);
            return true;
        } catch (const std::exception&) {
            // If we can't parse the response, still consider it successful if we got here
            updateConfigWithNewModel(engineId, modelUrl, inferenceEngine);
            return true;
        }
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool KolosalServerClient::getDownloadProgress(const std::string &modelId, long long &downloadedBytes,
                                              long long &totalBytes, double &percentage, std::string &status)
{
    std::string response;
    std::string endpoint = "/v1/downloads/" + modelId;

    // Try v1 endpoint first, then fallback to non-v1
    if (!makeGetRequest(endpoint, response))
    {
        endpoint = "/downloads/" + modelId;
        if (!makeGetRequest(endpoint, response))
        {
            // Check if response contains error indicating download not found
            if (!response.empty())
            {
                try
                {
                json errorJson = json::parse(response);
                if (errorJson.contains("error") && errorJson["error"].contains("code"))
                {
                    std::string errorCode = errorJson["error"]["code"];
                    if (errorCode == "download_not_found")
                    {
                        status = "not_found";
                        downloadedBytes = 0;
                        totalBytes = 0;
                        percentage = 0.0;
                        return true; // Return true to indicate we successfully determined there's no download
                    }
                }
            }
            catch (const std::exception &)
            {
                // If we can't parse the error response, fall through to return false
            }
            }
            return false;
        }
    }

    try
    {
        json progressJson = json::parse(response);

        status = progressJson.value("status", "unknown");

        if (progressJson.contains("progress"))
        {
            auto progress = progressJson["progress"];
            downloadedBytes = progress.value("downloaded_bytes", 0LL);
            totalBytes = progress.value("total_bytes", 0LL);
            percentage = progress.value("percentage", 0.0);
        }
        else
        {
            downloadedBytes = 0;
            totalBytes = 0;
            percentage = 0.0;
        }
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool KolosalServerClient::monitorDownloadProgress(const std::string &modelId,
                                                  std::function<void(double, const std::string &, long long, long long)> progressCallback,
                                                  int checkIntervalMs)
{
    auto startTime = std::chrono::steady_clock::now();
    auto maxDuration = std::chrono::minutes(30); // 30 minute timeout
    
    // Variables to track 100% completion status
    auto lastHundredPercentTime = std::chrono::steady_clock::time_point{};
    auto hundredPercentTimeout = std::chrono::seconds(30); // 30 seconds timeout at 100%
    bool hasReachedHundred = false;

    while (true)
    {
        // Check for timeout
        if (std::chrono::steady_clock::now() - startTime > maxDuration) {
            return false;
        }
        
        long long downloadedBytes, totalBytes;
        double percentage;
        std::string status;

        if (!getDownloadProgress(modelId, downloadedBytes, totalBytes, percentage, status))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
            continue;
        }

        progressCallback(percentage, status, downloadedBytes, totalBytes);

        // Success statuses
        if (status == "completed" || status == "creating_engine" || status == "engine_created")
        {
            return true;
        }
        // Failure statuses
        else if (status == "failed" || status == "cancelled" || status == "engine_creation_failed")
        {
            return false;
        }
        // No download found - this happens when a model file already exists and no download was needed
        else if (status == "not_found")
        {
            return true; // This is actually a success case - no download was needed
        }
        // Handle stuck at 100% case - download is complete but status hasn't transitioned yet
        else if (status == "downloading" && percentage >= 100.0)
        {
            if (!hasReachedHundred)
            {
                hasReachedHundred = true;
                lastHundredPercentTime = std::chrono::steady_clock::now();
                // Trigger status callback to show progress transition
                progressCallback(percentage, "completing", downloadedBytes, totalBytes);
            }
            else
            {
                auto timeSinceHundred = std::chrono::steady_clock::now() - lastHundredPercentTime;
                
                // Check if we've been stuck at 100% for a reasonable amount of time
                if (timeSinceHundred > hundredPercentTimeout)
                {
                    // First, check if the engine already exists (quick check)
                    if (engineExists(modelId))
                    {
                        // Engine exists, so download and engine creation was successful
                        progressCallback(100.0, "engine_created", downloadedBytes, totalBytes);
                        return true;
                    }
                    
                    // If more than 2 minutes at 100%, assume something is wrong
                    if (timeSinceHundred > std::chrono::minutes(2))
                    {
                        // Do one final check for engine existence before giving up
                        if (engineExists(modelId))
                        {
                            progressCallback(100.0, "engine_created", downloadedBytes, totalBytes);
                            return true;
                        }
                        return false; // Stuck too long, consider it failed
                    }
                    
                    // Between 30 seconds and 2 minutes, show "processing" status
                    if (timeSinceHundred > std::chrono::seconds(30))
                    {
                        progressCallback(percentage, "processing", downloadedBytes, totalBytes);
                    }
                }
            }
        }
        else
        {
            // Reset the 100% timer if we're not at 100%
            hasReachedHundred = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
    }
}

bool KolosalServerClient::makeGetRequest(const std::string &endpoint, std::string &response)
{
    std::string url = m_baseUrl + endpoint;

    std::vector<std::string> headers;
    if (!m_apiKey.empty())
    {
        headers.push_back("X-API-Key: " + m_apiKey);
    }

    HttpClient client;
    return client.get(url, response, headers);
}

bool KolosalServerClient::makePostRequest(const std::string &endpoint, const std::string &payload, std::string &response)
{
    std::string url = m_baseUrl + endpoint;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    if (!m_apiKey.empty())
    {
        headers.push_back("X-API-Key: " + m_apiKey);
    }

    HttpClient client;
    return client.post(url, payload, response, headers);
}

bool KolosalServerClient::makeDeleteRequest(const std::string &endpoint, const std::string &payload, std::string &response)
{
    std::string url = m_baseUrl + endpoint;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    if (!m_apiKey.empty())
    {
        headers.push_back("X-API-Key: " + m_apiKey);
    }

    HttpClient client;
    return client.deleteRequest(url, payload, response, headers);
}

bool KolosalServerClient::parseJsonValue(const std::string &jsonString, const std::string &key, std::string &value)
{
    try
    {
        json j = json::parse(jsonString);
        if (j.contains(key))
        {
            value = j[key].get<std::string>();
            return true;
        }
    }
    catch (const std::exception &)
    {
    }
    return false;
}

bool KolosalServerClient::parseJsonNumber(const std::string &jsonString, const std::string &key, double &value)
{
    try
    {
        json j = json::parse(jsonString);
        if (j.contains(key))
        {
            value = j[key].get<double>();
            return true;
        }
    }
    catch (const std::exception &)
    {
    }
    return false;
}

bool KolosalServerClient::shutdownServer()
{
    // Find the kolosal-server process
    ProcessId serverPid = findServerProcess();
    if (serverPid == 0) {
        // No server process found, consider it already stopped
        return true;
    }

    LoadingAnimation loading("Shutting down server");
    loading.start();

    // Terminate the process
    if (terminateProcess(serverPid)) {
        loading.complete("Server shutdown successfully");
        return true;
    } else {
        loading.stop();
        std::cerr << "Error: Failed to terminate server process" << std::endl;
        return false;
    }
}

bool KolosalServerClient::cancelDownload(const std::string &modelId)
{
    LoadingAnimation loading("Cancelling download");
    loading.start();

    std::string response;
    std::string endpoint = "/v1/downloads/" + modelId + "/cancel";

    // Try v1 endpoint first, then fallback to non-v1
    if (!makePostRequest(endpoint, "{}", response))
    {
        endpoint = "/downloads/" + modelId + "/cancel";
        if (!makePostRequest(endpoint, "{}", response))
        {
            loading.stop();
            return false;
        }
    }

    try
    {
        json cancelJson = json::parse(response);
        bool success = cancelJson.value("success", false);
        if (success) {
            loading.complete("Download cancelled");
            
            // Remove the model from config since the download was cancelled
            removeModelFromConfig(modelId);
        } else {
            loading.stop();
        }
        return success;
    }
    catch (const std::exception &)
    {
        loading.stop();
        return false;
    }
}

bool KolosalServerClient::cancelAllDownloads()
{
    LoadingAnimation loading("Cancelling all downloads");
    loading.start();

    std::string response;
    std::string endpoint = "/v1/downloads/cancel";

    // Try v1 endpoint first, then fallback to non-v1
    if (!makePostRequest(endpoint, "{}", response))
    {
        endpoint = "/downloads/cancel";
        if (!makePostRequest(endpoint, "{}", response))
        {
            loading.stop();
            return false;
        }
    }

    try
    {
        json cancelJson = json::parse(response);
        int cancelledCount = cancelJson.value("cancelled_count", 0);
        loading.complete("All downloads cancelled");
        return true;
    }
    catch (const std::exception &)
    {
        loading.stop();
        return false;
    }
}

bool KolosalServerClient::pauseDownload(const std::string &modelId)
{
    LoadingAnimation loading("Pausing download");
    loading.start();

    std::string response;
    std::string endpoint = "/v1/downloads/" + modelId + "/pause";

    // Try v1 endpoint first, then fallback to non-v1
    if (!makePostRequest(endpoint, "{}", response))
    {
        endpoint = "/downloads/" + modelId + "/pause";
        if (!makePostRequest(endpoint, "{}", response))
        {
            loading.stop();
            return false;
        }
    }

    try
    {
        json pauseJson = json::parse(response);
        bool success = pauseJson.value("success", false);
        if (success) {
            loading.complete("Download paused");
        } else {
            loading.stop();
        }
        return success;
    }
    catch (const std::exception &)
    {
        loading.stop();
        return false;
    }
}

bool KolosalServerClient::resumeDownload(const std::string &modelId)
{
    LoadingAnimation loading("Resuming download");
    loading.start();

    std::string response;
    std::string endpoint = "/v1/downloads/" + modelId + "/resume";

    // Try v1 endpoint first, then fallback to non-v1
    if (!makePostRequest(endpoint, "{}", response))
    {
        endpoint = "/downloads/" + modelId + "/resume";
        if (!makePostRequest(endpoint, "{}", response))
        {
            loading.stop();
            return false;
        }
    }

    try
    {
        json resumeJson = json::parse(response);
        bool success = resumeJson.value("success", false);
        if (success) {
            loading.complete("Download resumed");
        } else {
            loading.stop();
        }
        return success;
    }
    catch (const std::exception &)
    {
        loading.stop();
        return false;
    }
}

bool KolosalServerClient::getAllDownloads(std::vector<std::tuple<std::string, std::string, double, long long, long long>>& downloads)
{
    std::string response;
    std::string endpoint = "/v1/downloads";

    // Try v1 endpoint first, then fallback to non-v1
    if (!makeGetRequest(endpoint, response))
    {
        endpoint = "/downloads";
        if (!makeGetRequest(endpoint, response))
        {
            return false;
        }
    }

    try
    {
        json downloadsJson = json::parse(response);
        
        if (downloadsJson.contains("downloads") && downloadsJson["downloads"].is_array())
        {
            downloads.clear();
            for (const auto& download : downloadsJson["downloads"])
            {
                std::string modelId = download.value("model_id", "");
                std::string status = download.value("status", "");
                double percentage = download.value("percentage", 0.0);
                long long downloadedBytes = download.value("downloaded_bytes", 0LL);
                long long totalBytes = download.value("total_bytes", 0LL);
                
                downloads.emplace_back(modelId, status, percentage, downloadedBytes, totalBytes);
            }
            return true;
        }
        
        return false;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool KolosalServerClient::updateConfigWithNewModel(const std::string& engineId, const std::string& modelPath, const std::string& inferenceEngine)
{
    const std::string configPath = "config.yaml";
    
    try {
        if (!std::filesystem::exists(configPath)) {
            return false;
        }
        
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            return false;
        }
        
        std::string configContent;
        std::string line;
        bool inModelsSection = false;
        bool modelsAdded = false;
        
        while (std::getline(configFile, line)) {
            configContent += line + "\n";
            
            if (line.find("models:") == 0) {
                inModelsSection = true;
            }
            else if (inModelsSection && !line.empty() && line[0] != ' ' && line[0] != '#' && line[0] != '-') {
                inModelsSection = false;
            }
        }
        configFile.close();
        
        // Check if model with this ID already exists
        std::string searchPattern = "id: \"" + engineId + "\"";
        if (configContent.find(searchPattern) != std::string::npos) {
            // Model already exists in config, don't add duplicate
            return true;
        }
        
        std::string newModelEntry = "  - id: \"" + engineId + "\"\n"
                                   "    path: \"" + modelPath + "\"\n"
                                   "    load_immediately: false\n"
                                   "    main_gpu_id: 0\n"
                                   "    inference_engine: \"" + inferenceEngine + "\"\n"
                                   "    load_params:\n"
                                   "      n_ctx: 4096\n"
                                   "      n_keep: 2048\n"
                                   "      use_mmap: true\n"
                                   "      use_mlock: true\n"
                                   "      n_parallel: 4\n"
                                   "      cont_batching: true\n"
                                   "      warmup: false\n"
                                   "      n_gpu_layers: 50\n"
                                   "      n_batch: 2048\n"
                                   "      n_ubatch: 512\n";
        
        size_t modelsPos = configContent.find("models:");
        if (modelsPos != std::string::npos) {
            size_t afterModels = modelsPos + 7;
            size_t nextSection = configContent.find("\n# =====", afterModels);
            if (nextSection == std::string::npos) {
                nextSection = configContent.length();
            }
            
            std::string modelsSection = configContent.substr(afterModels, nextSection - afterModels);
            
            bool hasActiveModels = modelsSection.find("\n  - id:") != std::string::npos;
            
            if (hasActiveModels) {
                size_t lastModelEnd = configContent.rfind("      n_ubatch:", nextSection);
                if (lastModelEnd != std::string::npos) {
                    size_t lineEnd = configContent.find("\n", lastModelEnd);
                    if (lineEnd != std::string::npos) {
                        configContent.insert(lineEnd + 1, "\n" + newModelEntry);
                    }
                } else {
                    size_t lineEnd = configContent.find("\n", modelsPos);
                    if (lineEnd != std::string::npos) {
                        configContent.insert(lineEnd + 1, newModelEntry);
                    }
                }
            } else {
                size_t lineEnd = configContent.find("\n", modelsPos);
                if (lineEnd != std::string::npos) {
                    configContent.insert(lineEnd + 1, newModelEntry);
                }
            }
        } else {
            return false;
        }
        
        std::ofstream outFile(configPath);
        if (!outFile.is_open()) {
            return false;
        }
        
        outFile << configContent;
        outFile.close();
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::chatCompletion(const std::string& engineId, const std::string& message, std::string& response)
{
    try {
        json requestBody = {
            {"model", engineId},
            {"messages", json::array({
                {{"role", "user"}, {"content", message}}
            })},
            {"streaming", false},
            {"maxNewTokens", 2048},
            {"temperature", 0.7},
            {"topP", 0.9}
        };

        std::string jsonResponse;
        if (!makePostRequest("/v1/inference/chat/completions", requestBody.dump(), jsonResponse)) {
            return false;
        }

        // Parse the response - using inference format
        json responseJson = json::parse(jsonResponse);
        if (responseJson.contains("text")) {
            response = responseJson["text"].get<std::string>();
            return true;
        }

        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::streamingChatCompletion(const std::string& engineId, const std::string& message, 
                                                std::function<void(const std::string&, double, double)> responseCallback)
{
    try {
        json requestBody = {
            {"model", engineId},
            {"messages", json::array({
                {{"role", "user"}, {"content", message}}
            })},
            {"streaming", true},
            {"maxNewTokens", 2048},
            {"temperature", 0.7},
            {"topP", 0.9}
        };

        // For streaming, we need to make a custom HTTP request to handle Server-Sent Events
        std::string url = m_baseUrl + "/v1/inference/chat/completions";
        std::string headers = "Content-Type: application/json\r\n";
        headers += "Accept: text/event-stream\r\n";
        headers += "Cache-Control: no-cache\r\n";
        if (!m_apiKey.empty()) {
            headers += "Authorization: Bearer " + m_apiKey + "\r\n";
        }

        // Use a basic streaming implementation
        std::string buffer;
        bool receivedContent = false;
        bool streamComplete = false;
        bool httpSuccess = HttpClient::getInstance().makeStreamingRequest(url, requestBody.dump(), headers, 
            [&](const std::string& jsonData) {
                try {
                    json chunkJson = json::parse(jsonData);
                    
                    // Handle inference chat completion response format
                    if (chunkJson.contains("text")) {
                        std::string content = chunkJson["text"].get<std::string>();
                        double tps = chunkJson.value("tps", 0.0);
                        double ttft = chunkJson.value("ttft", 0.0);
                        
                        if (!content.empty()) {
                            responseCallback(content, tps, ttft);
                            receivedContent = true;
                        }
                    }
                    
                    // Check for completion - final chunk has partial=false
                    if (chunkJson.contains("partial") && !chunkJson["partial"].get<bool>()) {
                        streamComplete = true;
                    }
                    
                    // Check for error
                    if (chunkJson.contains("error")) {
                        streamComplete = true;
                    }
                } catch (const std::exception&) {
                    // Silently ignore JSON parse errors for malformed chunks
                }
            });

        // Consider streaming successful if we received any content or completed stream, regardless of HTTP status
        return receivedContent || streamComplete;
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::getLogs(std::vector<std::tuple<std::string, std::string, std::string>>& logs) {
    try {
        std::string response;
        if (!makeGetRequest("/logs", response)) {
            return false;
        }
        
        json jsonResponse = json::parse(response);
        
        if (!jsonResponse.contains("logs") || !jsonResponse["logs"].is_array()) {
            return false;
        }
        
        logs.clear();
        for (const auto& logEntry : jsonResponse["logs"]) {
            if (logEntry.contains("level") && logEntry.contains("timestamp") && logEntry.contains("message")) {
                logs.emplace_back(
                    logEntry["level"].get<std::string>(),
                    logEntry["timestamp"].get<std::string>(),
                    logEntry["message"].get<std::string>()
                );
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::getInferenceEngines(std::vector<std::tuple<std::string, std::string, std::string, std::string, bool>>& engines)
{
    std::string response;
    
    // Try both /v1/inference-engines and /inference-engines endpoints
    if (!makeGetRequest("/v1/inference-engines", response))
    {
        if (!makeGetRequest("/inference-engines", response))
        {
            return false;
        }
    }

    try
    {
        json enginesJson = json::parse(response);
        engines.clear();
        
        // Handle inference engines response format
        if (enginesJson.contains("inference_engines") && enginesJson["inference_engines"].is_array())
        {
            for (const auto& engine : enginesJson["inference_engines"])
            {
                std::string name = engine.value("name", "");
                std::string version = engine.value("version", "");
                std::string description = engine.value("description", "");
                std::string library_path = engine.value("library_path", "");
                bool is_loaded = engine.value("is_loaded", false);
                
                engines.emplace_back(name, version, description, library_path, is_loaded);
            }
        }
        
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool KolosalServerClient::removeModel(const std::string& modelId)
{
    try {
        std::string response;
        // Use DELETE request to the RESTful endpoint /models/{id}
        std::string endpoint = "/models/" + modelId;
        if (!makeDeleteRequest(endpoint, "", response)) {
            return false;
        }

        // Parse the response to check if removal was successful
        json responseJson = json::parse(response);
        return responseJson.contains("status") && responseJson["status"] == "removed";
    }
    catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::getModelStatus(const std::string& modelId, std::string& status, std::string& message)
{
    try {
        std::string response;
        // Use GET request to the RESTful status endpoint /models/{id}/status
        std::string endpoint = "/models/" + modelId + "/status";
        if (!makeGetRequest(endpoint, response)) {
            return false;
        }

        // Parse the response
        json responseJson = json::parse(response);
        status = responseJson.value("status", "unknown");
        message = responseJson.value("message", "");
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::removeModelFromConfig(const std::string& modelId)
{
    const std::string configPath = "config.yaml";
    
    try {
        if (!std::filesystem::exists(configPath)) {
            return false;
        }
        
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            return false;
        }
        
        std::string configContent;
        std::string line;
        std::vector<std::string> lines;
        
        // Read all lines
        while (std::getline(configFile, line)) {
            lines.push_back(line);
        }
        configFile.close();
        
        // Find and remove the model entry
        std::vector<std::string> filteredLines;
        bool inTargetModel = false;
        int modelIndent = -1;
        
        for (size_t i = 0; i < lines.size(); i++) {
            const std::string& currentLine = lines[i];
            
            // Check if this line starts a model entry
            if (currentLine.find("- id:") != std::string::npos) {
                // Extract the model ID from this line
                size_t idPos = currentLine.find("\"");
                if (idPos != std::string::npos) {
                    size_t idEnd = currentLine.find("\"", idPos + 1);
                    if (idEnd != std::string::npos) {
                        std::string foundId = currentLine.substr(idPos + 1, idEnd - idPos - 1);
                        if (foundId == modelId) {
                            inTargetModel = true;
                            // Calculate the indentation level
                            modelIndent = 0;
                            for (char c : currentLine) {
                                if (c == ' ') modelIndent++;
                                else break;
                            }
                            continue; // Skip this line
                        }
                    }
                }
                inTargetModel = false;
                modelIndent = -1;
            }
            
            if (inTargetModel) {
                // Check if we've moved to the next model or section
                int currentIndent = 0;
                for (char c : currentLine) {
                    if (c == ' ') currentIndent++;
                    else break;
                }
                
                // If the line is at the same or lower indentation level and not empty, we've left the model
                if (!currentLine.empty() && currentIndent <= modelIndent && currentLine.find_first_not_of(' ') != std::string::npos) {
                    inTargetModel = false;
                    modelIndent = -1;
                    filteredLines.push_back(currentLine);
                }
                // Otherwise, skip this line (it's part of the model we're removing)
            } else {
                filteredLines.push_back(currentLine);
            }
        }
        
        // Write the filtered content back to the file
        std::ofstream outFile(configPath);
        if (!outFile.is_open()) {
            return false;
        }
        
        for (const std::string& filteredLine : filteredLines) {
            outFile << filteredLine << "\n";
        }
        outFile.close();
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}
