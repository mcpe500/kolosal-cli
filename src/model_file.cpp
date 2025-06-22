#include "model_file.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>

std::string ModelFile::getDisplayName() const {
    return filename + " (" + quant.type + ": " + quant.description + ")";
}

std::string ModelFile::getDisplayNameWithMemory() const {
    // Format: "filename (QUANT_TYPE: description) [Memory: total (breakdown)]"
    std::string result = filename + " (" + quant.type + ": " + quant.description + ")";
    if (memoryUsage.hasEstimate) {
        result += " [Memory: " + memoryUsage.displayString + "]";
    }
    return result;
}

QuantizationInfo ModelFileUtils::detectQuantization(const std::string& filename) {
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
      // Check for various quantization patterns (order matters - check most specific first)
    
    // Unsloth Dynamic (UD) variants - selective parameter quantization
    if (lower_filename.find("iq1_s") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-IQ1_S", "1-bit Unsloth Dynamic quantization (small), selective parameter quantization", 1};
    } else if (lower_filename.find("iq1_m") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-IQ1_M", "1-bit Unsloth Dynamic quantization (medium), selective parameter quantization", 2};
    } else if (lower_filename.find("iq2_xxs") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-IQ2_XXS", "2-bit Unsloth Dynamic quantization (extra extra small), selective parameter quantization", 3};
    } else if (lower_filename.find("iq2_m") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-IQ2_M", "2-bit Unsloth Dynamic quantization (medium), selective parameter quantization", 4};
    } else if (lower_filename.find("iq3_xxs") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-IQ3_XXS", "3-bit Unsloth Dynamic quantization (extra extra small), selective parameter quantization", 5};    } else if (lower_filename.find("q2_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q2_K_XL", "2-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 6};
    } else if (lower_filename.find("q3_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q3_K_XL", "3-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 7};
    } else if (lower_filename.find("q4_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q4_K_XL", "4-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 8};
    } else if (lower_filename.find("q5_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q5_K_XL", "5-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 9};
    } else if (lower_filename.find("q6_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q6_K_XL", "6-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 10};
    } else if (lower_filename.find("q8_k_xl") != std::string::npos && lower_filename.find("ud-") != std::string::npos) {
        return {"UD-Q8_K_XL", "8-bit Unsloth Dynamic K-quantization (XL), selective parameter quantization", 11};
    
    // Standard XL variants
    } else if (lower_filename.find("q8_k_xl") != std::string::npos) {
        return {"Q8_K_XL", "8-bit K-quantization (XL), maximum quality", 12};
    } else if (lower_filename.find("q6_k_xl") != std::string::npos) {
        return {"Q6_K_XL", "6-bit K-quantization (XL), very high quality", 13};
    } else if (lower_filename.find("q5_k_xl") != std::string::npos) {
        return {"Q5_K_XL", "5-bit K-quantization (XL), high quality", 14};
    } else if (lower_filename.find("q4_k_xl") != std::string::npos) {
        return {"Q4_K_XL", "4-bit K-quantization (XL), good quality", 15};
    } else if (lower_filename.find("q3_k_xl") != std::string::npos) {
        return {"Q3_K_XL", "3-bit K-quantization (XL), compact with quality", 16};
    } else if (lower_filename.find("q2_k_xl") != std::string::npos) {
        return {"Q2_K_XL", "2-bit K-quantization (XL), very compact", 17};
    // Standard 8-bit variants
    } else if (lower_filename.find("q8_0") != std::string::npos) {
        return {"Q8_0", "8-bit quantization, excellent quality", 18};
    
    // 6-bit variants
    } else if (lower_filename.find("q6_k") != std::string::npos) {
        return {"Q6_K", "6-bit quantization, high quality with smaller size", 19};
    
    // 5-bit variants
    } else if (lower_filename.find("q5_k_m") != std::string::npos) {
        return {"Q5_K_M", "5-bit quantization (medium), good quality/size balance", 20};
    } else if (lower_filename.find("q5_k_s") != std::string::npos) {
        return {"Q5_K_S", "5-bit quantization (small), smaller size", 21};
    } else if (lower_filename.find("q5_0") != std::string::npos) {
        return {"Q5_0", "5-bit quantization, legacy format", 22};
    
    // 4-bit variants
    } else if (lower_filename.find("iq4_nl") != std::string::npos) {
        return {"IQ4_NL", "4-bit improved quantization (no lookup), very efficient", 23};
    } else if (lower_filename.find("iq4_xs") != std::string::npos) {
        return {"IQ4_XS", "4-bit improved quantization (extra small), ultra compact", 24};
    } else if (lower_filename.find("q4_k_m") != std::string::npos) {
        return {"Q4_K_M", "4-bit quantization (medium), good for most use cases", 25};
    } else if (lower_filename.find("q4_k_s") != std::string::npos) {
        return {"Q4_K_S", "4-bit quantization (small), very compact", 26};
    } else if (lower_filename.find("q4_1") != std::string::npos) {
        return {"Q4_1", "4-bit quantization v1, improved legacy format", 27};
    } else if (lower_filename.find("q4_0") != std::string::npos) {
        return {"Q4_0", "4-bit quantization, legacy format", 28};
    
    // 3-bit variants
    } else if (lower_filename.find("iq3_xxs") != std::string::npos) {
        return {"IQ3_XXS", "3-bit improved quantization (extra extra small), maximum compression", 29};
    } else if (lower_filename.find("q3_k_l") != std::string::npos) {
        return {"Q3_K_L", "3-bit quantization (large), experimental", 30};
    } else if (lower_filename.find("q3_k_m") != std::string::npos) {
        return {"Q3_K_M", "3-bit quantization (medium), very small size", 31};
    } else if (lower_filename.find("q3_k_s") != std::string::npos) {
        return {"Q3_K_S", "3-bit quantization (small), ultra compact", 32};
    
    // 2-bit variants
    } else if (lower_filename.find("iq2_xxs") != std::string::npos) {
        return {"IQ2_XXS", "2-bit improved quantization (extra extra small), extreme compression", 33};
    } else if (lower_filename.find("iq2_m") != std::string::npos) {
        return {"IQ2_M", "2-bit improved quantization (medium), balanced compression", 34};
    } else if (lower_filename.find("q2_k_l") != std::string::npos) {
        return {"Q2_K_L", "2-bit quantization (large), better quality at 2-bit", 35};
    } else if (lower_filename.find("q2_k") != std::string::npos) {
        return {"Q2_K", "2-bit quantization, extremely small but lower quality", 36};
    
    // 1-bit variants (experimental)
    } else if (lower_filename.find("iq1_s") != std::string::npos) {
        return {"IQ1_S", "1-bit improved quantization (small), experimental ultra compression", 37};
    } else if (lower_filename.find("iq1_m") != std::string::npos) {
        return {"IQ1_M", "1-bit improved quantization (medium), experimental compression", 38};
    
    // Floating point variants
    } else if (lower_filename.find("f16") != std::string::npos) {
        return {"F16", "16-bit floating point, highest quality but large size", 39};
    } else if (lower_filename.find("f32") != std::string::npos) {
        return {"F32", "32-bit floating point, original precision", 40};
    
    // Default
    } else {
        return {"Unknown", "Unknown quantization type", 41};
    }
}

MemoryUsage ModelFileUtils::calculateMemoryUsage(const ModelFile& modelFile, int contextSize) {
    MemoryUsage usage;
    
    // Try to read GGUF metadata if we have a download URL
    if (modelFile.downloadUrl.has_value()) {
        try {
            GGUFMetadataReader reader;
            auto params = reader.readModelParams(modelFile.downloadUrl.value(), false);
            
            if (params.has_value()) {
                // Calculate model size based on quantization and parameters
                usage.modelSizeMB = estimateModelSize(params.value(), modelFile.quant.type);
                
                // Calculate KV cache size
                // Formula: 4 * hidden_size * hidden_layers * context_size / (1024 * 1024)
                float kvCacheSizeBytes = 4.0f * 
                    params->hidden_size * 
                    params->hidden_layers * 
                    contextSize;
                usage.kvCacheMB = static_cast<size_t>(kvCacheSizeBytes / (1024 * 1024));
                
                // Total required memory = model size * 1.2 (20% overhead) + KV cache
                usage.totalRequiredMB = static_cast<size_t>(usage.modelSizeMB * 1.2) + usage.kvCacheMB;
                
                // Create display string
                std::ostringstream oss;
                oss << formatMemorySize(usage.totalRequiredMB) 
                    << " (Model: " << formatMemorySize(usage.modelSizeMB) 
                    << " + KV: " << formatMemorySize(usage.kvCacheMB) << ")";
                usage.displayString = oss.str();
                usage.hasEstimate = true;
            }        } catch (const std::exception&) {
            // If GGUF reading fails, fall back to rough estimation
            usage = estimateMemoryFromFilename(modelFile.filename, modelFile.quant.type, contextSize);
        }
    } else {
        // No URL available, use filename-based estimation
        usage = estimateMemoryFromFilename(modelFile.filename, modelFile.quant.type, contextSize);
    }
    
    return usage;
}

size_t ModelFileUtils::estimateModelSize(const GGUFModelParams& params, const std::string& quantType) {
    // Calculate approximate model size based on parameters and quantization
    
    // Base calculation: hidden_size * hidden_layers * attention_heads
    uint64_t totalParams = static_cast<uint64_t>(params.hidden_size) * 
                          params.hidden_layers * 
                          params.attention_heads * 1000; // Rough multiplier
    
    // Bits per parameter based on quantization type
    float bitsPerParam = 16.0f; // Default F16
    
    // Map quantization types to bits per parameter
    static const std::unordered_map<std::string, float> quantBits = {
        {"F32", 32.0f}, {"F16", 16.0f},
        {"Q8_0", 8.5f}, {"Q8_K_XL", 8.5f},
        {"Q6_K", 6.5f}, {"Q6_K_XL", 6.5f},
        {"Q5_K_M", 5.5f}, {"Q5_K_S", 5.1f}, {"Q5_K_XL", 5.5f}, {"Q5_0", 5.5f},
        {"Q4_K_M", 4.5f}, {"Q4_K_S", 4.1f}, {"Q4_K_XL", 4.5f}, {"Q4_0", 4.5f}, {"Q4_1", 4.5f},
        {"IQ4_NL", 4.2f}, {"IQ4_XS", 4.0f},
        {"Q3_K_L", 3.4f}, {"Q3_K_M", 3.3f}, {"Q3_K_S", 3.2f}, {"Q3_K_XL", 3.4f},
        {"IQ3_XXS", 3.1f},
        {"Q2_K", 2.6f}, {"Q2_K_L", 2.8f}, {"Q2_K_XL", 2.6f},
        {"IQ2_XXS", 2.1f}, {"IQ2_M", 2.4f},
        {"IQ1_S", 1.6f}, {"IQ1_M", 1.8f},
        // Unsloth Dynamic variants (similar to base)
        {"UD-Q8_K_XL", 8.5f}, {"UD-Q6_K_XL", 6.5f}, {"UD-Q5_K_XL", 5.5f},
        {"UD-Q4_K_XL", 4.5f}, {"UD-Q3_K_XL", 3.4f}, {"UD-Q2_K_XL", 2.6f},
        {"UD-IQ3_XXS", 3.1f}, {"UD-IQ2_XXS", 2.1f}, {"UD-IQ2_M", 2.4f},
        {"UD-IQ1_S", 1.6f}, {"UD-IQ1_M", 1.8f}
    };
    
    auto it = quantBits.find(quantType);
    if (it != quantBits.end()) {
        bitsPerParam = it->second;
    }
    
    // Convert to bytes and then MB
    uint64_t sizeBytes = static_cast<uint64_t>(totalParams * bitsPerParam / 8.0f);
    return static_cast<size_t>(sizeBytes / (1024 * 1024));
}

MemoryUsage ModelFileUtils::estimateMemoryFromFilename(const std::string& filename, const std::string& quantType, int contextSize) {
    MemoryUsage usage;
    
    // Rough estimation based on typical model sizes and quantization
    // This is a fallback when GGUF metadata is not available
    
    // Estimate base model size from quantization type (very rough)
    size_t estimatedModelSizeMB = 4000; // Default for medium model
    
    // Adjust based on quantization type
    static const std::unordered_map<std::string, float> sizeMultipliers = {
        {"F32", 2.0f}, {"F16", 1.0f},
        {"Q8_0", 0.6f}, {"Q8_K_XL", 0.6f},
        {"Q6_K", 0.45f}, {"Q6_K_XL", 0.45f},
        {"Q5_K_M", 0.38f}, {"Q5_K_S", 0.35f}, {"Q5_K_XL", 0.38f}, {"Q5_0", 0.38f},
        {"Q4_K_M", 0.30f}, {"Q4_K_S", 0.28f}, {"Q4_K_XL", 0.30f}, {"Q4_0", 0.30f}, {"Q4_1", 0.30f},
        {"IQ4_NL", 0.29f}, {"IQ4_XS", 0.27f},
        {"Q3_K_L", 0.23f}, {"Q3_K_M", 0.22f}, {"Q3_K_S", 0.21f}, {"Q3_K_XL", 0.23f},
        {"IQ3_XXS", 0.20f},
        {"Q2_K", 0.18f}, {"Q2_K_L", 0.19f}, {"Q2_K_XL", 0.18f},
        {"IQ2_XXS", 0.15f}, {"IQ2_M", 0.16f},
        {"IQ1_S", 0.12f}, {"IQ1_M", 0.13f}
    };
    
    auto it = sizeMultipliers.find(quantType);
    if (it != sizeMultipliers.end()) {
        estimatedModelSizeMB = static_cast<size_t>(estimatedModelSizeMB * it->second);
    }
    
    usage.modelSizeMB = estimatedModelSizeMB;
    
    // Rough KV cache estimation (assuming typical 7B model parameters)
    // For context size 4096: approximately 1-2GB for 7B model
    float kvCacheRatio = contextSize / 4096.0f;
    usage.kvCacheMB = static_cast<size_t>(1500 * kvCacheRatio); // Base 1.5GB for 4K context
    
    // Total memory
    usage.totalRequiredMB = static_cast<size_t>(usage.modelSizeMB * 1.2) + usage.kvCacheMB;
    
    // Create display string
    std::ostringstream oss;
    oss << formatMemorySize(usage.totalRequiredMB) 
        << " (Est. Model: " << formatMemorySize(usage.modelSizeMB) 
        << " + KV: " << formatMemorySize(usage.kvCacheMB) << ")";
    usage.displayString = oss.str();
    usage.hasEstimate = true;
    
    return usage;
}

std::string ModelFileUtils::formatMemorySize(size_t sizeInMB) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    
    if (sizeInMB >= 1024) {
        double sizeInGB = sizeInMB / 1024.0;
        oss << sizeInGB << " GB";
    } else {
        oss << sizeInMB << " MB";
    }
    
    return oss.str();
}

void ModelFileUtils::sortByPriority(std::vector<ModelFile>& modelFiles) {
    std::sort(modelFiles.begin(), modelFiles.end(), [](const ModelFile& a, const ModelFile& b) {
        return a.quant.priority < b.quant.priority;
    });
}
