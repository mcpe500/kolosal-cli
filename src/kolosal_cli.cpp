#include "kolosal_cli.h"
#include "http_client.h"
#include "hugging_face_client.h"
#include "interactive_list.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include "kolosal_server_client.h"
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <sstream>
#include <signal.h>
#include <csignal>
#include <thread>
#include <fstream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

// Static member initialization
KolosalCLI *KolosalCLI::s_instance = nullptr;

// Helper function to format file sizes
static std::string formatFileSize(long long bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4)
    {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    if (unitIndex == 0)
    {
        oss << static_cast<long long>(size) << " " << units[unitIndex];
    }
    else
    {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    }
    return oss.str();
}

void KolosalCLI::ensureConsoleEncoding()
{
    // Set console to UTF-8 mode for proper character display
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void KolosalCLI::initialize()
{
    ensureConsoleEncoding();

    HttpClient::initialize();
    CacheManager::initialize();

    // Initialize server client
    m_serverClient = std::make_unique<KolosalServerClient>();

    // Set up signal handling for graceful shutdown
    s_instance = this;
    signal(SIGINT, signalHandler);  // Ctrl+C
    signal(SIGTERM, signalHandler); // Termination request
#ifdef _WIN32
    // On Windows, also handle console control events
    SetConsoleCtrlHandler([](DWORD dwCtrlType) -> BOOL
                          {
        if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
            if (KolosalCLI::s_instance) {
                std::cout << "\nReceived shutdown signal. Cancelling downloads..." << std::endl;
                KolosalCLI::s_instance->cancelActiveDownloads();
            }
            return TRUE;
        }
        return FALSE; }, TRUE);
#endif
}

void KolosalCLI::cleanup()
{
    // Cancel any active downloads before cleanup
    cancelActiveDownloads();

    // Reset signal instance before destroying server client
    s_instance = nullptr;

    // Cleanup server client (this will properly close handles)
    if (m_serverClient)
    {
        m_serverClient.reset();
    }

    // Cleanup other components
    CacheManager::cleanup();
    HttpClient::cleanup();
}

bool KolosalCLI::stopBackgroundServer()
{
    if (!m_serverClient)
    {
        std::cerr << "Server client not initialized." << std::endl;
        return false;
    }

    std::cout << "Stopping background Kolosal server..." << std::endl;

    // Try to stop the server
    if (m_serverClient->shutdownServer())
    {
        return true;
    }

    std::cerr << "Failed to stop server." << std::endl;
    return false;
}

void KolosalCLI::showWelcome()
{
    std::cout << R"(
       ██     ██   ██   ███████   ██         ███████    ████████     ██     ██
     ██░     ░██  ██   ██░░░░░██ ░██        ██░░░░░██  ██░░░░░░     ████   ░██
   ██░       ░██ ██   ██     ░░██░██       ██     ░░██░██          ██░░██  ░██
 ██░         ░████   ░██      ░██░██      ░██      ░██░█████████  ██  ░░██ ░██
░░ ██        ░██░██  ░██      ░██░██      ░██      ░██░░░░░░░░██ ██████████░██
  ░░ ██      ░██░░██ ░░██     ██ ░██      ░░██     ██        ░██░██░░░░░░██░██
    ░░ ██    ░██ ░░██ ░░███████  ░████████ ░░███████   ████████ ░██     ░██░████████
      ░░     ░░   ░░   ░░░░░░░   ░░░░░░░░   ░░░░░░░   ░░░░░░░░  ░░      ░░ ░░░░░░░░

)" << std::endl;
}

std::vector<std::string> KolosalCLI::generateSampleModels()
{
    return {
        "kolosal/sample-model-1",
        "kolosal/sample-model-2",
        "kolosal/sample-model-3"};
}

