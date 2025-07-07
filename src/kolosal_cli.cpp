#include "kolosal_cli.h"
#include "http_client.h"
#include "hugging_face_client.h"
#include "interactive_list.h"
#include "model_file.h"
#include "cache_manager.h"
#include "loading_animation.h"
#include "kolosal_server_client.h"
#include "command_manager.h"
#include "model_repo_selector.h"
#include "model_file_selector.h"
#include "chat_interface.h"
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
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

// Static member initialization
KolosalCLI *KolosalCLI::s_instance = nullptr;

// Helper function to escape file paths for shell/server usage
static std::string escapeFilePath(const std::string& path) {
    std::string escapedPath = path;
    
    // Replace backslashes with forward slashes for consistency (works on both Windows and Linux)
    std::replace(escapedPath.begin(), escapedPath.end(), '\\', '/');
    
    // If path contains spaces or special characters, wrap in quotes
    bool needsQuotes = false;
    const std::string specialChars = " ()<>&|;\"'`${}[]?*~!#";
    for (char c : specialChars) {
        if (escapedPath.find(c) != std::string::npos) {
            needsQuotes = true;
            break;
        }
    }
    
    if (needsQuotes) {
        // Escape existing quotes and wrap in quotes
        std::string::size_type pos = 0;
        while ((pos = escapedPath.find('\"', pos)) != std::string::npos) {
            escapedPath.replace(pos, 1, "\\\"");
            pos += 2;
        }
        escapedPath = "\"" + escapedPath + "\"";
    }
    
    return escapedPath;
}

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

    // Initialize UI components
    m_repoSelector = std::make_unique<ModelRepoSelector>();
    m_fileSelector = std::make_unique<ModelFileSelector>();
    m_chatInterface = std::make_unique<ChatInterface>(
        std::shared_ptr<KolosalServerClient>(m_serverClient.get(), [](KolosalServerClient*){}),
        std::shared_ptr<CommandManager>(m_commandManager.get(), [](CommandManager*){})
    );

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

std::string KolosalCLI::parseRepositoryInput(const std::string &input) {
    return m_repoSelector->parseRepositoryInput(input);
}

bool KolosalCLI::isDirectGGUFUrl(const std::string &input) {
    return m_repoSelector->isDirectGGUFUrl(input);
}

bool KolosalCLI::isLocalGGUFPath(const std::string &input) {
    // Check if it's a file path (contains file separators or has .gguf extension)
    if (input.find(".gguf") != std::string::npos) {
        // Check if it's not a URL (doesn't start with http/https)
        if (input.find("http://") != 0 && input.find("https://") != 0) {
            return true;
        }
    }
    
    // Also check for common path patterns on both Windows and Linux
    if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos) {
        // Check if it's not a URL
        if (input.find("http://") != 0 && input.find("https://") != 0) {
            // Additional check for potential GGUF files even without extension
            std::filesystem::path path(input);
            if (std::filesystem::exists(path) && path.extension() == ".gguf") {
                return true;
            }
        }
    }
    
    return false;
}

