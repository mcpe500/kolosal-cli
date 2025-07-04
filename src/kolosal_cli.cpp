#include "kolosal_cli.h"
#include "http_client.h"
#include "hugging_face_client.h"
#include "interactive_list.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include "kolosal_server_client.h"
#include "command_manager.h"
#include <iostream>
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

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

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
    
    // Initialize command manager
    m_commandManager = std::make_unique<CommandManager>();

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
    // ANSI color codes for gradient effect (cyan to blue to purple)
    const std::string colors[] = {
        "\033[38;5;51m",   // Bright cyan
        "\033[38;5;45m",   // Cyan
        "\033[38;5;39m",   // Light blue
        "\033[38;5;33m",   // Blue
        "\033[38;5;27m",   // Dark blue
        "\033[38;5;21m",   // Blue-purple
        "\033[38;5;57m",   // Purple
        "\033[38;5;93m"    // Dark purple
    };
    const std::string reset = "\033[0m";
    
    std::cout << "\n";
    std::cout << colors[0] << "       ██     ██   ██   ███████   ██         ███████    ████████     ██     ██" << reset << "\n";
    std::cout << colors[1] << "     ██░     ░██  ██   ██░░░░░██ ░██        ██░░░░░██  ██░░░░░░     ████   ░██" << reset << "\n";
    std::cout << colors[2] << "   ██░       ░██ ██   ██     ░░██░██       ██     ░░██░██          ██░░██  ░██" << reset << "\n";
    std::cout << colors[3] << " ██░         ░████   ░██      ░██░██      ░██      ░██░█████████  ██  ░░██ ░██" << reset << "\n";
    std::cout << colors[4] << "░░ ██        ░██░██  ░██      ░██░██      ░██      ░██░░░░░░░░██ ██████████░██" << reset << "\n";
    std::cout << colors[5] << "  ░░ ██      ░██░░██ ░░██     ██ ░██      ░░██     ██        ░██░██░░░░░░██░██" << reset << "\n";
    std::cout << colors[6] << "    ░░ ██    ░██ ░░██ ░░███████  ░████████ ░░███████   ████████ ░██     ░██░████████" << reset << "\n";
    std::cout << colors[7] << "      ░░     ░░   ░░   ░░░░░░░   ░░░░░░░░   ░░░░░░░   ░░░░░░░░  ░░      ░░ ░░░░░░░░" << reset << "\n";
    std::cout << "\n";
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
        
        // Start chat interface directly
        std::cout << "\nStarting chat interface..." << std::endl;
        startChatInterface(engineId);
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
        
        // Start chat interface
        std::cout << "\n🎉 Model downloaded and registered successfully!" << std::endl;
        std::cout << "Starting chat interface..." << std::endl;
        startChatInterface(engineId);
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
#ifdef _WIN32
        Sleep(2000);
#else
        sleep(2);
#endif
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
#ifdef _WIN32
        Sleep(2000);
#else
        sleep(2);
#endif
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
            newModel["preload_context"] = true;
            
            // Add load_params with specified configuration
            YAML::Node loadParams;
            loadParams["n_ctx"] = 4096;
            loadParams["n_keep"] = 2048;
            loadParams["use_mmap"] = true;
            loadParams["use_mlock"] = true;
            loadParams["n_parallel"] = 4;
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
    catch (const std::exception &e)
    {
        std::cerr << "Error updating config: " << e.what() << std::endl;
        return false;
    }
}

bool KolosalCLI::startChatInterface(const std::string& engineId)
{
    // Clear the console screen
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', consoleSize, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);
#else
    // Clear screen on Linux using ANSI escape sequences
    std::cout << "\033[2J\033[H" << std::flush;
#endif

    // Print minimal chat header
    std::cout << "Running: ";
    // Model id in magenta
    std::cout << "\033[35m" << engineId << "\033[0m" << std::endl;
    std::cout << "Type '/exit' or press Ctrl+C to quit" << std::endl;
    std::cout << "Type '/help' to see available commands" << std::endl;

    // Set up signal handling for graceful exit
    bool shouldExit = false;
    // Initialize command manager with current context
    m_commandManager->setCurrentEngine(engineId);
    
#ifdef _WIN32
    // Windows-specific console control handler
    auto consoleHandler = [](DWORD dwCtrlType) -> BOOL {
        if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
            ExitProcess(0);
            return TRUE;
        }
        return FALSE;
    };
    
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    // Unix-style signal handling
    signal(SIGINT, [](int) {
        exit(0);
    });