std::vector<ModelFile> KolosalCLI::generateSampleFiles(const std::string &modelId)
{
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

bool KolosalCLI::initializeServer()
{
    if (!m_serverClient)
    {
        std::cerr << "Server client not initialized." << std::endl;
        return false;
    }

    // Check if server is already running
    if (m_serverClient->isServerHealthy())
    {
        std::cout << "Kolosal server is already running." << std::endl;
        return true;
    }

    // Try to start the server
    if (!m_serverClient->startServer())
    {
        std::cerr << "Failed to start server." << std::endl;
        return false;
    }

    // Wait for server to become ready
    if (!m_serverClient->waitForServerReady(30))
    {
        std::cerr << "Server failed to become ready within 30 seconds." << std::endl;
        return false;
    }

    return true;
}

bool KolosalCLI::processModelDownload(const std::string &modelId, const ModelFile &modelFile)
{
    // Ensure server is running and can be connected to
    if (!ensureServerConnection())
    {
        std::cerr << "Unable to connect to Kolosal server. Download cancelled." << std::endl;
        return false;
    }

    // Generate download URL
    std::string downloadUrl = "https://huggingface.co/" + modelId + "/resolve/main/" + modelFile.filename; // Generate simplified engine ID in format "model-name:quant"
    std::string modelName = modelId.substr(modelId.find('/') + 1);                                         // Extract model name after "/"
    std::string quantType = modelFile.quant.type;                                                          // Use full quantization type

    std::string engineId = modelName + ":" + quantType;
    std::cout << "\nProcessing: " << modelFile.filename << std::endl;

    // Check if engine already exists before attempting download
    if (m_serverClient->engineExists(engineId))
    {
        std::cout << "✓ Engine '" << engineId << "' already exists on the server." << std::endl;
        std::cout << "✓ Model is ready to use!" << std::endl;
        return true;
    }

    // Send engine creation request to server
    if (!m_serverClient->addEngine(engineId, downloadUrl, "./models/" + modelFile.filename))
    {
        std::cerr << "Failed to send download request." << std::endl;
        return false;
    }

    // Track this download
    m_activeDownloads.push_back(engineId);

    // Monitor download progress
    bool downloadSuccess = m_serverClient->monitorDownloadProgress(
        engineId,
        [this](double percentage, const std::string &status, long long downloadedBytes, long long totalBytes)
        {
            // Progress callback - update display
            ensureConsoleEncoding(); // Ensure proper encoding before each update

            // Handle special case where no download was needed
            if (status == "not_found")
            {
                std::cout << "✓ Model file already exists locally. Registering engine..." << std::endl;
                return;
            }

            // Only show progress for actual downloads
            if (status == "downloading" && totalBytes > 0)
            {
                std::cout << "\r";
                // Create progress bar
                int barWidth = 40;
                int pos = static_cast<int>(barWidth * percentage / 100.0);

                std::cout << "[";
                for (int i = 0; i < barWidth; ++i)
                {
                    if (i < pos)
                        std::cout << "█";
                    else
                        std::cout << "-";
                }
                std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "%";
                
                // Show file sizes
                std::cout << " (" << formatFileSize(downloadedBytes) << "/" << formatFileSize(totalBytes) << ")";
                std::cout.flush();
            }
            else if (status == "creating_engine")
            {
                std::cout << "\r✓ Download complete. Registering engine...                                      " << std::endl;
            }
            else if (status == "engine_created")
            {
                std::cout << "✓ Engine registered successfully." << std::endl;
            }
            else if (status == "completed")
            {
                std::cout << "✓ Process completed." << std::endl;
            }
        },
        1000 // Check every 1 second
    );

    // Remove from active downloads list
    auto it = std::find(m_activeDownloads.begin(), m_activeDownloads.end(), engineId);
    if (it != m_activeDownloads.end())
    {
        m_activeDownloads.erase(it);
    }

    std::cout << std::endl; // New line after progress bar
    if (downloadSuccess)
    {
        std::cout << "✓ Model ready for inference." << std::endl;
        
        // Update config.yaml to persist the model across server restarts
        std::string localPath = "./models/" + modelFile.filename;
        updateConfigWithModel(engineId, localPath, false); // load_immediately = false for lazy loading
    }
    else
    {
        std::cout << "✗ Download failed." << std::endl;
    }

    return downloadSuccess;
}

std::string KolosalCLI::selectModel()
{
    std::cout << "Browsing Kolosal models...\n\n";

    // Fetch models from Hugging Face API
    std::vector<std::string> models = HuggingFaceClient::fetchKolosalModels();
    if (models.empty())
    {
        std::cout << "No models found. Showing sample menu...\n\n";

        // Fallback to sample menu
        models = generateSampleModels();
        Sleep(2000);
    }

    // Add navigation option
    models.push_back("Back to Main Menu");

    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1)
    {
        return models[result];
    }

    return ""; // Cancelled or back to main menu
}