bool KolosalCLI::handleDirectGGUFUrl(const std::string &url) {
    // Use file selector to handle the GGUF URL processing and get model info
    ModelFile modelFile = m_fileSelector->handleDirectGGUFUrl(url);
    
    if (modelFile.filename.empty()) {
        std::cout << "Failed to process GGUF file." << std::endl;
        return false;
    }

    // Ask for confirmation
    std::cout << "Download this model? (y/n): ";
    char choice;
    std::cin >> choice;

    if (choice == 'y' || choice == 'Y') {
        // Ensure server is running and can be connected to
        if (!ensureServerConnection()) {
            std::cerr << "Unable to connect to Kolosal server. Download cancelled." << std::endl;
            return false;
        }

        // Generate simplified engine ID from filename
        std::string engineId = modelFile.filename;
        size_t dotPos = engineId.find_last_of('.');
        if (dotPos != std::string::npos) {
            engineId = engineId.substr(0, dotPos);
        }

        // Check if engine already exists before attempting download
        if (m_serverClient->engineExists(engineId)) {
            std::cout << "\n✓ Engine '" << engineId << "' already exists on the server." << std::endl;
            std::cout << "✓ Model is ready to use!" << std::endl;
            return true;
        }

        // Process download through server
        if (!m_serverClient->addEngine(engineId, url, "./models/" + modelFile.filename)) {
            std::cerr << "Failed to start download." << std::endl;
            return false;
        }

        m_activeDownloads.push_back(engineId);

        std::cout << "\n✓ Download started successfully!" << std::endl;
        std::cout << "Engine ID: " << engineId << std::endl;
        std::cout << "Downloading to: ./models/" << modelFile.filename << std::endl;
        std::cout << "\nDownload initiated. Check server logs for progress." << std::endl;
        
        // Update config.yaml to persist the model across server restarts
        std::string localPath = "./models/" + modelFile.filename;
        updateConfigWithModel(engineId, localPath, false); // load_immediately = false for lazy loading

        return true;
    } else {
        std::cout << "Download cancelled." << std::endl;
        return false;
    }
}

