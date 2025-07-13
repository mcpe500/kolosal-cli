#include "model_file_selector.h"
#include "hugging_face_client.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include <iostream>
#include <thread>

ModelFileSelector::ModelFileSelector() {
}

ModelFileSelector::~ModelFileSelector() {
}

ModelFile ModelFileSelector::selectModelFile(const std::string& modelId) {
    return selectModelFile(modelId, "");
}

ModelFile ModelFileSelector::selectModelFile(const std::string& modelId, const std::string& headerInfo) {
    std::cout << "Selected model: " << modelId << std::endl;

    // Fetch .gguf files for the selected model
    std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFiles(modelId);
    if (modelFiles.empty()) {
        std::cout << "No .gguf files found. Showing sample files...\n\n";
        // Fallback to sample files with quantization info
        modelFiles = generateSampleFiles(modelId);
#ifdef _WIN32
#else

#endif
    }
    std::cout << "Found " << modelFiles.size() << " .gguf file(s)!\n\n";

    // Show model files with real-time async memory updates and cache after completion
    int fileResult = ModelFileUtils::displayAsyncModelFileList(modelFiles, "Select a .gguf file:", headerInfo);

    // Cache the files with completed memory information (runs in background)
    std::thread cacheThread([modelId, modelFiles]() mutable {
        ModelFileUtils::cacheModelFilesWithMemory(modelId, modelFiles);
    });
    cacheThread.detach();

    if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size())) {
        return modelFiles[fileResult];
    }

    return ModelFile{}; // Return empty ModelFile if cancelled or back selected
}

ModelFile ModelFileSelector::handleDirectGGUFUrl(const std::string& url) {
    std::cout << "Processing direct GGUF file URL...\n\n";

    // Extract filename from URL
    std::string filename;
    size_t lastSlash = url.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < url.length() - 1) {
        filename = url.substr(lastSlash + 1);
    } else {
        filename = "model.gguf"; // Fallback filename
    }
    
    // Create a ModelFile object for the direct URL
    ModelFile modelFile;
    modelFile.filename = filename;
    modelFile.downloadUrl = url;

    // Try to get cached model file info first
    std::string cacheKey = "direct_url:" + url;
    ModelFile cachedFile = CacheManager::getCachedModelFile(cacheKey);
    if (!cachedFile.filename.empty()) {
        std::cout << "Using cached information for: " << filename << std::endl;
        modelFile = cachedFile;
    } else {
        std::cout << "Analyzing GGUF file: " << filename << std::endl;
        LoadingAnimation loading("Reading metadata");
        loading.start();

        // Extract quantization info from filename
        modelFile.quant = ModelFileUtils::detectQuantization(filename);
        // Calculate memory usage
        modelFile.memoryUsage = ModelFileUtils::calculateMemoryUsageAsync(modelFile);

        loading.stop();

        // Cache the model file info
        CacheManager::cacheModelFile(cacheKey, modelFile);
        std::cout << "✓ Cached model information" << std::endl;
    }

    // Display file information
    std::cout << "File: " << filename << std::endl;
    std::cout << "URL: " << url << std::endl;
    std::cout << "Quantization: " << modelFile.quant.type << " - " << modelFile.quant.description << std::endl;
    if (modelFile.memoryUsage.hasEstimate) {
        std::cout << "Estimated Memory Usage: " << modelFile.memoryUsage.displayString << std::endl;
    }

    std::cout << std::endl;

    return modelFile;
}

void ModelFileSelector::showSelectionResult(const std::string& modelId, const ModelFile& modelFile) {
    std::cout << "Selected file: " << modelFile.filename << std::endl;
    std::cout << "Quantization: " << modelFile.quant.type << " - " << modelFile.quant.description << std::endl;
    std::cout << "From model: " << modelId << std::endl;
    std::cout << "Download URL: https://huggingface.co/" << modelId << "/resolve/main/" << modelFile.filename << std::endl;
    std::cout << "\nFile download feature coming soon!" << std::endl;
}

std::vector<ModelFile> ModelFileSelector::generateSampleFiles(const std::string& modelId) {
    std::vector<ModelFile> modelFiles;
    std::string modelName = modelId.substr(modelId.find('/') + 1);
    
    ModelFile file1;
    file1.filename = modelName + "-Q8_0.gguf";
    file1.modelId = modelId;
    file1.quant = ModelFileUtils::detectQuantization(file1.filename);
    file1.downloadUrl = "https://huggingface.co/" + modelId + "/resolve/main/" + file1.filename;
    file1.memoryUsage = ModelFileUtils::calculateMemoryUsageAsync(file1, 4096);
    modelFiles.push_back(file1);

    ModelFile file2;
    file2.filename = modelName + "-Q4_K_M.gguf";
    file2.modelId = modelId;
    file2.quant = ModelFileUtils::detectQuantization(file2.filename);
    file2.downloadUrl = "https://huggingface.co/" + modelId + "/resolve/main/" + file2.filename;
    file2.memoryUsage = ModelFileUtils::calculateMemoryUsageAsync(file2, 4096);
    modelFiles.push_back(file2);

    ModelFile file3;
    file3.filename = modelName + "-Q5_K_M.gguf";
    file3.modelId = modelId;
    file3.quant = ModelFileUtils::detectQuantization(file3.filename);
    file3.downloadUrl = "https://huggingface.co/" + modelId + "/resolve/main/" + file3.filename;
    file3.memoryUsage = ModelFileUtils::calculateMemoryUsageAsync(file3, 4096);
    modelFiles.push_back(file3);
    
    return modelFiles;
}
