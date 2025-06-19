#include "hugging_face_client.h"
#include "http_client.h"
#include "model_file.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

const std::string HuggingFaceClient::API_BASE_URL = "https://huggingface.co/api";
const std::string HuggingFaceClient::USER_AGENT = "Kolosal-CLI/1.0";
const int HuggingFaceClient::REQUEST_TIMEOUT = 30;

std::vector<std::string> HuggingFaceClient::fetchKolosalModels() {
    std::vector<std::string> models;
    std::cout << "Initializing HTTP client...\n";
    
    HttpResponse response;
    std::string url = API_BASE_URL + "/models?search=kolosal&limit=50";
    
    if (!HttpClient::get(url, response)) {
        std::cerr << "Failed to fetch models from Hugging Face API" << std::endl;
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
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
        return models;
    }
    
    return models;
}

std::vector<ModelFile> HuggingFaceClient::fetchModelFiles(const std::string& modelId) {
    std::vector<ModelFile> modelFiles;
    std::cout << "Fetching files for model: " << modelId << "...\n";
    
    HttpResponse response;
    std::string url = API_BASE_URL + "/models/" + modelId + "/tree/main";
    
    if (!HttpClient::get(url, response)) {
        std::cerr << "Failed to fetch model files from Hugging Face API" << std::endl;
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
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
        return modelFiles;
    }
    
    // Sort by quantization priority (8-bit first)
    ModelFileUtils::sortByPriority(modelFiles);
    
    return modelFiles;
}