ModelFile KolosalCLI::selectModelFile(const std::string &modelId)
{
    std::cout << "Selected model: " << modelId << std::endl;

    // Fetch .gguf files for the selected model
    std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFiles(modelId);
    if (modelFiles.empty())
    {
        std::cout << "No .gguf files found. Showing sample files...\n\n";
        // Fallback to sample files with quantization info
        modelFiles = generateSampleFiles(modelId);
        Sleep(2000);
    }
    std::cout << "Found " << modelFiles.size() << " .gguf file(s)!\n\n";

    // Show model files with real-time async memory updates and cache after completion
    int fileResult = ModelFileUtils::displayAsyncModelFileList(modelFiles, "Select a .gguf file:");

    // Cache the files with completed memory information (runs in background)
    std::thread cacheThread([modelId, modelFiles]() mutable
                            { ModelFileUtils::cacheModelFilesWithMemory(modelId, modelFiles); });
    cacheThread.detach();

    if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size()))
    {
        return modelFiles[fileResult];
    }

    return ModelFile{}; // Return empty ModelFile if cancelled or back selected
}

void KolosalCLI::showSelectionResult(const std::string &modelId, const ModelFile &modelFile)
{
    std::cout << "Selected file: " << modelFile.filename << std::endl;
    std::cout << "Quantization: " << modelFile.quant.type << " - " << modelFile.quant.description << std::endl;
    std::cout << "From model: " << modelId << std::endl;
    std::cout << "Download URL: https://huggingface.co/" << modelId << "/resolve/main/" << modelFile.filename << std::endl;
    std::cout << "\nFile download feature coming soon!" << std::endl;
}

std::string KolosalCLI::parseRepositoryInput(const std::string &input)
{
    if (input.empty())
    {
        return "";
    }

    // Remove whitespace
    std::string trimmed = input;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());

    // Check if it's a direct GGUF file URL first
    if (isDirectGGUFUrl(trimmed))
    {
        return "DIRECT_URL"; // Special marker for direct URLs
    }

    // Check if it's a full Hugging Face URL
    std::regex urlPattern(R"(https?://huggingface\.co/([^/]+/[^/?\s#]+))");
    std::smatch matches;

    if (std::regex_search(trimmed, matches, urlPattern))
    {
        std::string modelId = matches[1].str();
        // Remove any trailing slashes or query parameters
        size_t pos = modelId.find_first_of("/?#");
        if (pos != std::string::npos)
        {
            modelId = modelId.substr(0, pos);
        }
        return modelId;
    }

    // Check if it's already in the correct format (owner/model-name)
    std::regex idPattern(R"(^[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+$)");
    if (std::regex_match(trimmed, idPattern))
    {
        return trimmed;
    }

    return ""; // Invalid format
}

bool KolosalCLI::isDirectGGUFUrl(const std::string &input)
{
    if (input.empty())
    {
        return false;
    }

    // Check if it's a URL that ends with .gguf
    std::regex ggufUrlPattern(R"(^https?://[^\s]+\.gguf$)", std::regex_constants::icase);
    return std::regex_match(input, ggufUrlPattern);
}

