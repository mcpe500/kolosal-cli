#include "hugging_face_client.h"
#include "http_client.h"
#include "model_file.h"
#include "cache_manager.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

const std::string HuggingFaceClient::API_BASE_URL = "https://huggingface.co/api";
const std::string HuggingFaceClient::USER_AGENT = "Kolosal-CLI/1.0";
const int HuggingFaceClient::REQUEST_TIMEOUT = 30;

std::vector<std::string> HuggingFaceClient::fetchKolosalModels() {
    std::cout << "Checking cache for Kolosal models...\n";
    
    // Try to get from cache first
    std::vector<std::string> cachedModels = CacheManager::getCachedModels();
    if (!cachedModels.empty()) {
        std::cout << "Using cached models (" << cachedModels.size() << " found)\n";
        return cachedModels;
    }
    
    // Cache miss - fetch from API
    std::vector<std::string> models;
    std::cout << "Fetching fresh data from Hugging Face API...\n";
    
    HttpResponse response;
    std::string url = API_BASE_URL + "/models?search=kolosal&limit=50";
      if (!HttpClient::get(url, response)) {
        std::cerr << "Failed to fetch models from Hugging Face API" << std::endl;
        
        // Try offline fallback - use cached data even if expired
        std::cout << "Attempting offline fallback using cached data..." << std::endl;
        std::vector<std::string> offlineModels = CacheManager::getCachedModelsOffline();
        if (!offlineModels.empty()) {
            std::cout << "Using offline cached data (" << offlineModels.size() << " models)" << std::endl;
            return offlineModels;
        }
        
        std::cout << "No offline data available" << std::endl;
        return models;
    }
    
    // Parse JSON response
    try {
        json jsonData = json::parse(response.data);
        
        // Extract model IDs
        if (jsonData.is_array()) {
            for (const auto& model : jsonData) {
                if (model.contains("id") && model["id"].is_string()) {
                    std::string modelId = model["id"];
                    // Only include models from kolosal organization
                    if (modelId.find("kolosal/") == 0) {
                        models.push_back(modelId);
                    }
                }
            }
        }
        
        // Cache the results for future use
        if (!models.empty()) {
            CacheManager::cacheModels(models);
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
        return models;
    }
    
    return models;
}

std::vector<ModelFile> HuggingFaceClient::fetchModelFiles(const std::string& modelId) {
    std::cout << "Checking cache for files of model: " << modelId << "...\n";
    
    // Try to get from cache first
    std::vector<ModelFile> cachedFiles = CacheManager::getCachedModelFiles(modelId);
    if (!cachedFiles.empty()) {
        std::cout << "Using cached files (" << cachedFiles.size() << " found)\n";
        return cachedFiles;
    }
    
    // Cache miss - fetch from API
    std::vector<ModelFile> modelFiles;
    std::cout << "Fetching fresh file data from Hugging Face API...\n";
    
    HttpResponse response;
    std::string url = API_BASE_URL + "/models/" + modelId + "/tree/main";
      if (!HttpClient::get(url, response)) {
        std::cerr << "Failed to fetch model files from Hugging Face API" << std::endl;
        
        // Try offline fallback - use cached data even if expired
        std::cout << "Attempting offline fallback using cached data..." << std::endl;
        std::vector<ModelFile> offlineFiles = CacheManager::getCachedModelFilesOffline(modelId);
        if (!offlineFiles.empty()) {
            std::cout << "Using offline cached data (" << offlineFiles.size() << " files)" << std::endl;
            return offlineFiles;
        }
        
        std::cout << "No offline data available for " << modelId << std::endl;
        return modelFiles;
    }
    
    // Parse JSON response
    try {
        json jsonData = json::parse(response.data);
        
        // Extract .gguf files
        if (jsonData.is_array()) {
            for (const auto& item : jsonData) {
                if (item.contains("type") && item["type"].is_string() && 
                    item["type"] == "file" && item.contains("path") && item["path"].is_string()) {
                    std::string filename = item["path"];
                    // Check if file has .gguf extension
                    if (filename.length() >= 5 && filename.substr(filename.length() - 5) == ".gguf") {
                        ModelFile modelFile;
                        modelFile.filename = filename;
                        modelFile.quant = ModelFileUtils::detectQuantization(filename);
                        modelFiles.push_back(modelFile);
                    }
                }
            }
        }
        
        // Sort by quantization priority (8-bit first)
        ModelFileUtils::sortByPriority(modelFiles);
        
        // Cache the results for future use
        if (!modelFiles.empty()) {
            CacheManager::cacheModelFiles(modelId, modelFiles);
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
        return modelFiles;
    }
    
    return modelFiles;
}
