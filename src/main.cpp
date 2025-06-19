#include "kolosal_cli.h"

/**
 * @file main.cpp
 * @brief Entry point for Kolosal CLI application
 * 
 * This is the main entry point for the Kolosal CLI application.
 * The actual application logic is implemented in the KolosalCLI class.
 */

int main() {
    KolosalCLI app;
    
    app.initialize();
    int result = app.run();
    app.cleanup();
    
    return result;
}