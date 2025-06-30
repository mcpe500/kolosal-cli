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
#include <windows.h>
#include <tlhelp32.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

        std::string sameDirPath = exeDir + "\\kolosal-server.exe";

        DWORD fileAttr = GetFileAttributesA(sameDirPath.c_str());
        if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            actualServerPath = sameDirPath;
        }
        else
        {
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
                actualServerPath = "kolosal-server.exe";
            }
        }
    } 
    
    std::string commandLine = "kolosal-server.exe";

    std::string workingDir = actualServerPath.substr(0, actualServerPath.find_last_of("\\/"));
    if (workingDir == actualServerPath)
    {
        workingDir = ".";
    }
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    
    BOOL result = CreateProcessA(
        actualServerPath.c_str(),
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
    
    // Try both /v1/engines and /engines endpoints
    if (!makeGetRequest("/v1/engines", response))
    {
        if (!makeGetRequest("/engines", response))
        {
            return false;
        }
    }

    try
    {
        json enginesJson = json::parse(response);
        engines.clear();
        
        // Handle different possible response formats
        if (enginesJson.is_array())
        {
            for (const auto& engine : enginesJson)
            {
                if (engine.contains("id"))
                {
                    engines.push_back(engine["id"].get<std::string>());
                }
                else if (engine.contains("engine_id"))
                {
                    engines.push_back(engine["engine_id"].get<std::string>());
                }
            }
        }
        else if (enginesJson.contains("engines") && enginesJson["engines"].is_array())
        {
            for (const auto& engine : enginesJson["engines"])
            {
                if (engine.contains("id"))
                {
                    engines.push_back(engine["id"].get<std::string>());
                }
                else if (engine.contains("engine_id"))
                {
                    engines.push_back(engine["engine_id"].get<std::string>());
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
    try
    {
        // Check if engine already exists
        if (engineExists(engineId))
        {
            return true;
        }

        // No loading animation for cleaner output - just show final result
        json payload;
        payload["engine_id"] = engineId;
        payload["model_path"] = modelUrl; // For download, we pass URL as model_path
        payload["load_immediately"] = false;
        payload["main_gpu_id"] = 0;
        
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
        if (!makePostRequest("/engines", payload.dump(), response))
        {
            return false;
        }
        
        std::string pathToStore = modelPath.empty() ? modelUrl : modelPath;
        updateConfigWithNewModel(engineId, pathToStore);
        return true;
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
    std::string endpoint = "/v1/download-progress/" + modelId;

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
    // Check if kolosal-server.exe process exists first
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return true;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return true;
    }

    bool processFound = false;
    DWORD serverPid = 0;
    do
    {
        if (_wcsicmp(pe32.szExeFile, L"kolosal-server.exe") == 0)
        {
            processFound = true;
            serverPid = pe32.th32ProcessID;
            break;
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    if (!processFound)
    {
        return true;
    }

    LoadingAnimation loading("Shutting down server");
    loading.start();

    // Terminate the process directly
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, serverPid);
    if (hProcess != NULL)
    {
        if (TerminateProcess(hProcess, 0))
        {
            loading.complete("Server shutdown successfully");
            CloseHandle(hProcess);
            return true;
        }
        else
        {
            loading.stop();
            std::cerr << "Error: Failed to terminate server process" << std::endl;
            CloseHandle(hProcess);
            return false;
        }
    }
    else
    {
        loading.stop();
        std::cerr << "Error: Failed to access server process for termination" << std::endl;
        return false;
    }
#else
    std::cerr << "Error: Server termination not supported on this platform" << std::endl;
    return false;
#endif
}

bool KolosalServerClient::cancelDownload(const std::string &modelId)
{
    LoadingAnimation loading("Cancelling download");
    loading.start();

    std::string response;
    std::string endpoint = "/downloads/" + modelId + "/cancel";

    if (!makePostRequest(endpoint, "{}", response))
    {
        loading.stop();
        return false;
    }

    try
    {
        json cancelJson = json::parse(response);
        bool success = cancelJson.value("success", false);
        if (success) {
            loading.complete("Download cancelled");
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
    std::string endpoint = "/downloads";

    if (!makePostRequest(endpoint, "{}", response))
    {
        loading.stop();
        return false;
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

bool KolosalServerClient::updateConfigWithNewModel(const std::string& engineId, const std::string& modelPath)
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
        
        std::string newModelEntry = "  - id: \"" + engineId + "\"\n"
                                   "    path: \"" + modelPath + "\"\n"
                                   "    load_immediately: false\n"
                                   "    main_gpu_id: 0\n"
                                   "    preload_context: true\n"
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
            {"stream", false},
            {"max_tokens", 2048},
            {"temperature", 0.7}
        };

        std::string jsonResponse;
        if (!makePostRequest("/v1/chat/completions", requestBody.dump(), jsonResponse)) {
            return false;
        }

        // Parse the response
        json responseJson = json::parse(jsonResponse);
        if (responseJson.contains("choices") && !responseJson["choices"].empty()) {
            if (responseJson["choices"][0].contains("message") && 
                responseJson["choices"][0]["message"].contains("content")) {
                response = responseJson["choices"][0]["message"]["content"];
                return true;
            }
        }

        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool KolosalServerClient::streamingChatCompletion(const std::string& engineId, const std::string& message, 
                                                std::function<void(const std::string&)> responseCallback)
{
    try {
        json requestBody = {
            {"model", engineId},
            {"messages", json::array({
                {{"role", "user"}, {"content", message}}
            })},
            {"stream", true},
            {"max_tokens", 2048},
            {"temperature", 0.7},
            {"top_p", 0.9}
        };

        // For streaming, we need to make a custom HTTP request to handle Server-Sent Events
        std::string url = m_baseUrl + "/v1/chat/completions";
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
                    
                    if (chunkJson.contains("choices") && !chunkJson["choices"].empty()) {
                        auto& choice = chunkJson["choices"][0];
                        if (choice.contains("delta")) {
                            auto& delta = choice["delta"];
                            if (delta.contains("content")) {
                                std::string content = delta["content"];
                                if (!content.empty()) {
                                    responseCallback(content);
                                    receivedContent = true;
                                }
                            }
                            // Also check for role in first chunk
                            if (delta.contains("role")) {
                                receivedContent = true; // Mark as having received valid streaming data
                            }
                        }
                        
                        // Check for finish_reason to detect end of stream
                        if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                            streamComplete = true;
                        }
                    }
                } catch (const std::exception& e) {
                    // Silently ignore JSON parse errors for malformed chunks
                }
            });

        // Consider streaming successful if we received any content or completed stream, regardless of HTTP status
        return receivedContent || streamComplete;
    } catch (const std::exception&) {
        return false;
    }
}
