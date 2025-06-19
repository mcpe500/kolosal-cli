#include "kolosal_cli.h"
#include "http_client.h"
#include "hugging_face_client.h"
#include "interactive_list.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <regex>
#include <algorithm>

void KolosalCLI::initialize() {
    HttpClient::initialize();
    CacheManager::initialize();
}

void KolosalCLI::cleanup() {
    CacheManager::cleanup();
    HttpClient::cleanup();
}

void KolosalCLI::showWelcome() {
    std::cout << "Welcome to Kolosal CLI!\n";
    std::cout << "🤗 Browse and download GGUF models from Hugging Face\n";
    std::cout << "(Using smart caching to reduce load times)\n";
    
    // Show offline capability status
    if (CacheManager::hasAnyCachedData()) {
        std::cout << "💾 Offline mode available - cached data found\n";
    }
    std::cout << std::endl;
}

std::vector<std::string> KolosalCLI::generateSampleModels() {
    return {
        "kolosal/sample-model-1",
        "kolosal/sample-model-2", 
        "kolosal/sample-model-3"
    };
}

std::vector<ModelFile> KolosalCLI::generateSampleFiles(const std::string& modelId) {
    std::vector<ModelFile> modelFiles;
    std::string modelName = modelId.substr(modelId.find('/') + 1);
    
    ModelFile file1;
    file1.filename = modelName + "-Q8_0.gguf";
    file1.quant = ModelFileUtils::detectQuantization(file1.filename);
    modelFiles.push_back(file1);
    
    ModelFile file2;
    file2.filename = modelName + "-Q4_K_M.gguf";
    file2.quant = ModelFileUtils::detectQuantization(file2.filename);
    modelFiles.push_back(file2);
    
    ModelFile file3;
    file3.filename = modelName + "-Q5_K_M.gguf";
    file3.quant = ModelFileUtils::detectQuantization(file3.filename);
    modelFiles.push_back(file3);
    
    return modelFiles;
}

std::string KolosalCLI::selectModel() {
    std::cout << "📚 Browsing Kolosal models...\n\n";
    
    // Fetch models from Hugging Face API
    std::vector<std::string> models = HuggingFaceClient::fetchKolosalModels();
      if (models.empty()) {
        std::cout << "No Kolosal models found or failed to fetch models.\n";
        std::cout << "This could be due to:\n";
        std::cout << "1. No internet connection\n";
        std::cout << "2. No public models in the kolosal organization\n";
        std::cout << "3. API request failed\n\n";
        std::cout << "Showing sample menu instead...\n\n";
        
        // Fallback to sample menu
        models = generateSampleModels();
        Sleep(2000);
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

ModelFile KolosalCLI::selectModelFile(const std::string& modelId) {
    std::cout << "You selected model: " << modelId << std::endl;
    
    // Fetch .gguf files for the selected model
    std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFiles(modelId);
    
    if (modelFiles.empty()) {
        std::cout << "No .gguf files found for this model.\n";
        std::cout << "This could be due to:\n";
        std::cout << "1. Model doesn't have quantized .gguf files\n";
        std::cout << "2. Files are in a different format\n";
        std::cout << "3. API request failed\n\n";
        std::cout << "Showing sample files instead...\n\n";
          // Fallback to sample files with quantization info
        modelFiles = generateSampleFiles(modelId);
        Sleep(2000);
    }
    
    // Convert ModelFile objects to display strings
    std::vector<std::string> displayFiles;
    for (const auto& file : modelFiles) {
        displayFiles.push_back(file.getDisplayName());
    }
    displayFiles.push_back("Back to Model Selection");
    
    // Show .gguf files in interactive list with default selection on Q8_0 (8-bit)
    InteractiveList fileMenu(displayFiles);
    
    int fileResult = fileMenu.run();
    
    if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size())) {
        return modelFiles[fileResult];
    }
    
    return ModelFile{}; // Return empty ModelFile if cancelled or back selected
}

void KolosalCLI::showSelectionResult(const std::string& modelId, const ModelFile& modelFile) {
    std::cout << "You selected file: " << modelFile.filename << std::endl;
    std::cout << "Quantization: " << modelFile.quant.type << " - " << modelFile.quant.description << std::endl;
    std::cout << "From model: " << modelId << std::endl;
    std::cout << "Download URL: https://huggingface.co/" << modelId << "/resolve/main/" << modelFile.filename << std::endl;
    std::cout << "\nFile download feature coming soon!" << std::endl;
}

std::string KolosalCLI::parseRepositoryInput(const std::string& input) {
    if (input.empty()) {
        return "";
    }
    
    // Remove whitespace
    std::string trimmed = input;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
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

bool KolosalCLI::isValidModelId(const std::string& modelId) {
    if (modelId.empty()) {
        return false;
    }
    
    // Check format: owner/model-name
    std::regex pattern(R"(^[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+$)");
    return std::regex_match(modelId, pattern);
}

int KolosalCLI::run(const std::string& repoId) {
    showWelcome();
    
    // If a repository ID is provided, go directly to file selection
    if (!repoId.empty()) {
        std::string modelId = parseRepositoryInput(repoId);
          if (modelId.empty()) {
            std::cout << "❌ Invalid repository URL or ID format.\n\n";
            std::cout << "Valid formats:\n";
            std::cout << "  • owner/model-name (e.g., microsoft/DialoGPT-medium)\n";
            std::cout << "  • https://huggingface.co/owner/model-name\n";
            return 1; // Exit with error code
        } else {
            std::cout << "🎯 Direct access to repository: " << modelId << "\n\n";
            
            // Fetch GGUF files from the specified repository
            std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFilesFromAnyRepo(modelId);
              if (modelFiles.empty()) {
                std::cout << "❌ No .gguf files found in repository: " << modelId << "\n";
                std::cout << "This repository may not contain quantized models.\n";
                return 1; // Exit with error code
            } else {
                // Show GGUF file selection
                std::vector<std::string> displayFiles;
                for (const auto& file : modelFiles) {
                    displayFiles.push_back(file.getDisplayName());
                }
                displayFiles.push_back("Exit");
                
                std::cout << "Found " << modelFiles.size() << " .gguf file(s) in " << modelId << "\n\n";
                
                InteractiveList fileMenu(displayFiles);
                int fileResult = fileMenu.run();
                
                if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size())) {
                    showSelectionResult(modelId, modelFiles[fileResult]);
                    return 0;
                }
                
                std::cout << "Selection cancelled." << std::endl;
                return 0;
            }
        }
    }
    
    // Standard flow: browse kolosal models
    while (true) {
        std::string selectedModel = selectModel();
        
        if (selectedModel.empty()) {
            std::cout << "Model selection cancelled." << std::endl;
            return 0;
        }
        
        ModelFile selectedFile = selectModelFile(selectedModel);
        
        if (selectedFile.filename.empty()) {
            // User selected "Back to Model Selection" - continue the loop
            continue;
        }
        
        showSelectionResult(selectedModel, selectedFile);
        return 0;
    }
}
