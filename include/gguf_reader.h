#ifndef GGUF_READER_H
#define GGUF_READER_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <memory>
#include <curl/curl.h>
#include <sstream>
#include <cstring>
#include <algorithm>

// Structure to hold the extracted model parameters
struct GGUFModelParams {
    uint64_t hidden_size = 0;       // Mapped from embedding_length
    uint32_t attention_heads = 0;   // Mapped from attention.head_count
    uint32_t hidden_layers = 0;     // Mapped from block_count
    uint32_t kv_heads = 0;          // Mapped from attention.head_count_kv or head_count
};

// Abstract base class for data sources
class DataSource {
public:
    virtual ~DataSource() = default;
    virtual bool read(char* buffer, size_t size) = 0;
    virtual bool seek(size_t position) = 0;
    virtual bool eof() const = 0;
    virtual size_t tell() = 0;
};

// File-based data source
class FileDataSource : public DataSource {
public:
    FileDataSource(const std::string& filename);
    ~FileDataSource() override;

    bool read(char* buffer, size_t size) override;
    bool seek(size_t position) override;
    bool eof() const override;
    size_t tell() override;

private:
    std::ifstream file;
};

// CURL callback data structure
struct CurlBuffer {
    char* buffer;
    size_t size;
    size_t pos;
    bool* abort_download;
};

// CURL-based URL data source
class UrlDataSource : public DataSource {
public:
    UrlDataSource(const std::string& url);
    ~UrlDataSource() override;

    bool read(char* buffer, size_t size) override;
    bool seek(size_t position) override;
    bool eof() const override;
    size_t tell() override;
    void setAbortFlag();

private:
    static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static int ProgressCallback(void* clientp, curl_off_t, curl_off_t dlnow, curl_off_t, curl_off_t);

    std::string url;
    CURL* curl;
    CurlBuffer writeData;
    std::vector<char> downloadedData;
    size_t bufferSize;
    size_t bufferPos;
    size_t currentPos;
    bool abortDownload;
    bool _eof = false;

    static constexpr size_t BUFFER_SIZE = 1024 * 1024;  // 1MB buffer
    static constexpr size_t CHUNK_SIZE = 256 * 1024;      // 256KB chunk size
};

class GGUFMetadataReader {
public:
    // GGUF metadata types
    enum class GGUFType : uint32_t {
        UINT8 = 0,
        INT8 = 1,
        UINT16 = 2,
        INT16 = 3,
        UINT32 = 4,
        INT32 = 5,
        FLOAT32 = 6,
        BOOL = 7,
        STRING = 8,
        ARRAY = 9,
        UINT64 = 10,
        INT64 = 11,
        FLOAT64 = 12,
        MAX_TYPE = 13
    };

    GGUFMetadataReader();
    ~GGUFMetadataReader();

    bool isUrl(const std::string& path);
    std::optional<GGUFModelParams> readModelParams(const std::string& path, bool verbose = false);

private:
    bool endsWith(const std::string& str, const std::string& suffix);
    std::string readString(DataSource* source);
    void skipArray(DataSource* source, GGUFType elemType);
    void skipValue(DataSource* source, GGUFType type);
};

#endif // GGUF_READER_H
