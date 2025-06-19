#include "model_file.h"
#include <algorithm>

std::string ModelFile::getDisplayName() const {
    return filename + " (" + quant.type + ": " + quant.description + ")";
}

QuantizationInfo ModelFileUtils::detectQuantization(const std::string& filename) {
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    // Check for various quantization patterns
    if (lower_filename.find("q8_0") != std::string::npos) {
        return {"Q8_0", "8-bit quantization, good balance of quality and size", 1};
    } else if (lower_filename.find("q6_k") != std::string::npos) {
        return {"Q6_K", "6-bit quantization, high quality with smaller size", 2};
    } else if (lower_filename.find("q5_k_m") != std::string::npos) {
        return {"Q5_K_M", "5-bit quantization (medium), good quality/size balance", 3};
    } else if (lower_filename.find("q5_k_s") != std::string::npos) {
        return {"Q5_K_S", "5-bit quantization (small), smaller size", 4};
    } else if (lower_filename.find("q5_0") != std::string::npos) {
        return {"Q5_0", "5-bit quantization, legacy format", 5};
    } else if (lower_filename.find("q4_k_m") != std::string::npos) {
        return {"Q4_K_M", "4-bit quantization (medium), good for most use cases", 6};
    } else if (lower_filename.find("q4_k_s") != std::string::npos) {
        return {"Q4_K_S", "4-bit quantization (small), very compact", 7};
    } else if (lower_filename.find("q4_0") != std::string::npos) {
        return {"Q4_0", "4-bit quantization, legacy format", 8};
    } else if (lower_filename.find("q3_k_l") != std::string::npos) {
        return {"Q3_K_L", "3-bit quantization (large), experimental", 9};
    } else if (lower_filename.find("q3_k_m") != std::string::npos) {
        return {"Q3_K_M", "3-bit quantization (medium), very small size", 10};
    } else if (lower_filename.find("q3_k_s") != std::string::npos) {
        return {"Q3_K_S", "3-bit quantization (small), ultra compact", 11};
    } else if (lower_filename.find("q2_k") != std::string::npos) {
        return {"Q2_K", "2-bit quantization, extremely small but lower quality", 12};
    } else if (lower_filename.find("f16") != std::string::npos) {
        return {"F16", "16-bit floating point, highest quality but large size", 13};
    } else if (lower_filename.find("f32") != std::string::npos) {
        return {"F32", "32-bit floating point, original precision", 14};
    } else {
        return {"Unknown", "Unknown quantization type", 15};
    }
}

void ModelFileUtils::sortByPriority(std::vector<ModelFile>& modelFiles) {
    std::sort(modelFiles.begin(), modelFiles.end(), [](const ModelFile& a, const ModelFile& b) {
        return a.quant.priority < b.quant.priority;
    });
}
