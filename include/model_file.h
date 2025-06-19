#ifndef MODEL_FILE_H
#define MODEL_FILE_H

#include <string>
#include <vector>

/**
 * @brief Information about quantization type and quality
 */
struct QuantizationInfo {
    std::string type;           ///< Quantization type (e.g., "Q8_0", "Q4_K_M")
    std::string description;    ///< Human-readable description
    int priority;               ///< Priority for default selection (lower = higher priority)
};

/**
 * @brief Represents a model file with quantization information
 */
struct ModelFile {
    std::string filename;       ///< Name of the model file
    QuantizationInfo quant;     ///< Quantization information
    
    /**
     * @brief Get a formatted display name for the model file
     * @return String containing filename and quantization info
     */
    std::string getDisplayName() const;
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
};

#endif // MODEL_FILE_H
