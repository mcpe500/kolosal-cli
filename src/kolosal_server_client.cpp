#include "kolosal_server_client.h"
#include "http_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <regex>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <tlhelp32.h>
#include <nlohmann/json.hpp>

// Note: For YAML processing, we'll use a simple string manipulation approach
// since we don't want to add yaml-cpp dependency to the CLI

using json = nlohmann::json;

KolosalServerClient::KolosalServerClient(const std::string &baseUrl, const std::string &apiKey)
    : m_baseUrl(baseUrl), m_apiKey(apiKey), m_serverProcess(nullptr), m_serverStartedByUs(false)
{
}

KolosalServerClient::~KolosalServerClient()
{
    // Server should keep running in background after CLI exits
    // Don't stop server in destructor anymore
    if (m_serverProcess)
    {
        CloseHandle(static_cast<HANDLE>(m_serverProcess));
        m_serverProcess = nullptr;
    }
}

bool KolosalServerClient::startServer(const std::string &serverPath, int port)
{
    // Check if server is already running
    if (isServerHealthy())
    {
        return true;
    }

    // Server not detected. Starting new server instance...
    // Determine server executable path
    std::string actualServerPath = serverPath;
    if (actualServerPath.empty())
    {
        // Try to find the server in the same directory as the CLI executable
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

        // First try: same directory as CLI executable
        std::string sameDirPath = exeDir + "\\kolosal-server.exe";

        // Check if the file exists in the same directory
        DWORD fileAttr = GetFileAttributesA(sameDirPath.c_str());
        if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            actualServerPath = sameDirPath;
        }
        else
        {
            // Fallback: Check for server-bin relative to CLI directory (for backward compatibility)
            std::string parentDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
            parentDir = parentDir.substr(0, parentDir.find_last_of("\\/"));

            std::string serverBinPath = parentDir + "\\server-bin\\kolosal-server.exe";

            fileAttr = GetFileAttributesA(serverBinPath.c_str());
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                actualServerPath = serverBinPath;
            }
            else
            {
                // Final fallback to current directory
                actualServerPath = "kolosal-server.exe";
            }
        }    } 
    
    // Prepare command line - server will automatically detect config.yaml
    std::string commandLine = "kolosal-server.exe";

    // Set up working directory to be the same as the server executable
    std::string workingDir = actualServerPath.substr(0, actualServerPath.find_last_of("\\/"));
    if (workingDir == actualServerPath)
    {
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
    // Create the process detached from parent so it can persist after CLI exits
    BOOL result = CreateProcessA(
        actualServerPath.c_str(),                // Application name
        const_cast<char *>(commandLine.c_str()), // Command line
        NULL,                                    // Process handle not inheritable
        NULL,                                    // Thread handle not inheritable
        FALSE,                                   // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,                      // Create new console only
        NULL,                                    // Use parent's environment block
        workingDir.c_str(),                      // Working directory
        &si,                                     // Pointer to STARTUPINFO structure
        &pi                                      // Pointer to PROCESS_INFORMATION structure
    );

    if (!result)
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to start Kolosal server. Error code: " << error << std::endl;

        // Provide more specific error messages
        switch (error)
        {
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
    } // Store process handle and mark that we started it
    m_serverProcess = pi.hProcess;
    m_serverStartedByUs = true;
    CloseHandle(pi.hThread); // Close thread handle as we don't need it

    return true;
}

