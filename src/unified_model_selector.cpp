#include "unified_model_selector.h"
#include "hugging_face_client.h"
#include "ollama_client.h"
#include "interactive_list.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

UnifiedModelSelector::UnifiedModelSelector() 
    : huggingFaceClient(std::make_unique<HuggingFaceClient>()),
      ollamaClient(std::make_unique<OllamaClient>()),
      currentFilter(""),
      currentSourceFilter("all") {
}

UnifiedModelSelector::~UnifiedModelSelector() {
}

std::string UnifiedModelSelector::selectModel(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels) {
    // Load all models from available sources
    loadModels(configModels, downloadedModels);
    
    // Apply initial filters
    applyFilters();
    
    // Show interactive interface
    int selectedIndex = showInteractiveInterface();
    
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(filteredModels.size())) {
        const UnifiedModel& selectedModel = filteredModels[selectedIndex];
        
        // Return the appropriate identifier based on the source
        if (selectedModel.source == "local") {
            return "LOCAL:" + selectedModel.id;
        } else if (selectedModel.source == "ollama") {
            return "OLLAMA:" + selectedModel.id;
        } else if (selectedModel.source == "direct") {
            return "DIRECT_URL:" + selectedModel.url;
        } else {
            // Hugging Face or other sources
            return selectedModel.id;
        }
    }
    
    return ""; // Cancelled
}

void UnifiedModelSelector::loadModels(const std::vector<std::string>& configModels, const std::vector<std::string>& downloadedModels) {
    allModels.clear();
    
    // Load local models first
    loadLocalModels(configModels);
    
    // Load Hugging Face models
    loadHuggingFaceModels();
    
    // Load Ollama models
    loadOllamaModels();
    
    // For now, we're not implementing favorites or downloaded models as separate sections
    // but we could in the future
}

void UnifiedModelSelector::loadLocalModels(const std::vector<std::string>& configModels) {
    for (const auto& modelId : configModels) {
        UnifiedModel model;
        model.id = modelId;
        model.name = modelId;
        model.source = "local";
        model.sourceTag = "LOCAL";
        model.description = "Local model from config";
        model.size = -1; // Unknown
        model.quantization = "";
        model.parameterCount = "";
        model.url = "";
        model.format = "";
        allModels.push_back(model);
    }
}

void UnifiedModelSelector::loadHuggingFaceModels() {
    std::vector<std::string> hfModels = HuggingFaceClient::fetchKolosalModels();
    
    for (const auto& modelId : hfModels) {
        UnifiedModel model;
        model.id = modelId;
        model.name = modelId;
        model.source = "huggingface";
        model.sourceTag = "HF";
        model.description = "Hugging Face model";
        model.size = -1; // Would need to fetch this from API
        model.quantization = "";
        model.parameterCount = "";
        model.url = "https://huggingface.co/" + modelId;
        model.format = "GGUF";
        
        // Try to extract some information from the model ID
        size_t lastSlash = modelId.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string modelName = modelId.substr(lastSlash + 1);
            model.name = modelName;
            
            // Try to extract parameter count from common patterns
            // This is a simple heuristic and could be improved
            if (modelName.find("7B") != std::string::npos) {
                model.parameterCount = "7B";
            } else if (modelName.find("8B") != std::string::npos) {
                model.parameterCount = "8B";
            } else if (modelName.find("13B") != std::string::npos) {
                model.parameterCount = "13B";
            } else if (modelName.find("30B") != std::string::npos) {
                model.parameterCount = "30B";
            } else if (modelName.find("65B") != std::string::npos) {
                model.parameterCount = "65B";
            }
        }
        
        allModels.push_back(model);
    }
}

void UnifiedModelSelector::loadOllamaModels() {
    if (!ollamaClient->isServerHealthy()) {
        // Ollama server not running, add a placeholder
        UnifiedModel model;
        model.id = "ollama_not_running";
        model.name = "Ollama server not running";
        model.source = "info";
        model.sourceTag = "INFO";
        model.description = "Start Ollama to browse Ollama models";
        model.size = -1;
        model.quantization = "";
        model.parameterCount = "";
        model.url = "";
        model.format = "";
        allModels.push_back(model);
        return;
    }
    
    std::vector<std::string> ollamaModels = ollamaClient->listModels();
    
    for (const auto& modelId : ollamaModels) {
        UnifiedModel model;
        model.id = modelId;
        model.name = modelId;
        model.source = "ollama";
        model.sourceTag = "OL";
        model.description = "Ollama model";
        model.size = -1; // Would need to fetch this from API
        model.quantization = "";
        
        // Try to extract parameter count from common patterns
        if (modelId.find(":7b") != std::string::npos || modelId.find(":7B") != std::string::npos) {
            model.parameterCount = "7B";
        } else if (modelId.find(":8b") != std::string::npos || modelId.find(":8B") != std::string::npos) {
            model.parameterCount = "8B";
        } else if (modelId.find(":13b") != std::string::npos || modelId.find(":13B") != std::string::npos) {
            model.parameterCount = "13B";
        } else if (modelId.find(":30b") != std::string::npos || modelId.find(":30B") != std::string::npos) {
            model.parameterCount = "30B";
        } else if (modelId.find(":65b") != std::string::npos || modelId.find(":65B") != std::string::npos) {
            model.parameterCount = "65B";
        }
        
        model.url = "";
        model.format = "GGUF";
        allModels.push_back(model);
    }
}

