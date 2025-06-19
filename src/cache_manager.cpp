#include "cache_manager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

using json = nlohmann::json;

// Static member definitions
std::map<std::string, CacheEntry> CacheManager::memoryCache;
std::string CacheManager::cacheDirectory;
const int CacheManager::DEFAULT_TTL_SECONDS = 3600; // 1 hour for models list
const int CacheManager::MODEL_FILES_TTL_SECONDS = 1800; // 30 minutes for model files

bool CacheEntry::isValid(int ttlSeconds) const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
    return elapsed.count() < ttlSeconds;
}

void CacheManager::initialize() {
    // Set cache directory based on platform
#ifdef _WIN32
    // Windows: Use %APPDATA%\kolosal-cli\cache
    char* appDataPath = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appDataPath, &len, "APPDATA") == 0 && appDataPath != nullptr) {
        cacheDirectory = std::string(appDataPath) + "\\kolosal-cli\\cache";
        free(appDataPath);
    } else {
        cacheDirectory = ".\\cache"; // Fallback to current directory
    }
#else
    // Unix/Linux: Use ~/.cache/kolosal-cli
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    cacheDirectory = std::string(homeDir) + "/.cache/kolosal-cli";
#endif

    ensureCacheDirectory();
    std::cout << "Cache initialized at: " << cacheDirectory << std::endl;
}

void CacheManager::cleanup() {
    // Memory cache is automatically cleaned up when the map goes out of scope
    // Disk cache persists for future runs
}

void CacheManager::ensureCacheDirectory() {
    try {
        std::filesystem::create_directories(cacheDirectory);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create cache directory: " << e.what() << std::endl;
        // Fallback to current directory
        cacheDirectory = "./cache";
        try {
            std::filesystem::create_directories(cacheDirectory);
        } catch (const std::filesystem::filesystem_error& fallbackError) {
            std::cerr << "Failed to create fallback cache directory: " << fallbackError.what() << std::endl;
        }
    }
}

std::string CacheManager::getCacheFilePath(const std::string& key) {
    // Replace invalid filename characters
    std::string safeKey = key;
    for (char& c : safeKey) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return cacheDirectory + "/" + safeKey + ".cache";
}

CacheEntry CacheManager::loadFromDisk(const std::string& key) {
    CacheEntry entry;
    std::string filePath = getCacheFilePath(key);
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return entry; // Return empty entry if file doesn't exist
    }
    
    try {
        json cacheJson;
        file >> cacheJson;
        
        if (cacheJson.contains("data") && cacheJson.contains("timestamp")) {
            entry.data = cacheJson["data"];
            
            // Parse timestamp
            auto timestampMs = cacheJson["timestamp"].get<int64_t>();
            entry.timestamp = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(timestampMs)
            );
        }
    } catch (const json::exception& e) {
        std::cerr << "Failed to parse cache file " << filePath << ": " << e.what() << std::endl;
    }
    
    return entry;
}

void CacheManager::saveToDisk(const std::string& key, const CacheEntry& entry) {
    std::string filePath = getCacheFilePath(key);
    
    try {
        json cacheJson;
        cacheJson["data"] = entry.data;
        
        // Convert timestamp to milliseconds since epoch
        auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()
        ).count();
        cacheJson["timestamp"] = timestampMs;
        
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << cacheJson.dump(4);
        } else {
            std::cerr << "Failed to open cache file for writing: " << filePath << std::endl;
        }
    } catch (const json::exception& e) {
        std::cerr << "Failed to save cache file " << filePath << ": " << e.what() << std::endl;
    }
}

std::vector<std::string> CacheManager::getCachedModels() {
    const std::string cacheKey = "kolosal_models";
      // Check memory cache first
    auto memIt = memoryCache.find(cacheKey);
    if (memIt != memoryCache.end() && memIt->second.isValid(DEFAULT_TTL_SECONDS)) {
        return jsonToVector(memIt->second.data);
    }
    
    // Check disk cache
    CacheEntry diskEntry = loadFromDisk(cacheKey);
    if (!diskEntry.data.empty() && diskEntry.isValid(DEFAULT_TTL_SECONDS)) {
        // Update memory cache
        memoryCache[cacheKey] = diskEntry;
        return jsonToVector(diskEntry.data);
    }
    
    return {};
}

void CacheManager::cacheModels(const std::vector<std::string>& models) {
    const std::string cacheKey = "kolosal_models";
    
    CacheEntry entry;
    entry.data = vectorToJson(models);
    entry.timestamp = std::chrono::system_clock::now();
      // Save to both memory and disk
    memoryCache[cacheKey] = entry;
    saveToDisk(cacheKey, entry);
}