#endif

    std::vector<std::pair<std::string, std::string>> chatHistory;
    
    // Set chat history reference for command manager
    m_commandManager->setChatHistory(&chatHistory);

    while (!shouldExit)
    {
        // Get user input with real-time autocomplete support
        std::cout << "\n";
        std::string userInput = getInputWithRealTimeAutocomplete("");

        // Check if input was cancelled (empty string returned)
        if (userInput.empty() && std::cin.eof())
        {
            // Handle EOF (Ctrl+C or stream closed)
            break;
        }

        // Trim whitespace
        userInput.erase(0, userInput.find_first_not_of(" \t\n\r\f\v"));
        userInput.erase(userInput.find_last_not_of(" \t\n\r\f\v") + 1);

        // Skip empty messages
        if (userInput.empty())
        {
            continue;
        }

        // Check if input is a command
        if (m_commandManager->isCommand(userInput))
        {
            CommandResult result = m_commandManager->executeCommand(userInput);

            if (!result.message.empty()) {
                std::cout << "\n\033[33m> " << result.message << "\033[0m\n" << std::endl;
            }

            if (result.shouldExit) {
                shouldExit = true;
                break;
            }

            if (!result.shouldContinueChat) {
                continue;
            }

            // Don't add commands to chat history unless they're conversation-related
            continue;
        }

        // Check for legacy exit commands (for backward compatibility)
        if (userInput == "exit" || userInput == "quit")
        {
            shouldExit = true;
            break;
        }

        // Add user message to history
        chatHistory.push_back({"user", userInput});

        // Force clear any lingering suggestions before showing AI response
        forceClearSuggestions();



        // Add an empty line to visually separate user input from spinner/assistant response
        std::cout << std::endl;
        // Show loading spinner using LoadingAnimation until first model response
        std::atomic<bool> gotFirstChunk{false};
        LoadingAnimation loadingAnim("");
        std::string fullResponse;
        bool printedPrompt = false;

        // Start spinner in a thread, but print prompt and clear spinner on first token
        std::thread loadingThread([&gotFirstChunk, &loadingAnim]() {
            loadingAnim.start();
            while (!gotFirstChunk) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            // Spinner will be cleared by main thread after first token
        });
        
        // Variables to track metrics and line state
        double currentTps = 0.0;
        double ttft = 0.0;
        bool hasMetrics = false;
        bool metricsShown = false;
        bool lastWasNewline = false;
        int currentColumn = 0;  // Track horizontal cursor position
        int terminalWidth = 80; // Default width, will be updated
        int terminalHeight = 24; // Default height, will be updated
        
#ifdef _WIN32
        // Get actual terminal dimensions on Windows
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
            terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            terminalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
#else
        // Get actual terminal dimensions on Linux
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            terminalWidth = w.ws_col;
            terminalHeight = w.ws_row;
        }