void UnifiedModelSelector::applyFilters() {
    filteredModels.clear();
    
    // If no filter, show all models
    if (currentFilter.empty() && currentSourceFilter == "all") {
        filteredModels = allModels;
        return;
    }
    
    // Apply source filter first
    std::vector<UnifiedModel> sourceFiltered;
    if (currentSourceFilter == "all") {
        sourceFiltered = allModels;
    } else {
        for (const auto& model : allModels) {
            if (model.source == currentSourceFilter) {
                sourceFiltered.push_back(model);
            }
        }
    }
    
    // Apply text filter
    if (currentFilter.empty()) {
        filteredModels = sourceFiltered;
    } else {
        for (const auto& model : sourceFiltered) {
            // Simple case-insensitive search in model name and ID
            std::string lowerName = model.name;
            std::string lowerId = model.id;
            std::string lowerFilter = currentFilter;
            
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerId.begin(), lowerId.end(), lowerId.begin(), ::tolower);
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
            
            if (lowerName.find(lowerFilter) != std::string::npos || 
                lowerId.find(lowerFilter) != std::string::npos) {
                filteredModels.push_back(model);
            }
        }
    }
}

std::vector<std::string> UnifiedModelSelector::convertToDisplayStrings() const {
    std::vector<std::string> displayStrings;
    
    // Group models by source for better organization
    std::map<std::string, std::vector<const UnifiedModel*>> modelsBySource;
    
    for (const auto& model : filteredModels) {
        modelsBySource[model.source].push_back(&model);
    }
    
    // Add models grouped by source
    bool firstSource = true;
    for (const auto& sourcePair : modelsBySource) {
        const std::string& source = sourcePair.first;
        const std::vector<const UnifiedModel*>& models = sourcePair.second;
        
        // Add separator between sources (except before the first one)
        if (!firstSource) {
            displayStrings.push_back("────────────────────────────────────────");
        }
        firstSource = false;
        
        // Add source header
        std::string sourceName;
        if (source == "huggingface") {
            sourceName = "Hugging Face Models";
        } else if (source == "ollama") {
            sourceName = "Ollama Models";
        } else if (source == "local") {
            sourceName = "Local Config Models";
        } else if (source == "info") {
            sourceName = "Information";
        } else {
            sourceName = source + " Models";
        }
        
        displayStrings.push_back("=== " + sourceName + " ===");
        
        // Add models for this source
        for (const auto& model : models) {
            displayStrings.push_back(formatModelForDisplay(*model));
        }
    }
    
    // Add navigation option
    if (!displayStrings.empty()) {
        displayStrings.push_back("────────────────────────────────────────");
    }
    displayStrings.push_back("Back to Main Menu");
    
    return displayStrings;
}

std::string UnifiedModelSelector::formatModelForDisplay(const UnifiedModel& model) const {
    std::ostringstream oss;
    
    // Add source tag
    oss << "[" << model.sourceTag << "] ";
    
    // Add model name
    oss << model.name;
    
    // Add parameter count if available
    if (!model.parameterCount.empty()) {
        oss << " " << model.parameterCount;
    }
    
    // Add size if available
    if (model.size > 0) {
        // Format size in a human-readable way
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = static_cast<double>(model.size);
        
        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }
        
        oss << " (";
        if (unitIndex == 0) {
            oss << static_cast<long long>(size) << units[unitIndex];
        } else {
            oss << std::fixed << std::setprecision(1) << size << units[unitIndex];
        }
        oss << ")";
    }
    
    // Add quantization if available
    if (!model.quantization.empty()) {
        oss << " [" << model.quantization << "]";
    }
    
    return oss.str();
}

int UnifiedModelSelector::showInteractiveInterface() {
    std::vector<std::string> displayStrings = convertToDisplayStrings();
    
    InteractiveList menu(displayStrings);
    menu.setHeaderInfo("Kolosal CLI - Unified Model Selection | ↑/↓ Navigate | Enter Select | Esc Cancel");
    
    int result = menu.run();
    
    // Convert the result to the filtered models index
    // We need to account for the headers and separators
    if (result >= 0 && result < static_cast<int>(displayStrings.size()) - 1) { // -1 to exclude "Back to Main Menu"
        // Count how many non-model items (headers, separators) are before this index
        int modelCount = 0;
        for (int i = 0; i <= result; i++) {
            const std::string& item = displayStrings[i];
            // Check if this is a model item (not a header or separator)
            if (item.find("===") == std::string::npos && 
                item.find("────────") == std::string::npos &&
                item != "Back to Main Menu") {
                if (i == result) {
                    return modelCount; // Return the model index
                }
                modelCount++;
            }
        }
    }
    
    return -1; // Cancelled or "Back to Main Menu"
}