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
#include <map>
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
static std::string escapeFilePath(const std::string &path)
{
    std::string escapedPath = path;

    // Replace backslashes with forward slashes for consistency (works on both Windows and Linux)
    std::replace(escapedPath.begin(), escapedPath.end(), '\\', '/');

    // If path contains spaces or special characters, wrap in quotes
    bool needsQuotes = false;
    const std::string specialChars = " ()<>&|;\"'`${}[]?*~!#";
    for (char c : specialChars)
    {
        if (escapedPath.find(c) != std::string::npos)
        {
            needsQuotes = true;
            break;
        }
    }

    if (needsQuotes)
    {
        // Escape existing quotes and wrap in quotes
        std::string::size_type pos = 0;
        while ((pos = escapedPath.find('\"', pos)) != std::string::npos)
        {
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
        std::shared_ptr<KolosalServerClient>(m_serverClient.get(), [](KolosalServerClient *) {}),
        std::shared_ptr<CommandManager>(m_commandManager.get(), [](CommandManager *) {}));

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
    showWelcome();

    if (!m_serverClient)
    {
        std::cerr << "Error: Server client not initialized\n";
        return false;
    }

    std::cout << "Stopping Kolosal server...\n";

    // Try to stop the server
    if (m_serverClient->shutdownServer())
    {
        std::cout << "Server stopped successfully\n";
        return true;
    }

    std::cerr << "Failed to stop server\n";
    return false;
}

void KolosalCLI::showWelcome()
{
    // ANSI color codes for gradient effect (cyan to blue to purple)
    const std::string colors[] = {
        "\033[38;5;51m", // Bright cyan
        "\033[38;5;45m", // Cyan
        "\033[38;5;39m", // Light blue
        "\033[38;5;33m", // Blue
        "\033[38;5;27m", // Dark blue
        "\033[38;5;21m", // Blue-purple
        "\033[38;5;57m", // Purple
        "\033[38;5;93m"  // Dark purple
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

    // Check if engine already exists before attempting download
    if (m_serverClient->engineExists(engineId))
    {
        std::cout << "Engine '" << engineId << "' already exists on the server." << std::endl;
        std::cout << "Model is ready to use!" << std::endl;

        // Start chat interface directly
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
                std::cout << "Model file already exists locally. Registering engine..." << std::endl;
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
                std::cout << "\rDownload complete. Registering engine...                                      " << std::endl;
            }
            else if (status == "engine_created")
            {
                std::cout << "Engine registered successfully." << std::endl;
            }
            else if (status == "completed")
            {
                std::cout << "Process completed." << std::endl;
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
        std::cout << "Model ready for inference." << std::endl;

        // Update config.yaml to persist the model across server restarts
        std::string localPath = "./models/" + modelFile.filename;
        updateConfigWithModel(engineId, localPath, false); // load_immediately = false for lazy loading

        // Start chat interface
        std::cout << "\nModel downloaded and registered successfully!" << std::endl;
        startChatInterface(engineId);
    }
    else
    {
        std::cout << "Download failed." << std::endl;
    }

    return downloadSuccess;
}

std::string KolosalCLI::parseRepositoryInput(const std::string &input)
{
    return m_repoSelector->parseRepositoryInput(input);
}

bool KolosalCLI::isDirectGGUFUrl(const std::string &input)
{
    return m_repoSelector->isDirectGGUFUrl(input);
}

bool KolosalCLI::isLocalGGUFPath(const std::string &input)
{
    // Check if it's a file path (contains file separators or has .gguf extension)
    if (input.find(".gguf") != std::string::npos)
    {
        // Check if it's not a URL (doesn't start with http/https)
        if (input.find("http://") != 0 && input.find("https://") != 0)
        {
            return true;
        }
    }

    // Also check for common path patterns on both Windows and Linux
    if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos)
    {
        // Check if it's not a URL
        if (input.find("http://") != 0 && input.find("https://") != 0)
        {
            // Additional check for potential GGUF files even without extension
            std::filesystem::path path(input);
            if (std::filesystem::exists(path) && path.extension() == ".gguf")
            {
                return true;
            }
        }
    }

    return false;
}

bool KolosalCLI::handleDirectGGUFUrl(const std::string &url)
{
    // Use file selector to handle the GGUF URL processing and get model info
    ModelFile modelFile = m_fileSelector->handleDirectGGUFUrl(url);

    if (modelFile.filename.empty())
    {
        std::cout << "Failed to process GGUF file." << std::endl;
        return false;
    }

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
        std::string engineId = modelFile.filename;
        size_t dotPos = engineId.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            engineId = engineId.substr(0, dotPos);
        }

        // Check if engine already exists before attempting download
        if (m_serverClient->engineExists(engineId))
        {
            std::cout << "\nEngine '" << engineId << "' already exists on the server." << std::endl;
            std::cout << "Model is ready to use!" << std::endl;
            return true;
        }

        // Process download through server
        if (!m_serverClient->addEngine(engineId, url, "./models/" + modelFile.filename))
        {
            std::cerr << "Failed to start download." << std::endl;
            return false;
        }

        m_activeDownloads.push_back(engineId);

        std::cout << "\nDownload started successfully!" << std::endl;

        // Update config.yaml to persist the model across server restarts
        std::string localPath = "./models/" + modelFile.filename;
        updateConfigWithModel(engineId, localPath, false); // load_immediately = false for lazy loading

        return true;
    }
    else
    {
        std::cout << "Download cancelled." << std::endl;
        return false;
    }
}

