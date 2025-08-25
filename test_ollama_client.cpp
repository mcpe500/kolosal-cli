#include "include/ollama_client.h"
#include <iostream>

int main() {
    std::cout << "Testing Ollama client...\n";
    
    // Test if Ollama server is running
    bool isRunning = OllamaClient::isServerRunning();
    std::cout << "Ollama server is " << (isRunning ? "running" : "not running") << std::endl;
    
    if (isRunning) {
        // Test listing local models
        std::vector<OllamaModel> models = OllamaClient::listLocalModels();
        std::cout << "Found " << models.size() << " local models:\n";
        
        for (const auto& model : models) {
            std::cout << "  - " << model.name << " (" << model.getFormattedSize() << ")\n";
        }
        
        // Test parsing model name
        std::pair<std::string, std::string> parsed = OllamaClient::parseModelName("llama3:8b");
        std::cout << "Parsed model name: " << parsed.first << ", tag: " << parsed.second << std::endl;
        
        // Test converting to ModelFile
        if (!models.empty()) {
            ModelFile modelFile = OllamaClient::convertToModelFile(models[0]);
            std::cout << "Converted to ModelFile: " << modelFile.filename << std::endl;
        }
    }
    
    return 0;
}