bool KolosalCLI::handleLocalGGUFPath(const std::string &path) {
    // Normalize path separators for cross-platform compatibility
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    
    // Create filesystem path object for proper handling
    std::filesystem::path filePath(normalizedPath);
    
    // Check if the file exists
    if (!std::filesystem::exists(filePath)) {
        std::cout << "Error: File not found: " << normalizedPath << std::endl;
        return false;
    }
    
    // Check if it's actually a file (not a directory)
    if (!std::filesystem::is_regular_file(filePath)) {
        std::cout << "Error: Path is not a regular file: " << normalizedPath << std::endl;
        return false;
    }
    
    // Check if it has .gguf extension
    if (filePath.extension() != ".gguf") {
        std::cout << "Error: File does not have .gguf extension: " << normalizedPath << std::endl;
        return false;
    }
    
    // Get absolute path and escape it properly
    std::string absolutePath;
    std::string escapedPath;
    try {
        absolutePath = std::filesystem::absolute(filePath).string();
        // Normalize path separators for the absolute path too
        std::replace(absolutePath.begin(), absolutePath.end(), '\\', '/');
        escapedPath = escapeFilePath(absolutePath);
    } catch (const std::exception &e) {
        std::cout << "Error: Failed to get absolute path for: " << normalizedPath << " - " << e.what() << std::endl;
        return false;
    }
    
    // Extract filename for engine ID
    std::string filename = filePath.filename().string();
    std::string engineId = filename;
    size_t dotPos = engineId.find_last_of('.');
    if (dotPos != std::string::npos) {
        engineId = engineId.substr(0, dotPos);
    }
    
    std::cout << "\nLocal GGUF file detected: " << filename << std::endl;
    std::cout << "File path: " << absolutePath << std::endl;
    std::cout << "Engine ID: " << engineId << std::endl;
    
    // Ask for confirmation
    std::cout << "\nLoad this model? (y/n): ";
    char choice;
    std::cin >> choice;
    
    if (choice != 'y' && choice != 'Y') {
        std::cout << "Model loading cancelled." << std::endl;
        return false;
    }
    
    // Ensure server is running and can be connected to
    if (!ensureServerConnection()) {
        std::cerr << "Unable to connect to Kolosal server. Model loading cancelled." << std::endl;
        return false;
    }
    
    // Check if engine already exists
    if (m_serverClient->engineExists(engineId)) {
        std::cout << "\n✓ Engine '" << engineId << "' already exists on the server." << std::endl;
        std::cout << "✓ Model is ready to use!" << std::endl;
        
        // Start chat interface directly
        std::cout << "\nStarting chat interface..." << std::endl;
        startChatInterface(engineId);
        return true;
    }
    
    // Add engine to server with the local file path (use absolute path for server)
    std::cout << "\nRegistering model with server..." << std::endl;
    if (!m_serverClient->addEngine(engineId, absolutePath, absolutePath)) {
        std::cerr << "Failed to register model with server." << std::endl;
        return false;
    }
    
    std::cout << "✓ Model registered successfully with server." << std::endl;
    
    // Update config.yaml to persist the model across server restarts
    updateConfigWithModel(engineId, absolutePath, false); // load_immediately = false for lazy loading
    
    // Start chat interface
    std::cout << "\n🎉 Model loaded and registered successfully!" << std::endl;
    std::cout << "Starting chat interface..." << std::endl;
    startChatInterface(engineId);
    
    return true;
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
        // Check if it's a local GGUF file path first
        if (isLocalGGUFPath(repoId))
        {
            // Handle local GGUF file path
            bool success = handleLocalGGUFPath(repoId);
            return success ? 0 : 1;
        }
        
        std::string modelId = m_repoSelector->parseRepositoryInput(repoId);
        if (modelId.empty())
        {
            std::cout << "Invalid repository URL or ID format.\n\n";
            std::cout << "Valid formats:\n";
            std::cout << "  • owner/model-name (e.g., microsoft/DialoGPT-medium)\n";
            std::cout << "  • https://huggingface.co/owner/model-name\n";
            std::cout << "  • Direct GGUF file URL (e.g., https://huggingface.co/owner/model/resolve/main/model.gguf)\n";
            std::cout << "  • Local GGUF file path (e.g., /path/to/model.gguf)\n";
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

            // Use file selector to get the model file
            ModelFile selectedFile = m_fileSelector->selectModelFile(modelId);

            if (selectedFile.filename.empty())
            {
                std::cout << "Selection cancelled." << std::endl;
                return 0;
            }
            else
            {
                // Process download through server
                bool success = processModelDownload(modelId, selectedFile);
                return success ? 0 : 1;
            }
        }
    }

    // Standard flow: browse kolosal models
    while (true)
    {
        // Get available model IDs from config
        std::vector<std::string> availableModels = getAvailableModelIds();
        
        std::string selectedModel = m_repoSelector->selectModel(availableModels);

        if (selectedModel.empty())
        {
            std::cout << "Model selection cancelled." << std::endl;
            return 0;
        }

        // Check if it's a local model from config
        if (selectedModel.find("LOCAL:") == 0) {
            std::string modelId = selectedModel.substr(6); // Remove "LOCAL:" prefix
            
            // Ensure server connection
            if (!ensureServerConnection()) {
                std::cerr << "Unable to connect to Kolosal server." << std::endl;
                return 1;
            }
            
            // Check if engine already exists and start chat
            if (m_serverClient->engineExists(modelId)) {
                std::cout << "✓ Model '" << modelId << "' is ready to use!" << std::endl;
                std::cout << "\nStarting chat interface..." << std::endl;
                startChatInterface(modelId);
                return 0;
            } else {
                std::cout << "⚠️ Model '" << modelId << "' is in config but not loaded on server." << std::endl;
                std::cout << "Please restart the server or manually load the model." << std::endl;
                return 1;
            }
        }

        ModelFile selectedFile = m_fileSelector->selectModelFile(selectedModel);

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

bool KolosalCLI::startChatInterface(const std::string& engineId) {
    return m_chatInterface->startChatInterface(engineId);
}

std::vector<std::string> KolosalCLI::getAvailableModelIds() {
    std::vector<std::string> modelIds;
    
    try {
        std::string configPath = "config.yaml";
        
        // Check if config file exists
        if (!std::filesystem::exists(configPath)) {
            return modelIds; // Return empty vector if no config file
        }
        
        // Load the config file
        YAML::Node config = YAML::LoadFile(configPath);
        
        // Check if models section exists
        if (!config["models"] || !config["models"].IsSequence()) {
            return modelIds; // Return empty vector if no models section
        }
        
        // Extract model IDs
        for (const auto& model : config["models"]) {
            if (model["id"]) {
                std::string modelId = model["id"].as<std::string>();
                modelIds.push_back(modelId);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading config file: " << e.what() << std::endl;
    }
    
    return modelIds;
}
