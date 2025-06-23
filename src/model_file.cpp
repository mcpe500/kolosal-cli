#include "model_file.h"
#include "http_client.h"
#include "cache_manager.h"
#include "interactive_list.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <future>
#include <chrono>
#include <iostream>
#include <windows.h>
#include <conio.h>
#include <curl/curl.h>

std::string ModelFile::getDisplayName() const
{
    // Extract model name from modelId (part after the last '/')
    std::string modelName = modelId;
    size_t lastSlash = modelName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        modelName = modelName.substr(lastSlash + 1);
    }
    
    // Convert to lowercase and replace underscores with hyphens
    std::transform(modelName.begin(), modelName.end(), modelName.begin(), [](char c) {
        return (c == '_') ? '-' : std::tolower(c);
    });
    
    // Format: "model-name:QUANT_TYPE"
    return modelName + ":" + quant.type;
}

std::string ModelFile::getDisplayNameWithMemory() const
{
    // Extract model name from modelId (part after the last '/')
    std::string modelName = modelId;
    size_t lastSlash = modelName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        modelName = modelName.substr(lastSlash + 1);
    }
    
    // Convert to lowercase and replace underscores with hyphens
    std::transform(modelName.begin(), modelName.end(), modelName.begin(), [](char c) {
        return (c == '_') ? '-' : std::tolower(c);
    });
    
    // Format: "model-name:QUANT_TYPE [Memory: info]"
    std::string result = modelName + ":" + quant.type;
    
    if (memoryUsage.isLoading)
    {
        result += " [Memory: calculating...]";
    }
    else if (memoryUsage.hasEstimate)
    {
        result += " [Memory: " + memoryUsage.displayString + "]";
    }
    
    return result;
}

