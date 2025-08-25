#include "model_repo_selector.h"
#include "hugging_face_client.h"
#include "ollama_client.h"
#include "unified_model_selector.h"
#include "interactive_list.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <map>

#ifdef _WIN32
#else
#endif

ModelRepoSelector::ModelRepoSelector() {
    ollamaClient = new OllamaClient();
}

ModelRepoSelector::~ModelRepoSelector() {
    if (ollamaClient) {
        delete ollamaClient;
    }
}

std::string ModelRepoSelector::selectModel() {
    std::cout << "Browsing Kolosal models...\n\n";

    // Use the unified model selector for a better experience
    std::vector<std::string> emptyModels;
    UnifiedModelSelector unifiedSelector;
    return unifiedSelector.selectModel(emptyModels, emptyModels);
}

std::string ModelRepoSelector::selectModel(const std::vector<std::string>& availableModels) {
    std::cout << "Browsing models...\n\n";

    // Use the unified model selector for a better experience
    std::vector<std::string> emptyModels;
    UnifiedModelSelector unifiedSelector;
    return unifiedSelector.selectModel(availableModels, emptyModels);
}

std::string ModelRepoSelector::selectUnifiedModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels) {
    std::cout << "Selecting models from Hugging Face and Ollama...\n\n";

    // Use the unified model selector for a better experience
    UnifiedModelSelector unifiedSelector;
    return unifiedSelector.selectModel(configModels, downloadedModels);
}

std::string ModelRepoSelector::selectModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels) {
    // Use the unified model selection interface
    return selectUnifiedModel(configModels, downloadedModels);
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