#endif
        
        bool success = m_serverClient->streamingChatCompletion(engineId, userInput,
            [&](const std::string& chunk, double tps, double timeToFirstToken) {
                if (!gotFirstChunk) {
                    gotFirstChunk = true;
                    // Move to start of spinner line, clear it, then print prompt and first token
                    loadingAnim.stop();
                    std::cout << "\r\033[32m> \033[32m";
                    printedPrompt = true;
                }
                
                // Update metrics
                if (tps > 0) {
                    currentTps = tps;
                    hasMetrics = true;
                }
                if (timeToFirstToken > 0) {
                    ttft = timeToFirstToken;
                }
                
                // Check if we need to clear previous metrics due to newline
                bool containsNewline = chunk.find('\n') != std::string::npos;
                if (containsNewline && metricsShown) {
                    // Clear the metrics line before printing the chunk
                    std::cout << "\033[s";  // Save cursor position
                    std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                    std::cout << "\033[u";  // Restore cursor position
                    metricsShown = false;
                }
                
                // Calculate if this chunk will cause line wrapping
                bool willWrap = false;
                int chunkVisibleLength = 0;
                for (char c : chunk) {
                    if (c == '\n') {
                        currentColumn = 0;
                    } else if (c >= 32 && c <= 126) { // Printable characters
                        chunkVisibleLength++;
                        if (currentColumn + chunkVisibleLength >= terminalWidth) {
                            willWrap = true;
                            break;
                        }
                    }
                }
                
                // Clear metrics if line wrapping will occur
                if (willWrap && metricsShown) {
                    std::cout << "\033[s";  // Save cursor position
                    std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                    std::cout << "\033[u";  // Restore cursor position
                    metricsShown = false;
                }
                
                std::cout << chunk;
                std::cout.flush();
                fullResponse += chunk;
                
                // Update cursor position tracking
                for (char c : chunk) {
                    if (c == '\n') {
                        currentColumn = 0;
                    } else if (c >= 32 && c <= 126) { // Printable characters
                        currentColumn++;
                        if (currentColumn >= terminalWidth) {
                            currentColumn = 0; // Wrapped to new line
                        }
                    }
                }
                
                // Update line state and clear metrics if newline was processed
                if (containsNewline || willWrap) {
                    lastWasNewline = true;
                    // Clear metrics again after printing the chunk to ensure they're gone
                    if (metricsShown) {
                        std::cout << "\033[s";  // Save cursor position
                        std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                        std::cout << "\033[u";  // Restore cursor position
                        metricsShown = false;
                    }
                } else {
                    lastWasNewline = false;
                }
                
                // Show metrics in real-time below the response (but not immediately after newlines)
                if (hasMetrics && !lastWasNewline) {
                    // Check if we're near the bottom of the terminal to avoid overriding text
                    bool canShowMetricsBelow = true;
                    
#ifdef _WIN32
                    CONSOLE_SCREEN_BUFFER_INFO currentCsbi;
                    if (GetConsoleScreenBufferInfo(hConsole, &currentCsbi)) {
                        int currentRow = currentCsbi.dwCursorPosition.Y;
                        int bottomRow = currentCsbi.srWindow.Bottom;
                        // Don't show metrics below if we're on the last line or second-to-last line
                        if (currentRow >= bottomRow - 1) {
                            canShowMetricsBelow = false;
                        }
                    }
#else
                    // On Linux, check if we're near the bottom by getting cursor position
                    // Use ANSI escape sequence to query cursor position
                    std::cout << "\033[6n"; // Query cursor position
                    std::cout.flush();
                    
                    // Parse response (ESC[row;colR) - simplified approach
                    // For safety, we'll be conservative and not show metrics if we might be near bottom
                    // This prevents scrolling issues in most cases
                    if (terminalHeight > 0) {
                        // Assume we might be near bottom if terminal height is small
                        // or if we've been printing for a while (conservative approach)
                        canShowMetricsBelow = terminalHeight > 10; // Only show metrics if terminal is reasonably tall
                    }
#endif
                    
                    if (canShowMetricsBelow) {
                        // Save current cursor position
                        std::cout << "\033[s";
                        
                        // Move to next line and go to beginning of line, then clear and show metrics
                        std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear line
                        std::cout << "\033[90m"; // Dim gray color
                        if (ttft > 0) {
                            std::cout << "TTFT: " << std::fixed << std::setprecision(2) << ttft << "ms";
                        }
                        if (currentTps > 0) {
                            if (ttft > 0) std::cout << " | ";
                            std::cout << "TPS: " << std::fixed << std::setprecision(1) << currentTps;
                        }
                        std::cout << "\033[0m";
                        metricsShown = true;
                        
                        // Restore cursor position
                        std::cout << "\033[u";
                        std::cout.flush();
                    } else {
                        // If we can't show metrics below, clear any existing metrics
                        if (metricsShown) {
                            std::cout << "\033[s";  // Save cursor position
                            std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                            std::cout << "\033[u";  // Restore cursor position
                            metricsShown = false;
                        }
                    }
                }
            });
        gotFirstChunk = true;
        if (loadingThread.joinable()) loadingThread.join();
        
        // If the model never sent any chunk, still print the prompt for consistency and clear spinner
        if (!printedPrompt) {
            loadingAnim.stop();
            std::cout << "\n\033[32m> \033[32m";
            std::cout.flush();
        }

        // Clear metrics line after completion to clean up
        if (metricsShown) {
            std::cout << "\033[s";  // Save cursor position
            std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
            std::cout << "\033[u";  // Restore cursor position
        }

        if (!success && fullResponse.empty())
        {
            std::cout << "❌ Error: Failed to get response from the model. Please try again.";
            continue;
        }

        // Add assistant response to history
        if (!fullResponse.empty())
        {
            chatHistory.push_back({"assistant", fullResponse});
        }

        std::cout << "\033[0m";
        
        // Display final metrics below the completed response
        if (hasMetrics && (ttft > 0 || currentTps > 0)) {
            std::cout << "\n\033[90m"; // New line and dim gray color
            if (ttft > 0) {
                std::cout << "TTFT: " << std::fixed << std::setprecision(2) << ttft << "ms";
            }
            if (currentTps > 0) {
                if (ttft > 0) std::cout << " | ";
                std::cout << "TPS: " << std::fixed << std::setprecision(1) << currentTps;
            }
            std::cout << "\033[0m"; // Reset color
        }
        
        std::cout << std::endl;
    }

