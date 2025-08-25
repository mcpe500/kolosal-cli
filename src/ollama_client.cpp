#include "ollama_client.h"
#include "http_client.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>

using json = nlohmann::json;

// Constants
const std::string OllamaClient::API_BASE_URL = "http://localhost:11434/api";
const std::string OllamaClient::USER_AGENT = "Kolosal-CLI/1.0";
const int OllamaClient::REQUEST_TIMEOUT = 30;
const int OllamaClient::MAX_RETRIES = 3;
const int OllamaClient::RETRY_DELAY_MS = 1000;
const int OllamaClient::CACHE_EXPIRATION_TIME = 300; // 5 minutes

// OllamaModel implementation
OllamaModel::OllamaModel() : size(0) {}

OllamaModel::OllamaModel(const std::string& name,
                         const std::string& displayName,
                         const std::string& modifiedAt,
                         long long size,
                         const std::string& digest,
                         const std::string& format,
                         const std::string& family,
                         const std::string& parameterSize,
                         const std::string& quantization)
    : name(name), displayName(displayName), modifiedAt(modifiedAt), size(size),
      digest(digest), format(format), family(family), parameterSize(parameterSize),
      quantization(quantization) {}

std::string OllamaModel::getFormattedSize() const {
    return OllamaClient::formatSize(size);
}

bool OllamaModel::isSameAs(const OllamaModel& other) const {
    return name == other.name && digest == other.digest;
}

// OllamaClient implementation
bool OllamaClient::isServerRunning() {
    HttpResponse response;
    std::string url = API_BASE_URL + "/tags";
    
    // Try to make a simple request to check if server is running
    return HttpClient::get(url, response);
}

std::vector<OllamaModel> OllamaClient::listLocalModels() {
    std::vector<OllamaModel> models;
    
    // Check if server is running
    if (!isServerRunning()) {
        std::cerr << "Ollama server is not running. Please start Ollama first." << std::endl;
        return models;
    }
    
    LoadingAnimation loader("Fetching local Ollama models");
    loader.start();
    
    HttpResponse response;
    std::string url = API_BASE_URL + "/tags";
    
    if (!HttpClient::get(url, response)) {
        loader.stop();
        std::cerr << "Failed to fetch models from Ollama API" << std::endl;
        return models;
    }
    
    try {
        models = parseModelList(response.data);
        loader.complete("Found " + std::to_string(models.size()) + " local models");
    } catch (const json::exception& e) {
        loader.stop();
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
    }
    
    return models;
}

bool OllamaClient::pullModel(const std::string& modelName) {
    if (!validateModelName(modelName)) {
        std::cerr << "Invalid model name format: " << modelName << std::endl;
        return false;
    }
    
    // Check if server is running
    if (!isServerRunning()) {
        std::cerr << "Ollama server is not running. Please start Ollama first." << std::endl;
        return false;
    }
    
    json payload;
    payload["name"] = modelName;
    
    std::string url = API_BASE_URL + "/pull";
    std::string payloadStr = payload.dump();
    
    // Headers for JSON content
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "User-Agent: " + USER_AGENT
    };
    
    std::string response;
    HttpClient httpClient;
    
    // Use streaming request for pull operation to show progress
    bool success = httpClient.makeStreamingRequest(url, payloadStr, 
        "Content-Type: application/json\r\nUser-Agent: " + USER_AGENT + "\r\n",
        [&modelName](const std::string& chunk) {
            // Process each chunk of the streaming response
            try {
                json chunkJson = json::parse(chunk);
                if (chunkJson.contains("status")) {
                    std::string status = chunkJson["status"];
                    if (chunkJson.contains("total") && chunkJson.contains("completed")) {
                        long long total = chunkJson["total"];
                        long long completed = chunkJson["completed"];
                        showPullProgress(modelName, status, completed, total);
                    } else {
                        showPullProgress(modelName, status, 0, 0);
                    }
                }
            } catch (const json::exception& e) {
                // Ignore parsing errors for individual chunks
            }
        });
    
    if (!success) {
        std::cerr << "Failed to pull model: " << modelName << std::endl;
        return false;
    }
    
    std::cout << "Model " << modelName << " pulled successfully!" << std::endl;
    return true;
}

std::vector<OllamaModel> OllamaClient::searchModels(const std::string& query) {
    // For now, we'll return all local models since Ollama doesn't have a search endpoint
    // In a real implementation, this would query a search API
    std::vector<OllamaModel> allModels = listLocalModels();
    std::vector<OllamaModel> filteredModels;
    
    if (query.empty()) {
        return allModels;
    }
    
    // Simple filtering by name
    for (const auto& model : allModels) {
        if (model.name.find(query) != std::string::npos) {
            filteredModels.push_back(model);
        }
    }
    
    return filteredModels;
}