bool KolosalServerClient::stopServer()
{
    if (!m_serverStartedByUs || !m_serverProcess)
    {
        return true;
    }

    HANDLE hProcess = static_cast<HANDLE>(m_serverProcess);

    // Try to terminate gracefully first
    if (!TerminateProcess(hProcess, 0))
    {
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

    while (std::chrono::steady_clock::now() - startTime < timeout)
    {
        if (isServerHealthy())
        {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return false;
}

bool KolosalServerClient::addEngine(const std::string &engineId, const std::string &modelUrl,
                                    const std::string &modelPath, bool loadAtStartup)
{
    try
    {
        json payload;
        payload["engine_id"] = engineId;
        payload["model_path"] = modelUrl; // For download, we pass URL as model_path
        payload["n_ctx"] = 4096;
        payload["n_gpu_layers"] = 50;
        payload["main_gpu_id"] = 0;
        payload["load_at_startup"] = false; // Always set to lazy loading for user-selected models

        std::string response;
        if (!makePostRequest("/engines", payload.dump(), response))
        {
            return false;
        }
        
        // If engine was added successfully, update config.yaml
        // Use the model path if provided, otherwise use the URL
        std::string pathToStore = modelPath.empty() ? modelUrl : modelPath;
        updateConfigWithNewModel(engineId, pathToStore);
        
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating engine: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::getDownloadProgress(const std::string &modelId, long long &downloadedBytes,
                                              long long &totalBytes, double &percentage, std::string &status)
{
    std::string response;
    std::string endpoint = "/v1/download-progress/" + modelId;

    if (!makeGetRequest(endpoint, response))
    {
        return false;
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
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing download progress: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::monitorDownloadProgress(const std::string &modelId,
                                                  std::function<void(double, const std::string &, long long, long long)> progressCallback,
                                                  int checkIntervalMs)
{

    while (true)
    {
        long long downloadedBytes, totalBytes;
        double percentage;
        std::string status;

        if (!getDownloadProgress(modelId, downloadedBytes, totalBytes, percentage, status))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
            continue;
        }

        // Call the progress callback with all details
        progressCallback(percentage, status, downloadedBytes, totalBytes);

        // Check if download is complete
        if (status == "completed" || status == "creating_engine")
        {
            return true;
        }
        else if (status == "failed" || status == "cancelled")
        {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
    }
}

bool KolosalServerClient::makeGetRequest(const std::string &endpoint, std::string &response)
{
    std::string url = m_baseUrl + endpoint;

    // Prepare headers
    std::vector<std::string> headers;
    if (!m_apiKey.empty())
    {
        headers.push_back("X-API-Key: " + m_apiKey);
    }

    // Use existing HTTP client
    HttpClient client;
    return client.get(url, response, headers);
}

bool KolosalServerClient::makePostRequest(const std::string &endpoint, const std::string &payload, std::string &response)
{
    std::string url = m_baseUrl + endpoint;

    // Prepare headers
    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    if (!m_apiKey.empty())
    {
        headers.push_back("X-API-Key: " + m_apiKey);
    }

    // Use existing HTTP client
    HttpClient client;
    return client.post(url, payload, response, headers);
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
        // JSON parsing failed
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
        // JSON parsing failed
    }
    return false;
}

bool KolosalServerClient::shutdownServer()
{
    // First try the API endpoint if it exists
    std::string response;
    if (makePostRequest("/v1/shutdown", "{}", response))
    {
        std::cout << "Shutdown request sent to server via API." << std::endl;

        // Wait a moment for the server to shutdown gracefully
        std::this_thread::sleep_for(std::chrono::seconds(2));

        return true;
    }

    // If API shutdown fails, try to find and terminate the server process
    std::cout << "API shutdown not available. Attempting to terminate server process..." << std::endl;

    // On Windows, find the kolosal-server process and terminate it
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create process snapshot." << std::endl;
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        std::cerr << "Failed to get first process." << std::endl;
        CloseHandle(hSnapshot);
        return false;
    }

    bool found = false;
    do
    {
        if (strcmp(pe32.szExeFile, "kolosal-server.exe") == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL)
            {
                if (TerminateProcess(hProcess, 0))
                {
                    std::cout << "Server process terminated successfully." << std::endl;
                    found = true;
                }
                else
                {
                    std::cerr << "Failed to terminate server process. Error: " << GetLastError() << std::endl;
                }
                CloseHandle(hProcess);
            }
            else
            {
                std::cerr << "Failed to open server process. Error: " << GetLastError() << std::endl;
            }
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    if (!found)
    {
        std::cout << "No kolosal-server.exe process found." << std::endl;
        return true; // Not an error if no process exists
    }

    return found;
#else
    std::cerr << "Process termination not implemented for this platform." << std::endl;
    return false;
#endif
}

bool KolosalServerClient::cancelDownload(const std::string &modelId)
{
    std::string response;
    std::string endpoint = "/downloads/" + modelId + "/cancel";

    if (!makePostRequest(endpoint, "{}", response))
    {
        return false;
    }

    try
    {
        json cancelJson = json::parse(response);
        bool success = cancelJson.value("success", false);
        return success;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing cancel response: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::cancelAllDownloads()
{
    std::string response;
    std::string endpoint = "/downloads";

    if (!makePostRequest(endpoint, "{}", response))
    {
        return false;
    }

    try
    {
        json cancelJson = json::parse(response);
        int cancelledCount = cancelJson.value("cancelled_count", 0);
        std::cout << "Cancelled " << cancelledCount << " downloads." << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing cancel all response: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalServerClient::updateConfigWithNewModel(const std::string& engineId, const std::string& modelPath)
{
    const std::string configPath = "config.yaml";
    
    try {
        // Check if config.yaml exists
        if (!std::filesystem::exists(configPath)) {
            std::cerr << "Warning: config.yaml not found, cannot update model configuration" << std::endl;
            return false;
        }
        
        // Read the existing config file
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            std::cerr << "Error: Cannot open config.yaml for reading" << std::endl;
            return false;
        }
        
        std::string configContent;
        std::string line;
        bool inModelsSection = false;
        bool modelsAdded = false;
        
        while (std::getline(configFile, line)) {
            configContent += line + "\n";
            
            // Check if we're entering the models section
            if (line.find("models:") == 0) {
                inModelsSection = true;
            }
            // Check if we're leaving the models section (next major section starts)
            else if (inModelsSection && !line.empty() && line[0] != ' ' && line[0] != '#' && line[0] != '-') {
                inModelsSection = false;
            }
        }
        configFile.close();
        
        // Generate the model entry to add
        std::string newModelEntry = "  - id: \"" + engineId + "\"\n"
                                   "    path: \"" + modelPath + "\"\n"
                                   "    load_at_startup: false\n"
                                   "    main_gpu_id: 0\n"
                                   "    preload_context: false\n"
                                   "    load_params:\n"
                                   "      n_ctx: 4096\n"
                                   "      n_keep: 2048\n"
                                   "      use_mmap: true\n"
                                   "      use_mlock: false\n"
                                   "      n_parallel: 1\n"
                                   "      cont_batching: true\n"
                                   "      warmup: false\n"
                                   "      n_gpu_layers: 50\n"
                                   "      n_batch: 2048\n"
                                   "      n_ubatch: 512\n";
        
        // Find where to insert the new model
        size_t modelsPos = configContent.find("models:");
        if (modelsPos != std::string::npos) {
            // Check if models section is empty (contains only comments)
            size_t afterModels = modelsPos + 7; // length of "models:"
            size_t nextSection = configContent.find("\n# =====", afterModels);
            if (nextSection == std::string::npos) {
                nextSection = configContent.length();
            }
            
            std::string modelsSection = configContent.substr(afterModels, nextSection - afterModels);
            
            // Check if there are already uncommented model entries
            bool hasActiveModels = modelsSection.find("\n  - id:") != std::string::npos;
            
            if (hasActiveModels) {
                // Add to existing models - find the end of the last model entry
                size_t lastModelEnd = configContent.rfind("      n_ubatch:", nextSection);
                if (lastModelEnd != std::string::npos) {
                    // Find the end of the line
                    size_t lineEnd = configContent.find("\n", lastModelEnd);
                    if (lineEnd != std::string::npos) {
                        configContent.insert(lineEnd + 1, "\n" + newModelEntry);
                    }
                } else {
                    // Fallback: add after the models: line
                    size_t lineEnd = configContent.find("\n", modelsPos);
                    if (lineEnd != std::string::npos) {
                        configContent.insert(lineEnd + 1, newModelEntry);
                    }
                }
            } else {
                // No active models, add as the first one after models:
                size_t lineEnd = configContent.find("\n", modelsPos);
                if (lineEnd != std::string::npos) {
                    configContent.insert(lineEnd + 1, newModelEntry);
                }
            }
        } else {
            std::cerr << "Warning: Could not find models section in config.yaml" << std::endl;
            return false;
        }
        
        // Write the updated config back to file
        std::ofstream outFile(configPath);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot open config.yaml for writing" << std::endl;
            return false;
        }
        
        outFile << configContent;
        outFile.close();
        
        std::cout << "✓ Updated config.yaml with model '" << engineId << "'" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating config.yaml: " << e.what() << std::endl;
        return false;
    }
}