bool KolosalCLI::handleLocalGGUFPath(const std::string &path)
{
    // Normalize path separators for cross-platform compatibility
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    // Create filesystem path object for proper handling
    std::filesystem::path filePath(normalizedPath);

    // Check if the file exists
    if (!std::filesystem::exists(filePath))
    {
        std::cout << "Error: File not found: " << normalizedPath << std::endl;
        return false;
    }

    // Check if it's actually a file (not a directory)
    if (!std::filesystem::is_regular_file(filePath))
    {
        std::cout << "Error: Path is not a regular file: " << normalizedPath << std::endl;
        return false;
    }

    // Check if it has .gguf extension
    if (filePath.extension() != ".gguf")
    {
        std::cout << "Error: File does not have .gguf extension: " << normalizedPath << std::endl;
        return false;
    }

    // Get absolute path and escape it properly
    std::string absolutePath;
    std::string escapedPath;
    try
    {
        absolutePath = std::filesystem::absolute(filePath).string();
        // Normalize path separators for the absolute path too
        std::replace(absolutePath.begin(), absolutePath.end(), '\\', '/');
        escapedPath = escapeFilePath(absolutePath);
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: Failed to get absolute path for: " << normalizedPath << " - " << e.what() << std::endl;
        return false;
    }

    // Extract filename for engine ID
    std::string filename = filePath.filename().string();
    std::string engineId = filename;
    size_t dotPos = engineId.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        engineId = engineId.substr(0, dotPos);
    }

    std::cout << "\nLocal GGUF file detected: " << filename << std::endl;

    // Ask for confirmation
    std::cout << "\nLoad this model? (y/n): ";
    char choice;
    std::cin >> choice;

    if (choice != 'y' && choice != 'Y')
    {
        std::cout << "Model loading cancelled." << std::endl;
        return false;
    }

    // Ensure server is running and can be connected to
    if (!ensureServerConnection())
    {
        std::cerr << "Unable to connect to Kolosal server. Model loading cancelled." << std::endl;
        return false;
    }

    // Check if engine already exists
    if (m_serverClient->engineExists(engineId))
    {
        std::cout << "\nEngine '" << engineId << "' already exists on the server." << std::endl;
        std::cout << "Model is ready to use!" << std::endl;

        // Start chat interface directly
        startChatInterface(engineId);
        return true;
    }

    // Add engine to server with the local file path (use absolute path for server)
    if (!m_serverClient->addEngine(engineId, absolutePath, absolutePath))
    {
        std::cerr << "Failed to register model with server." << std::endl;
        return false;
    }

    std::cout << "Model registered successfully with server." << std::endl;

    // Update config.yaml to persist the model across server restarts
    updateConfigWithModel(engineId, absolutePath, false); // load_immediately = false for lazy loading

    // Start chat interface
    std::cout << "\nModel loaded and registered successfully!" << std::endl;
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
        if (selectedModel.find("LOCAL:") == 0)
        {
            std::string modelId = selectedModel.substr(6); // Remove "LOCAL:" prefix

            // Ensure server connection
            if (!ensureServerConnection())
            {
                std::cerr << "Unable to connect to Kolosal server." << std::endl;
                return 1;
            }

            // Check if engine already exists and start chat
            if (m_serverClient->engineExists(modelId))
            {
                std::cout << "Model '" << modelId << "' is ready to use!" << std::endl;
                startChatInterface(modelId);
                return 0;
            }

            // Check if model is currently downloading
            long long downloadedBytes, totalBytes;
            double percentage;
            std::string status;

            if (m_serverClient->getDownloadProgress(modelId, downloadedBytes, totalBytes, percentage, status))
            {
                if (status == "downloading" || status == "creating_engine" || status == "pending")
                {
                    std::cout << "Model '" << modelId << "' is currently downloading..." << std::endl;

                    // Track this download
                    m_activeDownloads.push_back(modelId);

                    // Monitor the existing download progress
                    bool downloadSuccess = m_serverClient->monitorDownloadProgress(
                        modelId,
                        [this](double percentage, const std::string &status, long long downloadedBytes, long long totalBytes)
                        {
                            // Progress callback - update display
                            ensureConsoleEncoding(); // Ensure proper encoding before each update

                            // Handle special case where no download was needed
                            if (status == "not_found")
                            {
                                std::cout << "Model file already exists locally. Registering engine..." << std::endl;
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
                                std::cout << "\rDownload complete. Registering engine...                                      " << std::endl;
                            }
                            else if (status == "engine_created")
                            {
                                std::cout << "Engine registered successfully." << std::endl;
                            }
                            else if (status == "completed")
                            {
                                std::cout << "Process completed." << std::endl;
                            }
                        },
                        1000 // Check every 1 second
                    );

                    // Remove from active downloads list
                    auto it = std::find(m_activeDownloads.begin(), m_activeDownloads.end(), modelId);
                    if (it != m_activeDownloads.end())
                    {
                        m_activeDownloads.erase(it);
                    }

                    std::cout << std::endl; // New line after progress bar
                    if (downloadSuccess)
                    {
                        std::cout << "Model ready for inference." << std::endl;

                        // Start chat interface
                        std::cout << "\nModel download completed successfully!" << std::endl;
                        startChatInterface(modelId);
                        return 0;
                    }
                    else
                    {
                        std::cout << "Download failed." << std::endl;
                        return 1;
                    }
                }
            }

            // If we get here, the model is not downloading and doesn't exist on server
            // This could be due to a cancelled download or failed load - try to re-add it
            std::cout << "Model '" << modelId << "' is in config but not loaded on server." << std::endl;
            std::cout << "Attempting to restart model loading..." << std::endl;

            // Get model information from config to attempt re-adding
            try
            {
                std::string configPath = "config.yaml";
                if (std::filesystem::exists(configPath))
                {
                    YAML::Node config = YAML::LoadFile(configPath);
                    if (config["models"] && config["models"].IsSequence())
                    {
                        for (const auto &model : config["models"])
                        {
                            if (model["id"] && model["id"].as<std::string>() == modelId)
                            {
                                std::string modelPath = model["path"].as<std::string>();
                                std::string inferenceEngine = model["inference_engine"] ? model["inference_engine"].as<std::string>() : "llama-cpu";

                                std::cout << "Re-adding model from: " << modelPath << std::endl;

                                // Attempt to re-add the model
                                if (m_serverClient->addEngine(modelId, modelPath, modelPath, inferenceEngine))
                                {
                                    std::cout << "Model re-added successfully!" << std::endl;

                                    // If it's a URL, monitor the download progress
                                    if (modelPath.find("http://") == 0 || modelPath.find("https://") == 0)
                                    {
                                        std::cout << "Starting download monitoring..." << std::endl;

                                        // Track this download
                                        m_activeDownloads.push_back(modelId);

                                        // Monitor download progress
                                        bool downloadSuccess = m_serverClient->monitorDownloadProgress(
                                            modelId,
                                            [this](double percentage, const std::string &status, long long downloadedBytes, long long totalBytes)
                                            {
                                                ensureConsoleEncoding();

                                                if (status == "downloading" && totalBytes > 0)
                                                {
                                                    std::cout << "\r";
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
                                                    std::cout << " (" << formatFileSize(downloadedBytes) << "/" << formatFileSize(totalBytes) << ")";
                                                    std::cout.flush();
                                                }
                                                else if (status == "completing")
                                                {
                                                    std::cout << "\rDownload 100% complete. Processing...                                      " << std::endl;
                                                }
                                                else if (status == "processing")
                                                {
                                                    std::cout << "\rProcessing download. This may take a few moments...                        " << std::endl;
                                                }
                                                else if (status == "completing")
                                                {
                                                    std::cout << "\rDownload 100% complete. Processing...                                      " << std::endl;
                                                }
                                                else if (status == "processing")
                                                {
                                                    std::cout << "\rProcessing download. This may take a few moments...                        " << std::endl;
                                                }
                                                else if (status == "creating_engine")
                                                {
                                                    std::cout << "\rDownload complete. Registering engine...                                      " << std::endl;
                                                }
                                            },
                                            1000);

                                        // Remove from active downloads list
                                        auto it = std::find(m_activeDownloads.begin(), m_activeDownloads.end(), modelId);
                                        if (it != m_activeDownloads.end())
                                        {
                                            m_activeDownloads.erase(it);
                                        }

                                        std::cout << std::endl;
                                        if (downloadSuccess)
                                        {
                                            std::cout << "Model ready for inference!" << std::endl;
                                            startChatInterface(modelId);
                                            return 0;
                                        }
                                        else
                                        {
                                            std::cout << "Download failed." << std::endl;
                                            return 1;
                                        }
                                    }
                                    else
                                    {
                                        // Local file, should be ready immediately
                                        std::cout << "Model loaded successfully!" << std::endl;
                                        startChatInterface(modelId);
                                        return 0;
                                    }
                                }
                                else
                                {
                                    std::cout << "Failed to re-add model. Please check server logs for details." << std::endl;
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cout << "Error reading config: " << e.what() << std::endl;
            }

            std::cout << "Could not find model configuration or re-add failed." << std::endl;
            std::cout << "Please restart the server or manually load the model." << std::endl;
            return 1;
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

bool KolosalCLI::updateConfigWithModel(const std::string &engineId, const std::string &modelPath, bool loadImmediately)
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
        for (const auto &model : config["models"])
        {
            if (model["id"] && model["id"].as<std::string>() == engineId)
            {
                modelExists = true;
                break;
            }
        } // Only add if model doesn't already exist in config
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

bool KolosalCLI::startChatInterface(const std::string &engineId)
{
    return m_chatInterface->startChatInterface(engineId);
}

std::vector<std::string> KolosalCLI::getAvailableModelIds()
{
    std::vector<std::string> modelIds;

    try
    {
        std::string configPath = "config.yaml";

        // Check if config file exists
        if (!std::filesystem::exists(configPath))
        {
            return modelIds; // Return empty vector if no config file
        }

        // Load the config file
        YAML::Node config = YAML::LoadFile(configPath);

        // Check if models section exists
        if (!config["models"] || !config["models"].IsSequence())
        {
            return modelIds; // Return empty vector if no models section
        }

        // Extract model IDs
        for (const auto &model : config["models"])
        {
            if (model["id"])
            {
                std::string modelId = model["id"].as<std::string>();
                modelIds.push_back(modelId);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reading config file: " << e.what() << std::endl;
    }

    return modelIds;
}

bool KolosalCLI::showServerLogs()
{
    showWelcome();

    std::cout << "Retrieving server logs...\n\n";

    if (!m_serverClient)
    {
        std::cerr << "Error: Server client not initialized\n";
        return false;
    }

    // Check if server is running
    if (!m_serverClient->isServerHealthy())
    {
        std::cerr << "Error: Kolosal server is not running\n";
        std::cerr << "   Please start the server first by running a command that requires it\n";
        return false;
    }

    std::vector<std::tuple<std::string, std::string, std::string>> logs;
    if (!m_serverClient->getLogs(logs))
    {
        std::cerr << "Error: Failed to retrieve server logs\n";
        return false;
    }

    if (logs.empty())
    {
        std::cout << "No logs available\n";
        return true;
    }

    std::cout << "Server Logs (" << logs.size() << " entries):\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Display logs with color coding based on level
    for (const auto &logEntry : logs)
    {
        const std::string &level = std::get<0>(logEntry);
        const std::string &timestamp = std::get<1>(logEntry);
        const std::string &message = std::get<2>(logEntry);

        // Color code based on log level
        std::string levelColor;
        if (level == "ERROR")
        {
            levelColor = "\033[31m"; // Red
        }
        else if (level == "WARNING")
        {
            levelColor = "\033[33m"; // Yellow
        }
        else if (level == "INFO")
        {
            levelColor = "\033[32m"; // Green
        }
        else if (level == "DEBUG")
        {
            levelColor = "\033[36m"; // Cyan
        }
        else
        {
            levelColor = "\033[37m"; // White
        }

        // Reset color
        std::string resetColor = "\033[0m";

        std::cout << levelColor << "[" << level << "] " << resetColor
                  << "\033[90m" << timestamp << resetColor << "\n";
        std::cout << "   " << message << "\n\n";
    }

    return true;
}

bool KolosalCLI::showInferenceEngines()
{
    showWelcome();

    // Initialize Kolosal server first
    if (!initializeServer())
    {
        std::cerr << "Failed to initialize Kolosal server\n";
        return false;
    }

    std::cout << "Retrieving available inference engines...\n";

    if (!m_serverClient)
    {
        std::cerr << "Error: Server client not initialized\n";
        return false;
    }

    // Fetch available engines from server
    std::vector<std::tuple<std::string, std::string, std::string, std::string, bool>> serverEngines;
    if (!m_serverClient->getInferenceEngines(serverEngines))
    {
        std::cerr << "Error: Failed to retrieve inference engines from server\n";
        return false;
    }

    // Fetch available engine files from Hugging Face
    std::cout << "Fetching engine files from kolosal/engines repository...\n";
    std::vector<std::string> availableEngineFiles = HuggingFaceClient::fetchEngineFiles();

    // Create a map of server engines by name for easier lookup
    std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, bool>> serverEngineMap;
    for (const auto &engine : serverEngines)
    {
        const std::string &name = std::get<0>(engine);
        serverEngineMap[name] = engine;
    }

    // Create combined engine list with download status
    std::vector<std::tuple<std::string, std::string, bool, bool>> combinedEngines; // name, filename, isDownloaded, isLoaded

    // Add engines from Hugging Face with download status
    for (const std::string &filename : availableEngineFiles)
    {
        // Extract engine name from filename (remove extension and path)
        std::string engineName = filename;
        size_t lastSlash = engineName.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            engineName = engineName.substr(lastSlash + 1);
        }
        size_t lastDot = engineName.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            engineName = engineName.substr(0, lastDot);
        }

        // Check if this engine exists on the server
        bool isDownloaded = serverEngineMap.find(engineName) != serverEngineMap.end();
        bool isLoaded = false;
        if (isDownloaded)
        {
            isLoaded = std::get<4>(serverEngineMap[engineName]);
        }

        combinedEngines.emplace_back(engineName, filename, isDownloaded, isLoaded);
    }

    // Add any server engines that weren't found in Hugging Face (custom engines)
    for (const auto &serverEngine : serverEngines)
    {
        const std::string &name = std::get<0>(serverEngine);
        bool found = false;
        for (const auto &combined : combinedEngines)
        {
            if (std::get<0>(combined) == name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            bool isLoaded = std::get<4>(serverEngine);
            combinedEngines.emplace_back(name, "", true, isLoaded); // Custom engine, assume downloaded
        }
    }

    if (combinedEngines.empty())
    {
        std::cout << "No inference engines available\n";
        return true;
    }

    std::cout << "\n";

    // Create display items for interactive list
    std::vector<std::string> displayItems;

    for (const auto &engineEntry : combinedEngines)
    {
        const std::string &name = std::get<0>(engineEntry);
        bool isDownloaded = std::get<2>(engineEntry);
        bool isLoaded = std::get<3>(engineEntry);

        // Create display string with simplified status
        std::string displayString = name;
        if (isLoaded || isDownloaded)
        {
            displayString += " (DOWNLOADED: ready)";
        }
        else
        {
            displayString += " (NOT DOWNLOADED: available for download)";
        }

        displayItems.push_back(displayString);
    }

    // Add navigation option
    displayItems.push_back("Back to Main Menu");

    // Create and run interactive list
    InteractiveList engineList(displayItems);
    int result = engineList.run();

    if (result >= 0 && result < static_cast<int>(combinedEngines.size()))
    {
        // User selected an engine - show detailed information
        const auto &selectedEngine = combinedEngines[result];
        const std::string &name = std::get<0>(selectedEngine);
        const std::string &filename = std::get<1>(selectedEngine);
        bool isDownloaded = std::get<2>(selectedEngine);
        bool isLoaded = std::get<3>(selectedEngine);

        std::cout << "\nInference Engine Details:\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Name: " << name << "\n";
        if (!filename.empty())
        {
            std::cout << "Filename: " << filename << "\n";
        }

        if (isLoaded)
        {
            std::cout << "Status: LOADED (ready to use)\n";
        }
        else if (isDownloaded)
        {
            std::cout << "Status: DOWNLOADED (available to load)\n";
        }
        else
        {
            std::cout << "Status: NOT DOWNLOADED (available for download)\n";
            if (!filename.empty())
            {
                std::cout << "Download URL: https://huggingface.co/kolosal/engines/resolve/main/" << filename << "\n";
            }
        }

        // Show additional details if available from server
        if (isDownloaded && serverEngineMap.find(name) != serverEngineMap.end())
        {
            const auto &serverEngine = serverEngineMap[name];
            const std::string &version = std::get<1>(serverEngine);
            const std::string &description = std::get<2>(serverEngine);
            const std::string &library_path = std::get<3>(serverEngine);

            if (!version.empty())
            {
                std::cout << "Version: " << version << "\n";
            }
            if (!description.empty())
            {
                std::cout << "Description: " << description << "\n";
            }
            if (!library_path.empty())
            {
                std::cout << "Library Path: " << library_path << "\n";
            }
        }

        std::cout << std::string(50, '=') << "\n";

        // Future enhancement: Add actions like download, load/unload engine
        std::cout << "\nPress any key to continue...";
        std::cin.get();
    }

    return true;
}
