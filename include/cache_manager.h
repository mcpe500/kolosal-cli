#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <string>
#include <map>
#include <chrono>
#include <vector>
#include "model_file.h"

/**
 * @brief Structure to hold cached data with timestamp
 */
struct CacheEntry {
    std::string data;
    std::chrono::system_clock::time_point timestamp;
    bool isValid(int ttlSeconds) const;
};

/**
 * @brief Cache manager for storing API responses and reducing network requests
 */
class CacheManager {
public:
    /**
     * @brief Initialize cache manager and load existing cache from disk
     */
    static void initialize();
    
    /**
     * @brief Save cache to disk and cleanup
     */
    static void cleanup();
    
    /**
     * @brief Get cached models if available and not expired
     * @return Vector of model IDs, empty if cache miss or expired
     */
    static std::vector<std::string> getCachedModels();
    
    /**
     * @brief Cache models for future use
     * @param models Vector of model IDs to cache
     */
    static void cacheModels(const std::vector<std::string>& models);
    
    /**
     * @brief Get cached model files if available and not expired
     * @param modelId The model ID to get cached files for
     * @return Vector of ModelFile objects, empty if cache miss or expired
     */
    static std::vector<ModelFile> getCachedModelFiles(const std::string& modelId);
    
    /**
     * @brief Cache model files for future use
     * @param modelId The model ID these files belong to
     * @param files Vector of ModelFile objects to cache
     */
    static void cacheModelFiles(const std::string& modelId, const std::vector<ModelFile>& files);
      /**
     * @brief Clear all cached data
     */
    static void clearCache();
    
    /**
     * @brief Get cached models for offline use (ignores TTL)
     * @return Vector of model IDs from cache regardless of expiration
     */
    static std::vector<std::string> getCachedModelsOffline();
      /**
     * @brief Get cached model files for offline use (ignores TTL)
     * @param modelId The model ID to get cached files for
     * @return Vector of ModelFile objects from cache regardless of expiration
     */
    static std::vector<ModelFile> getCachedModelFilesOffline(const std::string& modelId);
    
    /**
     * @brief Check if any cached data is available (for offline capability info)
     * @return true if any cached data exists, false otherwise
     */
    static bool hasAnyCachedData();
    
    /**
     * @brief Check if cache directory exists and create if needed
     */
    static void ensureCacheDirectory();

private:
    static std::map<std::string, CacheEntry> memoryCache;
    static std::string cacheDirectory;
    static const int DEFAULT_TTL_SECONDS;
    static const int MODEL_FILES_TTL_SECONDS;
    
    /**
     * @brief Get cache file path for a specific key
     * @param key The cache key
     * @return Full path to cache file
     */
    static std::string getCacheFilePath(const std::string& key);
    
    /**
     * @brief Load cache entry from disk
     * @param key The cache key
     * @return CacheEntry object, empty if not found
     */
    static CacheEntry loadFromDisk(const std::string& key);
    
    /**
     * @brief Save cache entry to disk
     * @param key The cache key
     * @param entry The cache entry to save
     */
    static void saveToDisk(const std::string& key, const CacheEntry& entry);
    
    /**
     * @brief Convert vector of strings to JSON string
     * @param items Vector of strings to convert
     * @return JSON string representation
     */
    static std::string vectorToJson(const std::vector<std::string>& items);
    
    /**
     * @brief Convert JSON string to vector of strings
     * @param jsonStr JSON string to convert
     * @return Vector of strings
     */
    static std::vector<std::string> jsonToVector(const std::string& jsonStr);
    
    /**
     * @brief Convert vector of ModelFile to JSON string
     * @param files Vector of ModelFile objects to convert
     * @return JSON string representation
     */
    static std::string modelFilesToJson(const std::vector<ModelFile>& files);
    
    /**
     * @brief Convert JSON string to vector of ModelFile
     * @param jsonStr JSON string to convert
     * @return Vector of ModelFile objects
     */
    static std::vector<ModelFile> jsonToModelFiles(const std::string& jsonStr);
};

#endif // CACHE_MANAGER_H