std::pair<std::string, std::string> OllamaClient::parseModelName(const std::string& fullName) {
    size_t colonPos = fullName.find(':');
    if (colonPos == std::string::npos) {
        // No tag specified, default to "latest"
        return std::make_pair(fullName, "latest");
    }
    
    std::string name = fullName.substr(0, colonPos);
    std::string tag = fullName.substr(colonPos + 1);
    
    return std::make_pair(name, tag);
}

ModelFile OllamaClient::convertToModelFile(const OllamaModel& ollamaModel) {
    ModelFile modelFile;
    modelFile.filename = ollamaModel.name + ".gguf"; // Ollama models are already in GGUF format
    modelFile.modelId = "ollama/" + ollamaModel.name;
    
    // Detect quantization from the model name
    modelFile.quant = ModelFileUtils::detectQuantization(ollamaModel.name);
    
    // For Ollama models, we don't have a direct download URL since they're managed by Ollama
    // We'll set a special URL that indicates this is an Ollama model
    modelFile.downloadUrl = "ollama://" + ollamaModel.name;
    
    // Start async memory usage calculation
    modelFile.memoryUsage = ModelFileUtils::calculateMemoryUsageAsync(modelFile, 4096);
    
    return modelFile;
}

// Private methods
void OllamaClient::showPullProgress(const std::string& modelName,
                                   const std::string& status,
                                   long long completed,
                                   long long total) {
    if (total > 0) {
        double percentage = (static_cast<double>(completed) / total) * 100.0;
        std::cout << "\rPulling " << modelName << ": " << status 
                  << " (" << static_cast<int>(percentage) << "%)" << std::flush;
    } else {
        std::cout << "\rPulling " << modelName << ": " << status << std::flush;
    }
}

std::vector<OllamaModel> OllamaClient::parseModelList(const std::string& jsonData) {
    std::vector<OllamaModel> models;
    
    try {
        json parsedJson = json::parse(jsonData);
        
        if (parsedJson.contains("models") && parsedJson["models"].is_array()) {
            for (const auto& modelJson : parsedJson["models"]) {
                OllamaModel model = parseModel(modelJson.dump());
                models.push_back(model);
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing model list: " << e.what() << std::endl;
        throw;
    }
    
    return models;
}

OllamaModel OllamaClient::parseModel(const std::string& modelJson) {
    OllamaModel model;
    
    try {
        json parsedJson = json::parse(modelJson);
        
        if (parsedJson.contains("name") && parsedJson["name"].is_string()) {
            model.name = parsedJson["name"];
            model.displayName = model.name; // Use name as display name by default
        }
        
        if (parsedJson.contains("modified_at") && parsedJson["modified_at"].is_string()) {
            model.modifiedAt = parsedJson["modified_at"];
        }
        
        if (parsedJson.contains("size") && parsedJson["size"].is_number()) {
            model.size = parsedJson["size"];
        }
        
        if (parsedJson.contains("digest") && parsedJson["digest"].is_string()) {
            model.digest = parsedJson["digest"];
        }
        
        if (parsedJson.contains("details") && parsedJson["details"].is_object()) {
            json details = parsedJson["details"];
            
            if (details.contains("format") && details["format"].is_string()) {
                model.format = details["format"];
            }
            
            if (details.contains("family") && details["family"].is_string()) {
                model.family = details["family"];
            }
            
            if (details.contains("parameter_size") && details["parameter_size"].is_string()) {
                model.parameterSize = details["parameter_size"];
            }
            
            if (details.contains("quantization_level") && details["quantization_level"].is_string()) {
                model.quantization = details["quantization_level"];
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing individual model: " << e.what() << std::endl;
        throw;
    }
    
    return model;
}

bool OllamaClient::handlePullStream(const std::string& responseStream, const std::string& modelName) {
    // This method would handle streaming responses, but for now we're using the HttpClient's
    // streaming functionality directly in pullModel
    return true;
}

bool OllamaClient::validateModelName(const std::string& modelName) {
    // Basic validation - model name should not be empty
    return !modelName.empty();
}

std::string OllamaClient::formatSize(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unitIndex];
    return oss.str();
}

std::string OllamaClient::extractFamily(const std::string& modelName) {
    // Simple extraction - take the part before the first number or dash
    size_t pos = modelName.find_first_of("0123456789-");
    if (pos != std::string::npos) {
        return modelName.substr(0, pos);
    }
    return modelName;
}