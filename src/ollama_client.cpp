#include "ollama_client.h"
#include "http_client.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

OllamaClient::OllamaClient() : baseUrl("http://localhost:11434/api") {
}

OllamaClient::~OllamaClient() {
}

bool OllamaClient::isServerHealthy() const {
    std::string response = makeGetRequest("/tags");
    return !response.empty();
}

std::vector<std::string> OllamaClient::listModels() const {
    std::vector<std::string> models;
    
    std::string response = makeGetRequest("/tags");
    if (response.empty()) {
        return models;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        if (json.contains("models") && json["models"].is_array()) {
            for (const auto& model : json["models"]) {
                if (model.contains("name") && model["name"].is_string()) {
                    models.push_back(model["name"]);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Ollama models response: " << e.what() << std::endl;
    }
    
    return models;
}

nlohmann::json OllamaClient::getModelDetails(const std::string& modelName) const {
    nlohmann::json emptyJson;
    
    nlohmann::json requestBody;
    requestBody["name"] = modelName;
    
    std::string response = makePostRequest("/show", requestBody);
    if (response.empty()) {
        return emptyJson;
    }
    
    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Ollama model details response: " << e.what() << std::endl;
        return emptyJson;
    }
}

bool OllamaClient::pullModel(const std::string& modelName) const {
    nlohmann::json requestBody;
    requestBody["name"] = modelName;
    
    std::string response = makePostRequest("/pull", requestBody);
    return !response.empty();
}

bool OllamaClient::modelExists(const std::string& modelName) const {
    std::vector<std::string> models = listModels();
    for (const auto& model : models) {
        if (model == modelName) {
            return true;
        }
    }
    return false;
}

std::string OllamaClient::makeGetRequest(const std::string& endpoint) const {
    std::string url = baseUrl + endpoint;
    return HttpClient::get(url);
}

std::string OllamaClient::makePostRequest(const std::string& endpoint, const nlohmann::json& body) const {
    std::string url = baseUrl + endpoint;
    std::string bodyStr = body.dump();
    return HttpClient::post(url, bodyStr, "application/json");
}