std::vector<ModelFile> CacheManager::getCachedModelFiles(const std::string& modelId) {
    const std::string cacheKey = "model_files_" + modelId;
      // Check memory cache first
    auto memIt = memoryCache.find(cacheKey);
    if (memIt != memoryCache.end() && memIt->second.isValid(MODEL_FILES_TTL_SECONDS)) {
        return jsonToModelFiles(memIt->second.data);
    }
    
    // Check disk cache
    CacheEntry diskEntry = loadFromDisk(cacheKey);
    if (!diskEntry.data.empty() && diskEntry.isValid(MODEL_FILES_TTL_SECONDS)) {
        // Update memory cache
        memoryCache[cacheKey] = diskEntry;
        return jsonToModelFiles(diskEntry.data);
    }
    
    return {};
}

void CacheManager::cacheModelFiles(const std::string& modelId, const std::vector<ModelFile>& files) {
    const std::string cacheKey = "model_files_" + modelId;
    
    CacheEntry entry;
    entry.data = modelFilesToJson(files);
    entry.timestamp = std::chrono::system_clock::now();
      // Save to both memory and disk
    memoryCache[cacheKey] = entry;
    saveToDisk(cacheKey, entry);
}

void CacheManager::clearCache() {
    // Clear memory cache
    memoryCache.clear();
    
    // Clear disk cache
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cacheDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                std::filesystem::remove(entry.path());
            }
        }
        std::cout << "Cache cleared successfully" << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {        std::cerr << "Failed to clear disk cache: " << e.what() << std::endl;
    }
}

std::vector<std::string> CacheManager::getCachedModelsOffline() {
    const std::string cacheKey = "kolosal_models";
    
    // Check memory cache first (ignore TTL for offline use)
    auto memIt = memoryCache.find(cacheKey);
    if (memIt != memoryCache.end() && !memIt->second.data.empty()) {
        std::cout << "Offline cache hit (memory): Found cached models" << std::endl;
        return jsonToVector(memIt->second.data);
    }
    
    // Check disk cache (ignore TTL for offline use)
    CacheEntry diskEntry = loadFromDisk(cacheKey);
    if (!diskEntry.data.empty()) {
        std::cout << "Offline cache hit (disk): Found cached models" << std::endl;
        // Update memory cache
        memoryCache[cacheKey] = diskEntry;
        return jsonToVector(diskEntry.data);
    }
    
    std::cout << "No cached models available for offline use" << std::endl;
    return {};
}

std::vector<ModelFile> CacheManager::getCachedModelFilesOffline(const std::string& modelId) {
    const std::string cacheKey = "model_files_" + modelId;
    
    // Check memory cache first (ignore TTL for offline use)
    auto memIt = memoryCache.find(cacheKey);
    if (memIt != memoryCache.end() && !memIt->second.data.empty()) {
        std::cout << "Offline cache hit (memory): Found cached files for " << modelId << std::endl;
        return jsonToModelFiles(memIt->second.data);
    }
    
    // Check disk cache (ignore TTL for offline use)
    CacheEntry diskEntry = loadFromDisk(cacheKey);
    if (!diskEntry.data.empty()) {
        std::cout << "Offline cache hit (disk): Found cached files for " << modelId << std::endl;
        // Update memory cache
        memoryCache[cacheKey] = diskEntry;
        return jsonToModelFiles(diskEntry.data);
    }
    
    std::cout << "No cached files available for offline use for " << modelId << std::endl;    return {};
}

bool CacheManager::hasAnyCachedData() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cacheDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cache") {
                return true;
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Directory doesn't exist or can't be accessed
        return false;
    }
    return false;
}

std::string CacheManager::vectorToJson(const std::vector<std::string>& items) {
    json j = items;
    return j.dump();
}

std::vector<std::string> CacheManager::jsonToVector(const std::string& jsonStr) {
    try {
        json j = json::parse(jsonStr);
        return j.get<std::vector<std::string>>();
    } catch (const json::exception& e) {
        std::cerr << "Failed to parse JSON to vector: " << e.what() << std::endl;
        return {};
    }
}

std::string CacheManager::modelFilesToJson(const std::vector<ModelFile>& files) {
    json j = json::array();
    
    for (const auto& file : files) {
        json fileJson;
        fileJson["filename"] = file.filename;
        fileJson["quant_type"] = file.quant.type;
        fileJson["quant_description"] = file.quant.description;
        fileJson["quant_priority"] = file.quant.priority;
        j.push_back(fileJson);
    }
    
    return j.dump();
}

std::vector<ModelFile> CacheManager::jsonToModelFiles(const std::string& jsonStr) {
    std::vector<ModelFile> files;
    
    try {
        json j = json::parse(jsonStr);
        
        for (const auto& fileJson : j) {
            ModelFile file;
            file.filename = fileJson["filename"];
            file.quant.type = fileJson["quant_type"];
            file.quant.description = fileJson["quant_description"];
            file.quant.priority = fileJson["quant_priority"];
            files.push_back(file);
        }
    } catch (const json::exception& e) {
        std::cerr << "Failed to parse JSON to ModelFiles: " << e.what() << std::endl;
    }
    
    return files;
}
