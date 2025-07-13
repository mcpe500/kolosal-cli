#ifndef MODEL_FILE_H
#define MODEL_FILE_H

#include <string>
#include <vector>
#include <optional>
#include <future>
#include <memory>
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
    bool isLoading = false;         ///< Whether memory calculation is in progress
    std::shared_ptr<std::future<MemoryUsage>> asyncResult; ///< Future for async calculation
};

/**
 * @brief Represents a model file with quantization information
 */
struct ModelFile {
    std::string filename;           ///< Name of the model file
    std::string modelId;            ///< Full model ID (e.g., "kolosal/model-name")
    QuantizationInfo quant;         ///< Quantization information
    std::optional<std::string> downloadUrl; ///< Full download URL if available
    MemoryUsage memoryUsage;        ///< Memory usage estimation
    
    /**
     * @brief Get a formatted display name for the model file
     * @return String containing model ID and quantization info in format "model-name:QUANT_TYPE"
     */
    std::string getDisplayName() const;
    
    /**
     * @brief Get a formatted display name with memory usage
     * @return String containing model ID, quantization info, and memory usage in format "model-name:QUANT_TYPE [Memory: info]"
     */
    std::string getDisplayNameWithMemory() const;
    
    /**
     * @brief Update display string after async memory calculation completes
     * @return true if the display string was updated (calculation completed)
     */
    bool updateDisplayIfReady();
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
     * @brief Start async memory usage calculation for a model file
     * @param modelFile The model file to analyze
     * @param contextSize Context size to use for KV cache calculation (default: 4096)
     * @return MemoryUsage structure with async calculation future
     */
    static MemoryUsage calculateMemoryUsageAsync(const ModelFile& modelFile, int contextSize = 4096);
    
    /**
     * @brief Update memory usage if async calculation is complete
     * @param memoryUsage The memory usage structure to update
     * @return true if the calculation was completed and updated, false if still in progress
     */
    static bool updateAsyncMemoryUsage(MemoryUsage& memoryUsage);
    
    /**
     * @brief Update memory usage for all model files with pending async calculations
     * @param modelFiles Vector of ModelFile objects to update
     * @return true if any calculations were completed, false if all are still pending
     */
    static bool updateAllAsyncMemoryUsage(std::vector<ModelFile>& modelFiles);
    
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
    
    /**
     * @brief Wait for async memory calculations to complete with timeout and progress indication
     * @param modelFiles Vector of ModelFile objects to wait for
     * @param timeoutSeconds Maximum time to wait in seconds (default: 30)
     * @return true if all calculations completed, false if timeout reached
     */
    static bool waitForAsyncMemoryCalculations(std::vector<ModelFile>& modelFiles, int timeoutSeconds = 30);
    
    /**
     * @brief Display model files with real-time async memory updates
     * @param modelFiles Vector of ModelFile objects to display and update
     * @param title Title to show above the list
     * @return Index of selected file, or -1 if cancelled
     */
    static int displayAsyncModelFileList(std::vector<ModelFile>& modelFiles, const std::string& title);
    
    /**
     * @brief Display model files with real-time async memory updates (with header info)
     * @param modelFiles Vector of ModelFile objects to display and update
     * @param title Title to show above the list
     * @param headerInfo Additional header information to display
     * @return Index of selected file, or -1 if cancelled
     */
    static int displayAsyncModelFileList(std::vector<ModelFile>& modelFiles, const std::string& title, const std::string& headerInfo);
    
    /**
     * @brief Ensure model files have async memory calculations running if needed
     * @param modelFiles Vector of ModelFile objects to check and start calculations for
     */
    static void ensureAsyncMemoryCalculations(std::vector<ModelFile>& modelFiles);
    
    /**
     * @brief Cache model files after ensuring async memory calculations are complete
     * @param modelId The model ID to use as cache key
     * @param modelFiles Vector of ModelFile objects to cache
     */
    static void cacheModelFilesWithMemory(const std::string& modelId, std::vector<ModelFile>& modelFiles);
};

#endif // MODEL_FILE_H
