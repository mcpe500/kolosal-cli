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
    std::cout << "Usage: " << programName << " [options] [repository_url_or_id_or_file_path]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h        Show this help message\n";
    std::cout << "  --stop-server     Stop the background Kolosal server\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                                    # Browse all kolosal models\n";
    std::cout << "  " << programName << " microsoft/DialoGPT-medium          # Direct access to model\n";
    std::cout << "  " << programName << " https://huggingface.co/microsoft/DialoGPT-medium\n";
    std::cout << "  " << programName << " /path/to/model.gguf                # Load local GGUF file\n";
    std::cout << "  " << programName << " ./models/my-model.gguf             # Load local GGUF file (relative path)\n";
    std::cout << "  " << programName << " --stop-server                      # Stop the background server\n";
    std::cout << "\nArguments:\n";
    std::cout << "  repository_url_or_id_or_file_path  Hugging Face repository URL/ID or local GGUF file path\n";
}

int main(int argc, char* argv[]) {
    KolosalCLI app;
    
    std::string repoId;
    bool shouldStopServer = false;
    
    // Parse command line arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (std::string(argv[1]) == "--stop-server") {
            shouldStopServer = true;
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
    
    app.initialize();
    int result = app.run(repoId);
    app.cleanup();
    
    return result;
}