#ifdef _WIN32
    // Restore default console control handler
    SetConsoleCtrlHandler(nullptr, FALSE);
#endif

    return true;
}

std::string KolosalCLI::getInputWithRealTimeAutocomplete(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    
    // Enable input processing for character-by-character reading
    SetConsoleMode(hConsole, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
    
    bool showingSuggestions = false;
    int suggestionStartRow = 0;
    bool showingHint = false;  // Track if we're showing hint text (should start as false)
    const std::string hintText = "Type your message or use /help for commands...";
    
    // Get initial cursor position and ensure we have enough screen space
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    COORD promptPos = csbi.dwCursorPosition;
    
    // Pre-allocate space for suggestions by positioning cursor lower, then returning
    if (csbi.dwCursorPosition.Y > csbi.dwSize.Y - 8) {
        // If we're too close to bottom, scroll up
        std::cout << "\n\n\n\n\n";
        GetConsoleScreenBufferInfo(hOut, &csbi);
        promptPos = csbi.dwCursorPosition;
        // Move cursor back up to where we want the input line
        promptPos.Y -= 5;
        SetConsoleCursorPosition(hOut, promptPos);
    }
    
    // Show initial hint text only if input is empty
    if (input.empty()) {
        displayHintText(hintText, showingHint);
    }
    
    while (true) {
        INPUT_RECORD inputRecord;
        DWORD eventsRead;
        ReadConsoleInput(hConsole, &inputRecord, 1, &eventsRead);
        
        if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
            WORD keyCode = inputRecord.Event.KeyEvent.wVirtualKeyCode;
            char ch = inputRecord.Event.KeyEvent.uChar.AsciiChar;
            
            if (keyCode == VK_RETURN) {
                // Enter pressed - finalize input
                if (showingHint) {
                    clearHintText(showingHint);
                }
                if (showingSuggestions) {
                    clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                    // Force flush any remaining output
                    std::cout.flush();
                }
                std::cout << std::endl;
                break;
            } else if (keyCode == VK_ESCAPE) {
                // Escape pressed - clear suggestions and continue
                if (showingSuggestions) {
                    clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                }
                continue;
            } else if (keyCode == VK_TAB) {
                // Tab pressed - show interactive selection for commands
                if (input.length() >= 2 && input[0] == '/') {
                    auto suggestions = m_commandManager->getCommandSuggestions(input);
                    if (suggestions.size() >= 1) {
                        if (showingSuggestions) {
                            clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                        }
                        
                        if (suggestions.size() == 1) {
                            // Auto-complete single match
                            clearCurrentInput(input, showingSuggestions, suggestionStartRow, prompt);
                            input = "/" + suggestions[0];
                            std::cout << "\033[96m" << input << "\033[0m";
                            // Update cursor position
                            GetConsoleScreenBufferInfo(hOut, &csbi);
                            promptPos = csbi.dwCursorPosition;
                        } else {
                            // Show interactive selection for multiple suggestions
                            if (showingSuggestions) {
                                clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                            }
                            std::cout << std::endl;
                            
                            auto formattedSuggestions = m_commandManager->getFormattedCommandSuggestions(input);
                            formattedSuggestions.push_back("Continue with '" + input + "'");
                            
                            InteractiveList suggestionList(formattedSuggestions);
                            int selected = suggestionList.run();
                            
                            if (selected >= 0 && selected < static_cast<int>(suggestions.size())) {
                                // User selected a command
                                input = "/" + suggestions[selected];
                                std::cout << "U: \033[96m" << input << "\033[0m";
                            } else if (selected == static_cast<int>(suggestions.size())) {
                                // User chose to continue with their input
                                std::cout << "U: \033[96m" << input << "\033[0m";
                            } else {
                                // User cancelled - return empty
                                SetConsoleMode(hConsole, mode);
                                return "";
                            }
                            
                            // Update cursor position after interactive selection
                            GetConsoleScreenBufferInfo(hOut, &csbi);
                            promptPos = csbi.dwCursorPosition;
                            showingSuggestions = false; // Reset suggestion state
                        }
                    }
                }
                continue;
            } else if (keyCode == VK_BACK) {
                // Backspace pressed
                if (!input.empty()) {
                    input.pop_back();
                    // Move cursor back and clear the character
                    std::cout << "\b \b";
                    
                    // Update cursor position
                    GetConsoleScreenBufferInfo(hOut, &csbi);
                    promptPos = csbi.dwCursorPosition;
                    
                    // If input becomes empty, show hint text again
                    if (input.empty() && !showingHint) {
                        displayHintText(hintText, showingHint);
                    }
                    
                    // Update or clear suggestions
                    if (input.length() >= 2 && input[0] == '/') {
                        updateRealTimeSuggestions(input, showingSuggestions, suggestionStartRow, prompt);
                    } else if (showingSuggestions) {
                        clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                    }
                }
                continue;
            } else if (ch >= 32 && ch <= 126) {
                // Regular printable character
                
                // Clear hint text on any character typed (whether first or subsequent)
                if (showingHint) {
                    clearHintText(showingHint);
                }
                
                input += ch;
                
                // Display user input in cyan color
                std::cout << "\033[96m" << ch << "\033[0m";
                
                // Update cursor position
                GetConsoleScreenBufferInfo(hOut, &csbi);
                promptPos = csbi.dwCursorPosition;
                
                // Show suggestions for commands
                if (input.length() >= 2 && input[0] == '/') {
                    updateRealTimeSuggestions(input, showingSuggestions, suggestionStartRow, prompt);
                } else if (showingSuggestions) {
                    clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
                }
            }
        }
    }
    
    // Restore console mode
    SetConsoleMode(hConsole, mode);
#else
    // Linux implementation with hint text support
    bool showingHint = false;
    bool showingSuggestions = false;
    int suggestionStartRow = 0;
    const std::string hintText = "Type your message or use /help for commands...";
    
    // Set terminal to raw mode for character-by-character input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Show initial hint text if input is empty
    if (input.empty()) {
        displayHintTextLinux(hintText, showingHint);
    }
    
    while (true) {
        int ch = getchar();
        
        if (ch == 10 || ch == 13) { // Enter (LF or CR)
            if (showingHint) {
                clearHintTextLinux(hintText, showingHint);
            }
            if (showingSuggestions) {
                clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
            }
            std::cout << std::endl;
            break;
        } else if (ch == 27) { // ESC sequence
            // Handle arrow keys or standalone ESC
            int next1 = getchar();
            if (next1 == '[') {
                int next2 = getchar();
                // Ignore arrow keys in input mode, just continue
                continue;
            } else {
                // Standalone ESC - ignore for now, continue
                ungetc(next1, stdin);
                continue;
            }
        } else if (ch == 127 || ch == 8) { // Backspace (DEL or BS)
            if (!input.empty()) {
                input.pop_back();
                std::cout << "\b \b" << std::flush;
                
                // If input becomes empty, show hint text again
                if (input.empty() && !showingHint) {
                    displayHintTextLinux(hintText, showingHint);
                }
                
                // Update or clear suggestions
                if (input.length() >= 2 && input[0] == '/') {
                    updateRealTimeSuggestionsLinux(input, showingSuggestions, suggestionStartRow, prompt);
                } else if (showingSuggestions) {
                    clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
                }
            }
        } else if (ch == 9) { // Tab - for command suggestions
            if (input.length() >= 2 && input[0] == '/') {
                auto suggestions = m_commandManager->getCommandSuggestions(input);
                if (!suggestions.empty()) {
                    if (showingHint) {
                        clearHintTextLinux(hintText, showingHint);
                    }
                    if (showingSuggestions) {
                        clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
                    }
                    
                    // Clear current input line for interactive selection
                    for (size_t i = 0; i < input.length(); ++i) {
                        std::cout << "\b \b";
                    }
                    std::cout << std::flush;
                    
                    if (suggestions.size() == 1) {
                        // Auto-complete single match
                        input = "/" + suggestions[0];
                        std::cout << "\033[96m" << input << "\033[0m" << std::flush;
                    } else {
                        // Show interactive selection
                        std::cout << std::endl;
                        auto formattedSuggestions = m_commandManager->getFormattedCommandSuggestions(input);
                        formattedSuggestions.push_back("Continue with '" + input + "'");
                        
                        InteractiveList suggestionList(formattedSuggestions);
                        int selected = suggestionList.run();
                        
                        if (selected >= 0 && selected < static_cast<int>(suggestions.size())) {
                            input = "/" + suggestions[selected];
                            std::cout << "U: \033[96m" << input << "\033[0m" << std::flush;
                        } else if (selected == static_cast<int>(suggestions.size())) {
                            std::cout << "U: \033[96m" << input << "\033[0m" << std::flush;
                        } else {
                            // User cancelled
                            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                            return "";
                        }
                        showingSuggestions = false; // Reset suggestion state
                    }
                }
            }
        } else if (ch >= 32 && ch <= 126) { // Printable characters
            // Clear hint text on any character typed
            if (showingHint) {
                clearHintTextLinux(hintText, showingHint);
            }
            
            input += ch;
            // Display user input in cyan color
            std::cout << "\033[96m" << static_cast<char>(ch) << "\033[0m" << std::flush;
            
            // Show suggestions for commands
            if (input.length() >= 2 && input[0] == '/') {
                updateRealTimeSuggestionsLinux(input, showingSuggestions, suggestionStartRow, prompt);
            } else if (showingSuggestions) {
                clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
            }
        } else if (ch == 3) { // Ctrl+C
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return "";
        }
        // Ignore other control characters
    }
    
    // Restore terminal mode
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    
    return input;
}

