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
    std::cout << "Usage: " << programName << " [repository_url_or_id]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                                    # Browse all kolosal models\n";
    std::cout << "  " << programName << " microsoft/DialoGPT-medium          # Direct access to model\n";
    std::cout << "  " << programName << " https://huggingface.co/microsoft/DialoGPT-medium\n";
    std::cout << "\nArguments:\n";
    std::cout << "  repository_url_or_id  Hugging Face repository URL or ID (e.g., owner/model-name)\n";
}

int main(int argc, char* argv[]) {
    KolosalCLI app;
    
    std::string repoId;
    
    // Parse command line arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        repoId = argv[1];
    }
    
    app.initialize();
    int result = app.run(repoId);
    app.cleanup();
    
    return result;
}