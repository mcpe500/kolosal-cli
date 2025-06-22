#include "model_file.h"
#include <algorithm>

std::string ModelFile::getDisplayName() const {
    return filename + " (" + quant.type + ": " + quant.description + ")";
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

void ModelFileUtils::sortByPriority(std::vector<ModelFile>& modelFiles) {
    std::sort(modelFiles.begin(), modelFiles.end(), [](const ModelFile& a, const ModelFile& b) {
        return a.quant.priority < b.quant.priority;
    });
}