void KolosalCLI::updateRealTimeSuggestions(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt) {
    auto suggestions = m_commandManager->getCommandSuggestions(input);
    
    if (suggestions.empty()) {
        if (showingSuggestions) {
            clearSuggestions(showingSuggestions, suggestionStartRow, prompt, input);
        }
        return;
    }
    
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    // Store current cursor position (input line)
    COORD inputPos = csbi.dwCursorPosition;
    
    if (!showingSuggestions) {
        // First time showing suggestions - reserve space below input
        suggestionStartRow = inputPos.Y + 1;
        showingSuggestions = true;
        
        // Move cursor down to create space for suggestions without printing newlines
        COORD newPos = {0, static_cast<SHORT>(suggestionStartRow + 5)}; // Reserve 5 lines for suggestions
        SetConsoleCursorPosition(hConsole, newPos);
        SetConsoleCursorPosition(hConsole, inputPos); // Move back to input position
    }
    
    // Clear and update suggestion area
    COORD suggestionPos = {0, static_cast<SHORT>(suggestionStartRow)};
    SetConsoleCursorPosition(hConsole, suggestionPos);
    
    // Clear the suggestion area first
    DWORD written;
    COORD clearPos = {0, static_cast<SHORT>(suggestionStartRow)};
    for (int i = 0; i < 5; ++i) {
        SetConsoleCursorPosition(hConsole, clearPos);
        FillConsoleOutputCharacter(hConsole, ' ', 80, clearPos, &written);
        clearPos.Y++;
    }
    
    // Position cursor at suggestion area and display new suggestions
    SetConsoleCursorPosition(hConsole, suggestionPos);
    
    auto formattedSuggestions = m_commandManager->getFormattedCommandSuggestions(input);
    size_t maxDisplay = 3;
    size_t displayCount = (formattedSuggestions.size() < maxDisplay) ? formattedSuggestions.size() : maxDisplay;
    
    // Write suggestions line by line at specific positions with darker color
    COORD writePos = suggestionPos;
    
    // Set text color to dark gray for suggestions
    CONSOLE_SCREEN_BUFFER_INFO originalCsbi;
    GetConsoleScreenBufferInfo(hConsole, &originalCsbi);
    WORD originalAttributes = originalCsbi.wAttributes;
    WORD darkGrayAttributes = (originalAttributes & 0xF0) | 8; // Keep background, set foreground to dark gray
    
    for (size_t i = 0; i < displayCount; ++i) {
        SetConsoleCursorPosition(hConsole, writePos);
        SetConsoleTextAttribute(hConsole, darkGrayAttributes);
        std::string suggestionText = "  " + formattedSuggestions[i];
        std::cout << suggestionText;
        writePos.Y++;
    }
    
    if (formattedSuggestions.size() > maxDisplay) {
        SetConsoleCursorPosition(hConsole, writePos);
        SetConsoleTextAttribute(hConsole, darkGrayAttributes);
        std::string moreText = "  ... and " + std::to_string(formattedSuggestions.size() - maxDisplay) + " more";
        std::cout << moreText;
    }
    
    // Restore original text color
    SetConsoleTextAttribute(hConsole, originalAttributes);
    
    // Move cursor back to input line at the correct position
    SetConsoleCursorPosition(hConsole, inputPos);
#else
    // Linux implementation
    if (suggestions.empty()) {
        if (showingSuggestions) {
            clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
        }
        return;
    }
    
    if (!showingSuggestions) {
        showingSuggestions = true;
        suggestionStartRow = 1; // Reserve space below input line
    }
    
    // Save current cursor position
    printf("\033[s"); // Save cursor position
    
    // Move to suggestion area and display suggestions
    printf("\033[%d;1H", suggestionStartRow + 1); // Move to suggestion line
    
    auto formattedSuggestions = m_commandManager->getFormattedCommandSuggestions(input);
    size_t maxDisplay = 3;
    size_t displayCount = (formattedSuggestions.size() < maxDisplay) ? formattedSuggestions.size() : maxDisplay;
    
    // Clear the suggestion area first (3-4 lines)
    for (int i = 0; i < 4; ++i) {
        printf("\033[%d;1H\033[K", suggestionStartRow + 1 + i); // Move and clear line
    }
    
    // Display suggestions in dark gray
    for (size_t i = 0; i < displayCount; ++i) {
        printf("\033[%d;1H", suggestionStartRow + 1 + (int)i); // Move to line
        printf("\033[90m  %s\033[0m", formattedSuggestions[i].c_str()); // Dark gray
    }
    
    if (formattedSuggestions.size() > maxDisplay) {
        printf("\033[%d;1H", suggestionStartRow + 1 + (int)displayCount);
        printf("\033[90m  ... and %zu more\033[0m", formattedSuggestions.size() - maxDisplay);
    }
    
    // Restore cursor position
    printf("\033[u"); // Restore cursor position
    fflush(stdout);
#endif
}

