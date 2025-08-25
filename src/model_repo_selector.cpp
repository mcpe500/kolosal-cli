#include "model_repo_selector.h"
#include "hugging_face_client.h"
#include "ollama_client.h"
#include "interactive_list.h"
#include <iostream>
#include <regex>
#include <algorithm>

#ifdef _WIN32
#else
#endif

ModelRepoSelector::ModelRepoSelector() {
}

ModelRepoSelector::~ModelRepoSelector() {
}

std::string ModelRepoSelector::selectModel() {
    std::cout << "Browsing Kolosal models...\n\n";

    // Fetch models from Hugging Face API
    std::vector<std::string> models = HuggingFaceClient::fetchKolosalModels();
    if (models.empty()) {
        std::cout << "No models found.\n";
        return ""; // Return empty if no models found
    }

    // Add navigation option
    models.push_back("Back to Main Menu");

    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1) {
        return models[result];
    }

    return ""; // Cancelled or back to main menu
}

std::string ModelRepoSelector::selectModel(const std::vector<std::string>& availableModels) {
    std::cout << "Browsing models...\n\n";

    std::vector<std::string> models;
    
    // Add available models from config at the top
    if (!availableModels.empty()) {
        std::cout << "Available models in config:\n";
        for (const auto& modelId : availableModels) {
            models.push_back("[Local] " + modelId);
        }
        // Add separator
        models.push_back("──────────────────────────");
    }

    // Fetch models from Hugging Face API
    std::vector<std::string> onlineModels = HuggingFaceClient::fetchKolosalModels();
    if (!onlineModels.empty()) {
        if (!availableModels.empty()) {
            std::cout << "Online Kolosal models:\n";
        }
        for (const auto& model : onlineModels) {
            models.push_back(model);
        }
    } else {
        // If fetching online models failed, show a message but continue
        if (!availableModels.empty()) {
            std::cout << "Note: Could not fetch online models from Hugging Face. Showing available local models only.\n";
        } else {
            std::cout << "Note: Could not fetch models from Hugging Face and no local models are available in config.\n";
            std::cout << "You can still use direct model URLs or local GGUF files.\n";
            return ""; // Return empty if no models found at all
        }
    }

    // Add navigation option
    models.push_back("Back to Main Menu");

    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1) {
        std::string selectedModel = models[result];
        
        // Check if it's a local model (starts with [Local])
        if (selectedModel.find("[Local] ") == 0) {
            // Extract the actual model ID (remove "[Local] ")
            std::string modelId = selectedModel.substr(8); // Remove "[Local] "
            return "LOCAL:" + modelId; // Special prefix to indicate it's a local model
        }
        
        return selectedModel;
    }

    return ""; // Cancelled or back to main menu
}

std::string ModelRepoSelector::parseRepositoryInput(const std::string& input) {
    if (input.empty()) {
        return "";
    }

    // Remove whitespace
    std::string trimmed = input;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());

    // Check if it's a direct GGUF file URL first
    if (isDirectGGUFUrl(trimmed)) {
        return "DIRECT_URL"; // Special marker for direct URLs
    }

    // Check if it's a full Hugging Face URL
    std::regex urlPattern(R"(https?://huggingface\.co/([^/]+/[^/?\s#]+))");
    std::smatch matches;

    if (std::regex_search(trimmed, matches, urlPattern)) {
        std::string modelId = matches[1].str();
        // Remove any trailing slashes or query parameters
        size_t pos = modelId.find_first_of("/?#");
        if (pos != std::string::npos) {
            modelId = modelId.substr(0, pos);
        }
        return modelId;
    }

    // Check if it's already in the correct format (owner/model-name)
    std::regex idPattern(R"(^[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+$)");
    if (std::regex_match(trimmed, idPattern)) {
        return trimmed;
    }

    return ""; // Invalid format
}

bool ModelRepoSelector::isDirectGGUFUrl(const std::string& input) {
    if (input.empty()) {
        return false;
    }

    // Check if it's a URL that ends with .gguf
    std::regex ggufUrlPattern(R"(^https?://[^\s]+\.gguf$)", std::regex_constants::icase);
    return std::regex_match(input, ggufUrlPattern);
}

std::string ModelRepoSelector::selectModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels) {
    std::cout << "Browsing models...\n\n";

    std::vector<std::string> models;
    
    // Add available models from config at the top
    if (!configModels.empty()) {
        std::cout << "Available models in config:\n";
        for (const auto& modelId : configModels) {
            models.push_back("[Local] " + modelId);
        }
        // Add separator
        models.push_back("──────────────────────────");
    }

    // Fetch models from Hugging Face API
    std::vector<std::string> onlineModels = HuggingFaceClient::fetchKolosalModels();
    if (!onlineModels.empty()) {
        if (!configModels.empty()) {
            std::cout << "Online Kolosal models:\n";
        }
        for (const auto& model : onlineModels) {
            models.push_back(model);
        }
    } else {
        // If fetching online models failed, show a message but continue
        if (!configModels.empty()) {
            std::cout << "Note: Could not fetch online models from Hugging Face. Showing available local models only.\n";
        } else {
            std::cout << "Note: Could not fetch models from Hugging Face and no local models are available.\n";
            std::cout << "You can still use direct model URLs or local GGUF files.\n";
            return ""; // Return empty if no models found at all
        }
    }

    // Add navigation option
    models.push_back("Back to Main Menu");

    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1) {
        std::string selectedModel = models[result];
        
        // Check if it's a local model (starts with [Local])
        if (selectedModel.find("[Local] ") == 0) {
            // Extract the actual model ID (remove "[Local] ")
            std::string modelId = selectedModel.substr(8); // Remove "[Local] "
            return "LOCAL:" + modelId; // Special prefix to indicate it's a local model
        }
        
        return selectedModel;
    }

    return ""; // Cancelled or back to main menu
}

std::string ModelRepoSelector::selectOllamaModel() {
    std::cout << "Browsing Ollama models...\n\n";

    // Check if Ollama server is running
    if (!OllamaClient::isServerRunning()) {
        std::cout << "Ollama server is not running. Please start Ollama first.\n";
        std::cout << "You can download and install Ollama from: https://ollama.com/\n\n";
        return ""; // Return empty if server is not running
    }

    // Fetch models from Ollama API
    std::vector<OllamaModel> ollamaModels = OllamaClient::listLocalModels();
    std::vector<std::string> models;
    
    // Convert Ollama models to string representations
    for (const auto& model : ollamaModels) {
        models.push_back(model.name + " (" + model.getFormattedSize() + ")");
    }
    
    if (models.empty()) {
        std::cout << "No Ollama models found locally.\n";
        std::cout << "You can pull models using: ollama pull <model-name>\n";
        std::cout << "Or visit https://ollama.com/library to browse available models.\n\n";
        return ""; // Return empty if no models found
    }

    // Add navigation option
    models.push_back("Back to Main Menu");

    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1) {
        // Return the model name without the size information
        std::string selectedModel = ollamaModels[result].name;
        return "OLLAMA:" + selectedModel; // Special prefix to indicate it's an Ollama model
    }

    return ""; // Cancelled or back to main menu
}
