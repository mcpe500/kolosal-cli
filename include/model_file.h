#ifndef MODEL_FILE_H
#define MODEL_FILE_H

#include <string>
#include <vector>
#include <optional>
#include "gguf_reader.h"

/**
 * @brief Information about quantization type and quality
 */
struct QuantizationInfo {
    std::string type;           ///< Quantization type (e.g., "Q8_0", "Q4_K_M")
    std::string description;    ///< Human-readable description
    int priority;               ///< Priority for default selection (lower = higher priority)
};

/**
 * @brief Memory usage estimation for a model
 */
struct MemoryUsage {
    size_t modelSizeMB = 0;         ///< Model size in MB
    size_t kvCacheMB = 0;           ///< KV cache size in MB
    size_t totalRequiredMB = 0;     ///< Total required memory in MB
    std::string displayString;     ///< Formatted display string
    bool hasEstimate = false;       ///< Whether we have valid estimates
};

/**
 * @brief Represents a model file with quantization information
 */
struct ModelFile {
    std::string filename;           ///< Name of the model file
    QuantizationInfo quant;         ///< Quantization information
    std::optional<std::string> downloadUrl; ///< Full download URL if available
    MemoryUsage memoryUsage;        ///< Memory usage estimation
    
    /**
     * @brief Get a formatted display name for the model file
     * @return String containing filename and quantization info
     */
    std::string getDisplayName() const;
    
    /**
     * @brief Get a formatted display name with memory usage
     * @return String containing filename, quantization info, and memory usage
     */
    std::string getDisplayNameWithMemory() const;
};

/**
 * @brief Utility class for model file operations
 */
class ModelFileUtils {
public:
    /**
     * @brief Detect quantization type from filename
     * @param filename The filename to analyze
     * @return QuantizationInfo structure with detected information
     */
    static QuantizationInfo detectQuantization(const std::string& filename);
    
    /**
     * @brief Sort model files by quantization priority
     * @param modelFiles Vector of ModelFile objects to sort
     */
    static void sortByPriority(std::vector<ModelFile>& modelFiles);
    
    /**
     * @brief Calculate memory usage estimation for a model file
     * @param modelFile The model file to analyze
     * @param contextSize Context size to use for KV cache calculation (default: 4096)
     * @return MemoryUsage structure with estimated memory requirements
     */
    static MemoryUsage calculateMemoryUsage(const ModelFile& modelFile, int contextSize = 4096);
    
    /**
     * @brief Estimate model file size based on quantization type and model parameters
     * @param params Model parameters from GGUF metadata
     * @param quantType Quantization type
     * @return Estimated model size in MB
     */
    static size_t estimateModelSize(const GGUFModelParams& params, const std::string& quantType);    /**
     * @brief Format memory size in human-readable format
     * @param sizeInMB Size in megabytes
     * @return Formatted string (e.g., "2.4 GB", "512 MB")
     */
    static std::string formatMemorySize(size_t sizeInMB);
    
    /**
     * @brief Get actual file size from URL using HTTP HEAD request
     * @param url The URL to check
     * @return File size in bytes, or 0 if unable to determine
     */
    static size_t getActualFileSizeFromUrl(const std::string& url);
};

#endif // MODEL_FILE_H