bool KolosalCLI::handleDirectGGUFUrl(const std::string &url)
{
    std::cout << "Processing direct GGUF file URL...\n\n";

    // Extract filename from URL
    std::string filename;
    size_t lastSlash = url.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < url.length() - 1)
    {
        filename = url.substr(lastSlash + 1);
    }
    else
    {
        filename = "model.gguf"; // Fallback filename
    }
    // Create a ModelFile object for the direct URL
    ModelFile modelFile;
    modelFile.filename = filename;
    modelFile.downloadUrl = url;

    // Try to get cached model file info first
    std::string cacheKey = "direct_url:" + url;
    ModelFile cachedFile = CacheManager::getCachedModelFile(cacheKey);
    if (!cachedFile.filename.empty())
    {
        std::cout << "Using cached information for: " << filename << std::endl;
        modelFile = cachedFile;
    }
    else
    {
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
    if (modelFile.memoryUsage.hasEstimate)
    {
        std::cout << "Estimated Memory Usage: " << modelFile.memoryUsage.displayString << std::endl;
    }

    std::cout << std::endl;

    // Ask for confirmation
    std::cout << "Download this model? (y/n): ";
    char choice;
    std::cin >> choice;

    if (choice == 'y' || choice == 'Y')
    {
        // Ensure server is running and can be connected to
        if (!ensureServerConnection())
        {
            std::cerr << "Unable to connect to Kolosal server. Download cancelled." << std::endl;
            return false;
        }

        // Generate simplified engine ID from filename
        std::string engineId = filename;
        size_t dotPos = engineId.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            engineId = engineId.substr(0, dotPos);
        }

        // Check if engine already exists before attempting download
        if (m_serverClient->engineExists(engineId))
        {
            std::cout << "\n✓ Engine '" << engineId << "' already exists on the server." << std::endl;
            std::cout << "✓ Model is ready to use!" << std::endl;
            return true;
        }

        // Process download through server
        if (!m_serverClient->addEngine(engineId, url, "./models/" + filename))
        {
            std::cerr << "Failed to start download." << std::endl;
            return false;
        }

        m_activeDownloads.push_back(engineId);

        std::cout << "\n✓ Download started successfully!" << std::endl;
        std::cout << "Engine ID: " << engineId << std::endl;
        std::cout << "Downloading to: ./models/" << filename << std::endl;
        std::cout << "\nDownload initiated. Check server logs for progress." << std::endl;
        
        // Update config.yaml to persist the model across server restarts
        std::string localPath = "./models/" + filename;
        updateConfigWithModel(engineId, localPath, false); // load_immediately = false for lazy loading

        return true;
    }
    else
    {
        std::cout << "Download cancelled." << std::endl;
        return false;
    }
}

int KolosalCLI::run(const std::string &repoId)
{
    showWelcome();
    // Initialize Kolosal server first
    if (!initializeServer())
    {
        std::cerr << "Failed to initialize Kolosal server. Exiting." << std::endl;
        return 1;
    }

    // If a repository ID is provided, go directly to file selection
    if (!repoId.empty())
    {
        std::string modelId = parseRepositoryInput(repoId);
        if (modelId.empty())
        {
            std::cout << "Invalid repository URL or ID format.\n\n";
            std::cout << "Valid formats:\n";
            std::cout << "  • owner/model-name (e.g., microsoft/DialoGPT-medium)\n";
            std::cout << "  • https://huggingface.co/owner/model-name\n";
            std::cout << "  • Direct GGUF file URL (e.g., https://huggingface.co/owner/model/resolve/main/model.gguf)\n";
            return 1; // Exit with error code
        }
        else if (modelId == "DIRECT_URL")
        {
            // Handle direct GGUF file URL
            bool success = handleDirectGGUFUrl(repoId);
            return success ? 0 : 1;
        }
        else
        {
            std::cout << "Direct access to repository: " << modelId << "\n\n";

            // Fetch GGUF files from the specified repository
            std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFilesFromAnyRepo(modelId);
            if (modelFiles.empty())
            {
                std::cout << "No .gguf files found in repository: " << modelId << "\n";
                std::cout << "This repository may not contain quantized models.\n";
                return 1; // Exit with error code
            }
            else
            {
                std::cout << "Found " << modelFiles.size() << " .gguf file(s) in " << modelId << "\n\n";

                // Show model files with real-time async memory updates
                int fileResult = ModelFileUtils::displayAsyncModelFileList(modelFiles, "Select a .gguf file to download:");

                // Cache the files with completed memory information (runs in background)
                std::thread cacheThread([modelId, modelFiles]() mutable
                                        { ModelFileUtils::cacheModelFilesWithMemory(modelId, modelFiles); });
                cacheThread.detach();

                if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size()))
                {
                    // Process download through server
                    bool success = processModelDownload(modelId, modelFiles[fileResult]);
                    return success ? 0 : 1;
                }
                std::cout << "Selection cancelled." << std::endl;
                return 0;
            }
        }
    }

    // Standard flow: browse kolosal models
    while (true)
    {
        std::string selectedModel = selectModel();

        if (selectedModel.empty())
        {
            std::cout << "Model selection cancelled." << std::endl;
            return 0;
        }

        ModelFile selectedFile = selectModelFile(selectedModel);

        if (selectedFile.filename.empty())
        {
            // User selected "Back to Model Selection" - continue the loop
            continue;
        }

        // Process download through server
        bool success = processModelDownload(selectedModel, selectedFile);
        return success ? 0 : 1;
    }
}

