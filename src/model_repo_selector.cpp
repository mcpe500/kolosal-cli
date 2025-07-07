#include "model_repo_selector.h"
#include "hugging_face_client.h"
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