void KolosalCLI::updateRealTimeSuggestionsLinux(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt) {
#ifndef _WIN32
    auto suggestions = m_commandManager->getCommandSuggestions(input);
    
    if (suggestions.empty()) {
        if (showingSuggestions) {
            clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
        }
        return;
    }
    
    if (!showingSuggestions) {
        showingSuggestions = true;
        suggestionStartRow = 1; // Mark that we're showing suggestions below
    }
    
    // Save current cursor position (input line)
    printf("\033[s");
    
    // Move cursor down one line to start suggestions below input
    printf("\033[B");  // Move cursor down one line
    printf("\033[1G"); // Move to beginning of line
    
    auto formattedSuggestions = m_commandManager->getFormattedCommandSuggestions(input);
    size_t maxDisplay = 3;
    size_t displayCount = (formattedSuggestions.size() < maxDisplay) ? formattedSuggestions.size() : maxDisplay;
    
    // Clear the suggestion area first (4 lines should be enough)
    for (int i = 0; i < 4; ++i) {
        printf("\033[K"); // Clear current line
        if (i < 3) printf("\033[B"); // Move to next line
    }
    
    // Move back up to start of suggestions area
    for (int i = 0; i < 3; ++i) {
        printf("\033[A"); // Move cursor up
    }
    printf("\033[1G"); // Move to beginning of line
    
    // Display suggestions in dark gray
    for (size_t i = 0; i < displayCount; ++i) {
        printf("\033[K"); // Clear current line
        printf("\033[90m  %s\033[0m", formattedSuggestions[i].c_str()); // Dark gray
        if (i < displayCount - 1) {
            printf("\033[B"); // Move to next line for next suggestion
            printf("\033[1G"); // Move to beginning of line
        }
    }
    
    if (formattedSuggestions.size() > maxDisplay) {
        printf("\033[B"); // Move to next line
        printf("\033[1G\033[K"); // Beginning of line and clear
        printf("\033[90m  ... and %zu more\033[0m", formattedSuggestions.size() - maxDisplay);
    }
    
    // Restore cursor position to input line
    printf("\033[u");
    fflush(stdout);
#endif
}