void KolosalCLI::cancelActiveDownloads()
{
    if (m_activeDownloads.empty())
    {
        return;
    }

    std::cout << "Cancelling " << m_activeDownloads.size() << " active download(s)..." << std::endl;

    for (const std::string &downloadId : m_activeDownloads)
    {
        if (m_serverClient && m_serverClient->cancelDownload(downloadId))
        {
            std::cout << "Cancelled download: " << downloadId << std::endl;
        }
        else
        {
            std::cerr << "Failed to cancel download: " << downloadId << std::endl;
        }
    }

    m_activeDownloads.clear();
}

void KolosalCLI::signalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ". Performing cleanup..." << std::endl;

    if (s_instance)
    {
        s_instance->cancelActiveDownloads();

        // Note: We don't stop the server here since it should continue running
        // even after the CLI exits (background server model)
    }

    // Reset signal handler to default and re-raise signal for clean exit
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

bool KolosalCLI::ensureServerConnection()
{
    if (!m_serverClient)
    {
        std::cerr << "Server client not initialized." << std::endl;
        return false;
    }

    // First check if server is already healthy
    if (m_serverClient->isServerHealthy())
    {
        return true;
    }

    std::cout << "Connecting to Kolosal server..." << std::endl;

    // Try to start the server if it's not running
    if (!m_serverClient->startServer())
    {
        std::cerr << "Failed to start Kolosal server." << std::endl;
        return false;
    }

    // Wait for server to become ready with a shorter timeout for downloads
    if (!m_serverClient->waitForServerReady(15))
    {
        std::cerr << "Server failed to become ready within 15 seconds." << std::endl;
        return false;
    }

    return true;
}

bool KolosalCLI::updateConfigWithModel(const std::string& engineId, const std::string& modelPath, bool loadImmediately)
{
    try
    {
        std::string configPath = "config.yaml";
        YAML::Node config;
        
        // Try to load existing config
        if (std::filesystem::exists(configPath))
        {
            config = YAML::LoadFile(configPath);
        }
        else
        {
            // Create a minimal config structure if file doesn't exist
            config["models"] = YAML::Node(YAML::NodeType::Sequence);
        }
        
        // Ensure models section exists
        if (!config["models"])
        {
            config["models"] = YAML::Node(YAML::NodeType::Sequence);
        }
        
        // Check if model with this ID already exists
        bool modelExists = false;
        for (const auto& model : config["models"])
        {
            if (model["id"] && model["id"].as<std::string>() == engineId)
            {
                modelExists = true;
                break;
            }
        }
        
        // Only add if model doesn't already exist in config
        if (!modelExists)
        {
            YAML::Node newModel;
            newModel["id"] = engineId;
            newModel["path"] = modelPath;
            newModel["load_immediately"] = loadImmediately;
            newModel["main_gpu_id"] = 0;
            newModel["preload_context"] = false;
            
            // Add basic load_params
            YAML::Node loadParams;
            loadParams["n_ctx"] = 4096;
            loadParams["n_keep"] = 2048;
            loadParams["use_mmap"] = true;
            loadParams["use_mlock"] = false;
            loadParams["n_parallel"] = 1;
            loadParams["cont_batching"] = true;
            loadParams["warmup"] = false;
            loadParams["n_gpu_layers"] = 50;
            loadParams["n_batch"] = 2048;
            loadParams["n_ubatch"] = 512;
            newModel["load_params"] = loadParams;
            
            config["models"].push_back(newModel);
            
            // Write updated config back to file
            std::ofstream configFile(configPath);
            if (!configFile.is_open())
            {
                std::cerr << "Failed to open config file for writing: " << configPath << std::endl;
                return false;
            }
            
            configFile << config;
            configFile.close();
            
            std::cout << "✓ Added model '" << engineId << "' to config.yaml" << std::endl;
            return true;
        }
        else
        {
            // Model already exists in config - no need to announce this
            return true;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to update config.yaml: " << e.what() << std::endl;
        return false;
    }
}
