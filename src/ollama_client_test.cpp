#include "ollama_client.h"
#include <iostream>

int main() {
    OllamaClient client;
    
    std::cout << "Testing Ollama client...\n";
    
    // Test server health
    bool isHealthy = client.isServerHealthy();
    std::cout << "Server health: " << (isHealthy ? "OK" : "NOT AVAILABLE") << "\n";
    
    if (isHealthy) {
        // Test listing models
        std::vector<std::string> models = client.listModels();
        std::cout << "Found " << models.size() << " models:\n";
        for (const auto& model : models) {
            std::cout << "  - " << model << "\n";
        }
    } else {
        std::cout << "Ollama server is not running. Please start Ollama to test model listing.\n";
    }
    
    return 0;
}