void KolosalCLI::clearSuggestions(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input) {
    if (!showingSuggestions) return;
    
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    // Store current cursor position
    COORD inputPos = csbi.dwCursorPosition;
    
    // Clear suggestions by using FillConsoleOutputCharacter and reset attributes
    DWORD written;
    COORD clearPos = {0, static_cast<SHORT>(suggestionStartRow)};
    
    // Clear multiple lines efficiently and more thoroughly
    for (int i = 0; i < 8; ++i) { // Increased from 5 to 8 lines to be more thorough
        FillConsoleOutputCharacter(hConsole, ' ', 120, clearPos, &written); // Increased from 80 to 120 chars
        // Also reset attributes to default
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, 120, clearPos, &written);
        clearPos.Y++;
    }
    
    // Move cursor back to original input position
    SetConsoleCursorPosition(hConsole, inputPos);
    
    // Force flush the console buffer
    std::cout.flush();
#endif
    
    showingSuggestions = false;
}

void KolosalCLI::clearSuggestionsLinux(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input) {
#ifndef _WIN32
    if (!showingSuggestions) return;
    
    // Save current cursor position (input line)
    printf("\033[s");
    
    // Move cursor down one line to start clearing suggestions below input
    printf("\033[B");  // Move cursor down one line
    printf("\033[1G"); // Move to beginning of line
    
    // Clear the suggestion area (4 lines should be enough)
    for (int i = 0; i < 4; ++i) {
        printf("\033[K"); // Clear current line
        if (i < 3) {
            printf("\033[B"); // Move to next line
            printf("\033[1G"); // Move to beginning of line
        }
    }
    
    // Restore cursor position to input line
    printf("\033[u");
    fflush(stdout);
    
    showingSuggestions = false;
#endif
}