QuantizationInfo ModelFileUtils::detectQuantization(const std::string &filename)
{
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    // Check for various quantization patterns (order matters - check most specific first)

    // Unsloth Dynamic (UD) variants - selective parameter quantization
    if (lower_filename.find("iq1_s") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-IQ1_S", "1-bit Unsloth Dynamic quantization (small), selective parameter quantization", 1};
    }
    else if (lower_filename.find("iq1_m") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-IQ1_M", "1-bit Unsloth Dynamic quantization (medium), selective parameter quantization", 2};
    }
    else if (lower_filename.find("iq2_xxs") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-IQ2_XXS", "2-bit Unsloth Dynamic quantization (extra extra small), selective parameter quantization", 3};
    }
    else if (lower_filename.find("iq2_m") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-IQ2_M", "2-bit Unsloth Dynamic quantization (medium), selective parameter quantization", 4};
    }
    else if (lower_filename.find("iq3_xxs") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-IQ3_XXS", "3-bit Unsloth Dynamic quantization (extra extra small), selective parameter quantization", 5};
    }
    else if (lower_filename.find("q2_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q2_K_XL", "2-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 6};
    }
    else if (lower_filename.find("q3_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q3_K_XL", "3-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 7};
    }
    else if (lower_filename.find("q4_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q4_K_XL", "4-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 8};
    }
    else if (lower_filename.find("q5_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q5_K_XL", "5-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 9};
    }
    else if (lower_filename.find("q6_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q6_K_XL", "6-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 10};
    }
    else if (lower_filename.find("q8_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos)
    {
        return {"UD-Q8_K_XL", "8-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 11};

        // Standard XL variants
    }
    else if (lower_filename.find("q8_k_xl") != std::string::npos)
    {
        return {"Q8_K_XL", "8-bit K-quantization (XL), maximum quality", 12};
    }
    else if (lower_filename.find("q6_k_xl") != std::string::npos)
    {
        return {"Q6_K_XL", "6-bit K-quantization (XL), very high quality", 13};
    }
    else if (lower_filename.find("q5_k_xl") != std::string::npos)
    {
        return {"Q5_K_XL", "5-bit K-quantization (XL), high quality", 14};
    }
    else if (lower_filename.find("q4_k_xl") != std::string::npos)
    {
        return {"Q4_K_XL", "4-bit K-quantization (XL), good quality", 15};
    }
    else if (lower_filename.find("q3_k_xl") != std::string::npos)
    {
        return {"Q3_K_XL", "3-bit K-quantization (XL), compact with quality", 16};
    }
    else if (lower_filename.find("q2_k_xl") != std::string::npos)
    {
        return {"Q2_K_XL", "2-bit K-quantization (XL), very compact", 17};
        // Standard 8-bit variants
    }
    else if (lower_filename.find("q8_0") != std::string::npos)
    {
        return {"Q8_0", "8-bit quantization, excellent quality", 18};

        // 6-bit variants
    }
    else if (lower_filename.find("q6_k") != std::string::npos)
    {
        return {"Q6_K", "6-bit quantization, high quality with smaller size", 19};

        // 5-bit variants
    }
    else if (lower_filename.find("q5_k_m") != std::string::npos)
    {
        return {"Q5_K_M", "5-bit quantization (medium), good quality/size balance", 20};
    }
    else if (lower_filename.find("q5_k_s") != std::string::npos)
    {
        return {"Q5_K_S", "5-bit quantization (small), smaller size", 21};
    }
    else if (lower_filename.find("q5_0") != std::string::npos)
    {
        return {"Q5_0", "5-bit quantization, legacy format", 22};

        // 4-bit variants
    }
    else if (lower_filename.find("iq4_nl") != std::string::npos)
    {
        return {"IQ4_NL", "4-bit improved quantization (no lookup), very efficient", 23};
    }
    else if (lower_filename.find("iq4_xs") != std::string::npos)
    {
        return {"IQ4_XS", "4-bit improved quantization (extra small), ultra compact", 24};
    }
    else if (lower_filename.find("q4_k_m") != std::string::npos)
    {
        return {"Q4_K_M", "4-bit quantization (medium), good for most use cases", 25};
    }
    else if (lower_filename.find("q4_k_l") != std::string::npos)
    {
        return {"Q4_K_L", "4-bit quantization (large), better quality at 4-bit", 26};
    }
    else if (lower_filename.find("q4_k_s") != std::string::npos)
    {
        return {"Q4_K_S", "4-bit quantization (small), very compact", 27};
    }
    else if (lower_filename.find("q4_1") != std::string::npos)
    {
        return {"Q4_1", "4-bit quantization v1, improved legacy format", 28};
    }
    else if (lower_filename.find("q4_0") != std::string::npos)
    {
        return {"Q4_0", "4-bit quantization, legacy format", 29};

        // 3-bit variants
    }
    else if (lower_filename.find("iq3_xxs") != std::string::npos)
    {
        return {"IQ3_XXS", "3-bit improved quantization (extra extra small), maximum compression", 30};
    }
    else if (lower_filename.find("q3_k_l") != std::string::npos)
    {
        return {"Q3_K_L", "3-bit quantization (large), experimental", 31};
    }
    else if (lower_filename.find("q3_k_m") != std::string::npos)
    {
        return {"Q3_K_M", "3-bit quantization (medium), very small size", 32};
    }
    else if (lower_filename.find("q3_k_s") != std::string::npos)
    {
        return {"Q3_K_S", "3-bit quantization (small), ultra compact", 33};

        // 2-bit variants
    }
    else if (lower_filename.find("iq2_xxs") != std::string::npos)
    {
        return {"IQ2_XXS", "2-bit improved quantization (extra extra small), extreme compression", 34};
    }
    else if (lower_filename.find("iq2_m") != std::string::npos)
    {
        return {"IQ2_M", "2-bit improved quantization (medium), balanced compression", 35};
    }
    else if (lower_filename.find("q2_k_l") != std::string::npos)
    {
        return {"Q2_K_L", "2-bit quantization (large), better quality at 2-bit", 36};
    }
    else if (lower_filename.find("q2_k") != std::string::npos)
    {
        return {"Q2_K", "2-bit quantization, extremely small but lower quality", 37};

        // 1-bit variants (experimental)
    }
    else if (lower_filename.find("iq1_s") != std::string::npos)
    {
        return {"IQ1_S", "1-bit improved quantization (small), experimental ultra compression", 38};
    }
    else if (lower_filename.find("iq1_m") != std::string::npos)
    {
        return {"IQ1_M", "1-bit improved quantization (medium), experimental compression", 39};

        // Floating point variants
    }
    else if (lower_filename.find("f16") != std::string::npos)
    {
        return {"F16", "16-bit floating point, highest quality but large size", 40};
    }
    else if (lower_filename.find("f32") != std::string::npos)
    {
        return {"F32", "32-bit floating point, original precision", 41};

        // Default
    }
    else
    {
        return {"Unknown", "Unknown quantization type", 42};
    }
}

MemoryUsage ModelFileUtils::calculateMemoryUsage(const ModelFile &modelFile, int contextSize)
{
    MemoryUsage usage;

    // Only calculate if we have a download URL
    if (!modelFile.downloadUrl.has_value())
    {
        return usage; // Return invalid usage (hasEstimate = false)
    }

    try
    {
        // Get the actual file size from URL
        size_t actualFileSizeBytes = getActualFileSizeFromUrl(modelFile.downloadUrl.value());

        if (actualFileSizeBytes == 0)
        {
            return usage; // Return invalid usage if file size can't be determined
        }
        // Convert to MB using decimal conversion (1 MB = 1,000,000 bytes) to match browser/file system display
        usage.modelSizeMB = actualFileSizeBytes / (1000 * 1000);

        // Read GGUF metadata to get model parameters for KV cache calculation
        GGUFMetadataReader reader;
        auto params = reader.readModelParams(modelFile.downloadUrl.value(), false);

        if (!params.has_value())
        {
            return usage; // Return invalid usage if metadata can't be read
        }

        // Calculate KV cache size using actual model parameters (convert to decimal MB for consistency)
        // Formula: 4 * hidden_size * hidden_layers * context_size / (1000 * 1000)
        float kvCacheSizeBytes = 4.0f *
                                 params->hidden_size *
                                 params->hidden_layers *
                                 contextSize;
        usage.kvCacheMB = static_cast<size_t>(kvCacheSizeBytes / (1000 * 1000));

        // Total required memory = model file size * 1.2 (20% overhead) + KV cache
        usage.totalRequiredMB = static_cast<size_t>(usage.modelSizeMB + usage.kvCacheMB);

        // Create display string
        std::ostringstream oss;
        oss << formatMemorySize(usage.totalRequiredMB)
            << " (Model: " << formatMemorySize(usage.modelSizeMB)
            << " + KV: " << formatMemorySize(usage.kvCacheMB) << ")";
        usage.displayString = oss.str();
        usage.hasEstimate = true;

        return usage;
    }
    catch (const std::exception &)
    {
        // Return invalid usage if any error occurs
        return usage;
    }
}

MemoryUsage ModelFileUtils::calculateMemoryUsageAsync(const ModelFile &modelFile, int contextSize)
{
    MemoryUsage usage;
    
    // Only calculate if we have a download URL
    if (!modelFile.downloadUrl.has_value())
    {
        return usage; // Return invalid usage (hasEstimate = false)
    }
    
    // Set loading state
    usage.isLoading = true;
    usage.hasEstimate = false;
    
    // Create async task
    auto asyncTask = std::make_shared<std::future<MemoryUsage>>(
        std::async(std::launch::async, [modelFile, contextSize]() -> MemoryUsage {
            return calculateMemoryUsage(modelFile, contextSize);
        })
    );
    
    usage.asyncResult = asyncTask;
    return usage;
}

bool ModelFileUtils::updateAsyncMemoryUsage(MemoryUsage& memoryUsage)
{
    if (!memoryUsage.isLoading || !memoryUsage.asyncResult)
    {
        return false; // Not in loading state or no async task
    }
    
    // Check if the async calculation is ready
    if (memoryUsage.asyncResult->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        try
        {
            // Get the result and update the memory usage
            MemoryUsage result = memoryUsage.asyncResult->get();
            
            memoryUsage.modelSizeMB = result.modelSizeMB;
            memoryUsage.kvCacheMB = result.kvCacheMB;
            memoryUsage.totalRequiredMB = result.totalRequiredMB;
            memoryUsage.displayString = result.displayString;
            memoryUsage.hasEstimate = result.hasEstimate;
            memoryUsage.isLoading = false;
            memoryUsage.asyncResult.reset(); // Clear the future
            
            return true; // Calculation completed
        }
        catch (const std::exception&)
        {
            // Error in calculation
            memoryUsage.isLoading = false;
            memoryUsage.hasEstimate = false;
            memoryUsage.asyncResult.reset();
            return true; // Calculation completed (with error)
        }
    }
    
    return false; // Still calculating
}

bool ModelFileUtils::updateAllAsyncMemoryUsage(std::vector<ModelFile>& modelFiles)
{
    bool anyCompleted = false;
    
    for (auto& modelFile : modelFiles)
    {
        if (updateAsyncMemoryUsage(modelFile.memoryUsage))
        {
            anyCompleted = true;
        }
    }
    
    return anyCompleted;
}

size_t ModelFileUtils::estimateModelSize(const GGUFModelParams &params, const std::string &quantType)
{
    // Calculate approximate model size based on parameters and quantization

    // Base calculation: hidden_size * hidden_layers * attention_heads
    uint64_t totalParams = static_cast<uint64_t>(params.hidden_size) *
                           params.hidden_layers *
                           params.attention_heads * 1000; // Rough multiplier

    // Bits per parameter based on quantization type
    float bitsPerParam = 16.0f; // Default F16    // Map quantization types to bits per parameter
    static const std::unordered_map<std::string, float> quantBits = {
        {"F32", 32.0f}, {"F16", 16.0f}, {"Q8_0", 8.5f}, {"Q8_K_XL", 8.5f}, {"Q6_K", 6.5f}, {"Q6_K_XL", 6.5f}, {"Q5_K_M", 5.5f}, {"Q5_K_S", 5.1f}, {"Q5_K_XL", 5.5f}, {"Q5_0", 5.5f}, {"Q4_K_M", 4.5f}, {"Q4_K_L", 4.6f}, {"Q4_K_S", 4.1f}, {"Q4_K_XL", 4.5f}, {"Q4_0", 4.5f}, {"Q4_1", 4.5f}, {"IQ4_NL", 4.2f}, {"IQ4_XS", 4.0f}, {"Q3_K_L", 3.4f}, {"Q3_K_M", 3.3f}, {"Q3_K_S", 3.2f}, {"Q3_K_XL", 3.4f}, {"IQ3_XXS", 3.1f}, {"Q2_K", 2.6f}, {"Q2_K_L", 2.8f}, {"Q2_K_XL", 2.6f}, {"IQ2_XXS", 2.1f}, {"IQ2_M", 2.4f}, {"IQ1_S", 1.6f}, {"IQ1_M", 1.8f},
        // Unsloth Dynamic variants (similar to base)
        {"UD-Q8_K_XL", 8.5f},
        {"UD-Q6_K_XL", 6.5f},
        {"UD-Q5_K_XL", 5.5f},
        {"UD-Q4_K_XL", 4.5f},
        {"UD-Q3_K_XL", 3.4f},
        {"UD-Q2_K_XL", 2.6f},
        {"UD-IQ3_XXS", 3.1f},
        {"UD-IQ2_XXS", 2.1f},
        {"UD-IQ2_M", 2.4f},
        {"UD-IQ1_S", 1.6f},
        {"UD-IQ1_M", 1.8f}};

    auto it = quantBits.find(quantType);
    if (it != quantBits.end())
    {
        bitsPerParam = it->second;
    }

    // Convert to bytes and then MB
    uint64_t sizeBytes = static_cast<uint64_t>(totalParams * bitsPerParam / 8.0f);
    return static_cast<size_t>(sizeBytes / (1024 * 1024));
}

std::string ModelFileUtils::formatMemorySize(size_t sizeInMB)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);

    // Use decimal conversion (1000) to match file system and browser display
    if (sizeInMB >= 1000)
    {
        double sizeInGB = sizeInMB / 1000.0;
        oss << sizeInGB << " GB";
    }
    else
    {
        oss << sizeInMB << " MB";
    }

    return oss.str();
}

void ModelFileUtils::sortByPriority(std::vector<ModelFile> &modelFiles)
{
    std::sort(modelFiles.begin(), modelFiles.end(), [](const ModelFile &a, const ModelFile &b)
              { return a.quant.priority < b.quant.priority; });
}

size_t ModelFileUtils::getActualFileSizeFromUrl(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return 0;
    }

    // Configure CURL for HEAD request to get file size
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request only
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Perform the HEAD request
    CURLcode res = curl_easy_perform(curl);

    size_t fileSize = 0;
    if (res == CURLE_OK)
    {
        // Get response code
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code == 200)
        {
            // Get content length
            curl_off_t content_length = 0;
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &content_length);

            if (content_length > 0)
            {
                fileSize = static_cast<size_t>(content_length);
            }
        }
    }

    curl_easy_cleanup(curl);
    return fileSize;
}

bool ModelFileUtils::waitForAsyncMemoryCalculations(std::vector<ModelFile>& modelFiles, int timeoutSeconds)
{
    auto startTime = std::chrono::steady_clock::now();
    auto timeoutDuration = std::chrono::seconds(timeoutSeconds);
    
    int totalFiles = 0;
    for (const auto& modelFile : modelFiles)
    {
        if (modelFile.memoryUsage.isLoading)
        {
            totalFiles++;
        }
    }
    
    if (totalFiles == 0)
    {
        return true; // No async calculations in progress
    }
    
    std::cout << "Calculating memory usage for " << totalFiles << " file(s)";
    
    while (true)
    {
        int completedFiles = 0;
        
        // Update and count completed calculations
        updateAllAsyncMemoryUsage(modelFiles);
        
        for (const auto& modelFile : modelFiles)
        {
            if (!modelFile.memoryUsage.isLoading && modelFile.memoryUsage.hasEstimate)
            {
                completedFiles++;
            }
        }
        
        // Show progress
        if (completedFiles < totalFiles)
        {
            std::cout << "\rCalculating memory usage for " << totalFiles << " file(s) [" 
                      << completedFiles << "/" << totalFiles << "]";
            std::cout.flush();
        }
        else
        {
            std::cout << "\rMemory usage calculated for all " << totalFiles << " file(s) ✓" << std::endl;
            return true;
        }
        
        // Check timeout
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime - startTime >= timeoutDuration)
        {
            std::cout << "\rMemory calculation timeout (showing partial results)" << std::endl;
            return false;
        }
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

bool ModelFile::updateDisplayIfReady()
{
    return ModelFileUtils::updateAsyncMemoryUsage(memoryUsage);
}

int ModelFileUtils::displayAsyncModelFileList(std::vector<ModelFile>& modelFiles, const std::string& title)
{
    if (modelFiles.empty()) {
        std::cout << "No model files available." << std::endl;
        return -1;
    }
    
    // Helper function to convert ModelFile to display string
    auto createDisplayString = [](const ModelFile& file) -> std::string {
        // Extract model name from modelId (part after the last '/')
        std::string modelName = file.modelId;
        size_t lastSlash = modelName.find_last_of('/');
        if (lastSlash != std::string::npos) {
            modelName = modelName.substr(lastSlash + 1);
        }
        
        // Convert to lowercase and replace underscores with hyphens
        std::transform(modelName.begin(), modelName.end(), modelName.begin(), [](char c) {
            return (c == '_') ? '-' : std::tolower(c);
        });
        
        // Create base display string: model-name:QUANT_TYPE (TYPE: description)
        std::string result = modelName + ":" + file.quant.type + " (" + file.quant.type + ": " + file.quant.description + ")";
        
        // Add memory information
        if (file.memoryUsage.isLoading) {
            result += " [Memory: calculating...]";
        } else if (file.memoryUsage.hasEstimate) {
            result += " [Memory: " + file.memoryUsage.displayString + "]";
        }
        
        return result;
    };
    
    // Start async memory calculations for all files
    ensureAsyncMemoryCalculations(modelFiles);
    
    // Create initial display items
    std::vector<std::string> displayItems;
    for (const auto& file : modelFiles) {
        displayItems.push_back(createDisplayString(file));
    }
    displayItems.push_back("Back to Model Selection");
    
    // Create interactive list
    InteractiveList list(displayItems);
    
    // Run with periodic updates
    int result = list.runWithUpdates([&]() -> bool {
        // Check if any memory calculations have completed
        bool anyUpdated = updateAllAsyncMemoryUsage(modelFiles);
        
        if (anyUpdated) {
            // Recreate display items with updated memory information
            std::vector<std::string> updatedItems;
            for (const auto& file : modelFiles) {
                updatedItems.push_back(createDisplayString(file));
            }
            updatedItems.push_back("Back to Model Selection");
            
            // Update the list items
            list.updateItems(updatedItems);
            return true; // Signal that an update occurred
        }
        
        return false; // No updates
    });
    
    // Handle the result
    if (result != -1 && result < static_cast<int>(modelFiles.size())) {
        return result; // Selected a model file
    }
    return -1; // Back/Exit selected or cancelled
}

void ModelFileUtils::ensureAsyncMemoryCalculations(std::vector<ModelFile>& modelFiles)
{
    for (auto& modelFile : modelFiles)
    {
        // If the file doesn't have a memory estimate and isn't currently loading, start async calculation
        if (!modelFile.memoryUsage.hasEstimate && !modelFile.memoryUsage.isLoading)
        {
            // Only start calculation if we have a download URL
            if (modelFile.downloadUrl.has_value())
            {
                modelFile.memoryUsage = calculateMemoryUsageAsync(modelFile, 4096);
            }
        }
    }
}

void ModelFileUtils::cacheModelFilesWithMemory(const std::string& modelId, std::vector<ModelFile>& modelFiles)
{
    // Wait for async calculations to complete before caching
    waitForAsyncMemoryCalculations(modelFiles, 30);
    
    // Cache the results with completed memory information
    if (!modelFiles.empty()) {
        CacheManager::cacheModelFiles(modelId, modelFiles);
        std::cout << "✓ Cached " << modelFiles.size() << " model files with memory information" << std::endl;
    }
}
