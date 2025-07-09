#include "kolosal_cli.h"
#include <iostream>

/**
 * @file main.cpp
 * @brief Entry point for Kolosal CLI application
 * 
 * This is the main entry point for the Kolosal CLI application.
 * The actual application logic is implemented in the KolosalCLI class.
 */

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [command] [repository_url_or_id_or_file_path]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  help              Show this help message\n";
    std::cout << "  stop              Stop the background Kolosal server\n";
    std::cout << "  logs              Display server logs\n";
    std::cout << "  engines           Display available inference engines\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                                    # Browse all kolosal models\n";
    std::cout << "  " << programName << " microsoft/DialoGPT-medium          # Direct access to model\n";
    std::cout << "  " << programName << " https://huggingface.co/microsoft/DialoGPT-medium\n";
    std::cout << "  " << programName << " /path/to/model.gguf                # Load local GGUF file\n";
    std::cout << "  " << programName << " ./models/my-model.gguf             # Load local GGUF file (relative path)\n";
    std::cout << "  " << programName << " stop                               # Stop the background server\n";
    std::cout << "  " << programName << " logs                               # Display server logs\n";
    std::cout << "  " << programName << " engines                            # Display available inference engines\n";
    std::cout << "\nArguments:\n";
    std::cout << "  repository_url_or_id_or_file_path  Hugging Face repository URL/ID or local GGUF file path\n";
}

int main(int argc, char* argv[]) {
    KolosalCLI app;
    
    std::string repoId;
    bool shouldStopServer = false;
    bool shouldShowLogs = false;
    bool shouldShowEngines = false;
    
    // Parse command line arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "help") {
            printUsage(argv[0]);
            return 0;
        } else if (std::string(argv[1]) == "stop") {
            shouldStopServer = true;
        } else if (std::string(argv[1]) == "logs") {
            shouldShowLogs = true;
        } else if (std::string(argv[1]) == "engines") {
            shouldShowEngines = true;
        } else {
            repoId = argv[1];
        }
    }
    
    // Handle server stop request
    if (shouldStopServer) {
        app.initialize();
        bool success = app.stopBackgroundServer();
        app.cleanup();
        return success ? 0 : 1;
    }
    
    // Handle logs request
    if (shouldShowLogs) {
        app.initialize();
        bool success = app.showServerLogs();
        app.cleanup();
        return success ? 0 : 1;
    }
    
    // Handle engines request
    if (shouldShowEngines) {
        app.initialize();
        bool success = app.showInferenceEngines();
        app.cleanup();
        return success ? 0 : 1;
    }
    
    app.initialize();
    int result = app.run(repoId);
    app.cleanup();
    
    return result;
}