void KolosalCLI::clearCurrentInput(const std::string& input, bool& showingSuggestions, int suggestionStartRow, const std::string& prompt) {
#ifdef _WIN32
    // Clear only the user input part (after the '> ')
    for (size_t i = 0; i < input.length(); ++i) {
        std::cout << "\b \b";
    }
    
    // Clear suggestions if showing
    if (showingSuggestions) {
        clearSuggestions(showingSuggestions, suggestionStartRow, prompt, "");
    }
#endif
}

void KolosalCLI::forceClearSuggestions() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    // Store current cursor position
    COORD currentPos = csbi.dwCursorPosition;
    
    // Clear several lines below current position to ensure suggestions are gone
    DWORD written;
    for (int i = 1; i <= 10; ++i) {
        COORD clearPos = {0, static_cast<SHORT>(currentPos.Y + i)};
        FillConsoleOutputCharacter(hConsole, ' ', 120, clearPos, &written);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, 120, clearPos, &written);
    }
    
    // Restore cursor position
    SetConsoleCursorPosition(hConsole, currentPos);
    std::cout.flush();
#endif
}

void KolosalCLI::displayHintText(const std::string& hintText, bool& showingHint) {
#ifdef _WIN32
    if (showingHint) return; // Already showing hint
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    // Store current cursor position (where user will type)
    COORD inputPos = csbi.dwCursorPosition;
    
    // Get original attributes for restoration
    WORD originalAttributes = csbi.wAttributes;
    
    // Set dark gray color for hint text (same as command suggestions)
    WORD hintAttributes = (originalAttributes & 0xF0) | 8; // Dark gray
    SetConsoleTextAttribute(hConsole, hintAttributes);
    
    // Display hint text at current cursor position
    std::cout << hintText;
    
    // Restore original attributes
    SetConsoleTextAttribute(hConsole, originalAttributes);
    
    // Move cursor back to the beginning (where user should type)
    SetConsoleCursorPosition(hConsole, inputPos);
    
    showingHint = true;
    std::cout.flush();
#endif
}

void KolosalCLI::displayHintTextLinux(const std::string& hintText, bool& showingHint) {
#ifndef _WIN32
    if (showingHint) return; // Already showing hint
    
    // Display hint text in dark gray color
    std::cout << "\033[90m" << hintText << "\033[0m" << std::flush;
    
    // Move cursor back to the beginning (where user should type)
    for (size_t i = 0; i < hintText.length(); ++i) {
        std::cout << "\b";
    }
    std::cout << std::flush;
    
    showingHint = true;
#endif
}

void KolosalCLI::clearHintText(bool& showingHint) {
#ifdef _WIN32
    if (!showingHint) return; // No hint to clear
    const std::string hintText = "Type your message or use /help for commands...";
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD inputPos = csbi.dwCursorPosition;
    // Move to start of input (where hint was drawn)
    SetConsoleCursorPosition(hConsole, inputPos);
    // Overwrite hint text with spaces
    DWORD written;
    std::string clearLine(hintText.length(), ' ');
    WriteConsoleA(hConsole, clearLine.c_str(), static_cast<DWORD>(clearLine.length()), &written, NULL);
    // Move cursor back to start
    SetConsoleCursorPosition(hConsole, inputPos);
    showingHint = false;
    std::cout.flush();
#endif
}

void KolosalCLI::clearHintTextLinux(const std::string& hintText, bool& showingHint) {
#ifndef _WIN32
    if (!showingHint) return; // No hint to clear
    
    // Overwrite hint text with spaces
    std::string clearText(hintText.length(), ' ');
    std::cout << clearText << std::flush;
    
    // Move cursor back to start
    for (size_t i = 0; i < hintText.length(); ++i) {
        std::cout << "\b";
    }
    std::cout << std::flush;
    
    showingHint = false;
#endif
}
