#include "chat_interface.h"
#include "interactive_list.h"
#include "loading_animation.h"
#include "command_manager.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <signal.h>
#include <csignal>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

ChatInterface::ChatInterface(std::shared_ptr<KolosalServerClient> serverClient, 
                           std::shared_ptr<CommandManager> commandManager)
    : m_serverClient(serverClient), m_commandManager(commandManager) {
}

ChatInterface::~ChatInterface() {
}

bool ChatInterface::startChatInterface(const std::string& engineId) {
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

    while (!shouldExit) {
        // Get user input with real-time autocomplete support
        std::cout << "\n";
        std::string userInput = getInputWithRealTimeAutocomplete("");

        // Check if input was cancelled (empty string returned)
        if (userInput.empty() && std::cin.eof()) {
            // Handle EOF (Ctrl+C or stream closed)
            break;
        }

        // Trim whitespace
        userInput.erase(0, userInput.find_first_not_of(" \t\n\r\f\v"));
        userInput.erase(userInput.find_last_not_of(" \t\n\r\f\v") + 1);

        // Skip empty messages
        if (userInput.empty()) {
            continue;
        }

        // Check if input is a command
        if (m_commandManager->isCommand(userInput)) {
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
        if (userInput == "exit" || userInput == "quit") {
            shouldExit = true;
            break;
        }

        // Add user message to history
        chatHistory.push_back({"user", userInput});

        // Force clear any lingering suggestions before showing AI response
        forceClearSuggestions();

        // Show loading spinner using LoadingAnimation until first model response
        std::atomic<bool> gotFirstChunk{false};
        std::atomic<bool> promptPrinted{false};
        LoadingAnimation loadingAnim("");
        std::string fullResponse;

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
        int lineCount = 0; // Track approximate line count to avoid bottom-of-terminal issues
        
        // Use safer mode on macOS to avoid terminal crashes
        bool useSimpleMode = false;
#ifdef __APPLE__
        useSimpleMode = true; // Default to simple mode on macOS to prevent crashes
#endif

#ifdef _WIN32
        // Get actual terminal dimensions on Windows
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
            terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            terminalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
#else
        // Get actual terminal dimensions on Linux/macOS
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            terminalWidth = w.ws_col;
            terminalHeight = w.ws_row;
        }
#endif
        
        bool success = m_serverClient->streamingChatCompletion(engineId, userInput,
            [&](const std::string& chunk, double tps, double timeToFirstToken) {
                // Update metrics first (before any processing)
                if (tps > 0) {
                    currentTps = tps;
                    hasMetrics = true;
                }
                if (timeToFirstToken > 0) {
                    ttft = timeToFirstToken;
                }
                
                // Add chunk to full response first
                fullResponse += chunk;
                
                // Check if this is the first time we're processing any chunk
                if (!gotFirstChunk.load()) {
                    gotFirstChunk = true;
                    // Only print prompt if we haven't already
                    if (!promptPrinted.exchange(true)) {
                        // Move to start of spinner line, clear it, then print prompt
                        loadingAnim.stop();
                        std::cout << "\r\033[32m> \033[32m";
                    }
                }
                
                // Print the chunk content directly (no need to recalculate from fullResponse)
                std::cout << chunk;
                std::cout.flush();
                
                if (!useSimpleMode) {
                    // Update cursor position tracking for the chunk content (only if not in simple mode)
                    for (char c : chunk) {
                        if (c == '\n') {
                            currentColumn = 0;
                            lineCount++; // Track line count
                        } else if (c >= 32 && c <= 126) { // Printable characters
                            currentColumn++;
                            if (currentColumn >= terminalWidth) {
                                currentColumn = 0; // Wrapped to new line
                                lineCount++; // Count wrapped lines
                            }
                        }
                    }
                    
                    // Update line state
                    bool containsNewline = chunk.find('\n') != std::string::npos;
                    lastWasNewline = containsNewline;
                } else {
                    // Count newlines in simple mode too
                    for (char c : chunk) {
                        if (c == '\n') {
                            lineCount++;
                        }
                    }
                }
                
                // Handle metrics and complex display logic (only for non-simple mode)
                if (!useSimpleMode) {
                    // Check if we need to clear previous metrics due to newline
                    bool containsNewline = chunk.find('\n') != std::string::npos;
                    if (containsNewline && metricsShown) {
                        // Clear the metrics line before processing the chunk
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
                    
                    // Update line state for metrics display logic
                    if (containsNewline || willWrap) {
                        lastWasNewline = true;
                        // Clear metrics again after processing to ensure they're gone
                        if (metricsShown) {
                            std::cout << "\033[s";  // Save cursor position
                            std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                            std::cout << "\033[u";  // Restore cursor position
                            metricsShown = false;
                        }
                    } else {
                        lastWasNewline = false;
                    }
                }
                
                // Show metrics in real-time below the response (but not immediately after newlines)
                // Skip metrics display in simple mode to avoid ANSI sequence issues on macOS
                // Also skip real-time metrics for single-line responses to avoid text duplication
                if (!useSimpleMode && hasMetrics && !lastWasNewline && lineCount > 0) {
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
                    // Simplified approach for Linux/macOS - use line counting instead of cursor queries
                    // Estimate if we're near the bottom based on line count
                    if (lineCount >= terminalHeight - 3) {
                        canShowMetricsBelow = false;
                    }
#endif
                    
                    if (canShowMetricsBelow) {
                        // Clear any existing metrics first to avoid interference
                        if (metricsShown) {
                            std::cout << "\033[s";  // Save cursor position
                            std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
                            std::cout << "\033[u";  // Restore cursor position
                            metricsShown = false;
                        }
                        
                        // For single-line responses (lineCount == 0), don't show real-time metrics
                        // to avoid cursor position conflicts that cause text duplication
                        if (lineCount > 0) {
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
                            
                            // Restore cursor position to end of response text
                            std::cout << "\033[u";
                            std::cout.flush();
                        }
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
        if (!promptPrinted.load()) {
            loadingAnim.stop();
            std::cout << "\n\033[32m> \033[32m";
            std::cout.flush();
        }

        // Clear metrics line after completion to clean up (only if not in simple mode)
        if (!useSimpleMode && metricsShown) {
            std::cout << "\033[s";  // Save cursor position
            std::cout << "\033[B\033[1G\033[2K"; // Move down, go to column 1, clear entire line
            std::cout << "\033[u";  // Restore cursor position
        }

        if (!success && fullResponse.empty()) {
            std::cout << "❌ Error: Failed to get response from the model. Please try again." << std::endl;
            continue;
        }

        // Add assistant response to history
        if (!fullResponse.empty()) {
            chatHistory.push_back({"assistant", fullResponse});
        }

        std::cout << "\033[0m";
        
        // Display final metrics below the completed response
        if (hasMetrics && (ttft > 0 || currentTps > 0)) {
            if (useSimpleMode) {
                // Simple metrics display for macOS without complex ANSI sequences
                std::cout << "\n";
                if (ttft > 0) {
                    std::cout << "TTFT: " << std::fixed << std::setprecision(2) << ttft << "ms";
                }
                if (currentTps > 0) {
                    if (ttft > 0) std::cout << " | ";
                    std::cout << "TPS: " << std::fixed << std::setprecision(1) << currentTps;
                }
            } else {
                // Full metrics display with colors for Windows/Linux
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
        }
        
        std::cout << std::endl;
    }

#ifdef _WIN32
    // Restore default console control handler
    SetConsoleCtrlHandler(nullptr, FALSE);
#endif

    return true;
}

std::string ChatInterface::getInputWithRealTimeAutocomplete(const std::string& prompt) {
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
    
    // Get terminal dimensions for boundary protection
    struct winsize w;
    int terminalHeight = 24; // Default fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminalHeight = w.ws_row;
    }
    
    // Check if we're too close to bottom for safe suggestion display
    // Add some newlines to ensure we have space if needed
    int currentLine = 1; // Assume we start somewhere in the middle
    try {
        // Simple approach: if terminal is small, add some padding
        if (terminalHeight < 15) {
            std::cout << "\n\n\n";
        }
    } catch (...) {
        // Ignore errors in terminal handling
    }
    
    // Set terminal to raw mode for character-by-character input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_iflag &= ~(IXON | ICRNL); // Disable flow control and CR-to-NL mapping
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Ensure stdout is flushed before starting input
    std::cout.flush();
    fflush(stdout);
    
    // Clear any pending input that might be in the buffer (like cursor position responses)
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while (getchar() != EOF); // Drain any pending input
    fcntl(STDIN_FILENO, F_SETFL, flags); // Restore blocking mode
    
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
                
                // Update or clear suggestions with boundary protection
                if (input.length() >= 2 && input[0] == '/') {
                    // Only show suggestions if we have enough terminal space
                    if (terminalHeight > 10) {
                        updateRealTimeSuggestionsLinux(input, showingSuggestions, suggestionStartRow, prompt);
                    }
                } else if (showingSuggestions) {
                    clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
                }
            }
        } else if (ch == 9) { // Tab - for command suggestions
            if (input.length() >= 2 && input[0] == '/' && terminalHeight > 10) {
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

void ChatInterface::updateRealTimeSuggestions(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt) {
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

void ChatInterface::updateRealTimeSuggestionsLinux(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt) {
#ifndef _WIN32
    auto suggestions = m_commandManager->getCommandSuggestions(input);
    
    if (suggestions.empty()) {
        if (showingSuggestions) {
            clearSuggestionsLinux(showingSuggestions, suggestionStartRow, prompt, input);
        }
        return;
    }
    
    // Get terminal height for boundary checking
    struct winsize w;
    int terminalHeight = 24;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminalHeight = w.ws_row;
    }
    
    // Don't show suggestions if terminal is too small
    if (terminalHeight < 10) {
        return;
    }
    
    if (!showingSuggestions) {
        showingSuggestions = true;
        suggestionStartRow = 1; // Mark that we're showing suggestions below
    }
    
    // Save current cursor position (input line)
    printf("\033[s");
    
    try {
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
    } catch (...) {
        // If anything goes wrong with ANSI sequences, just restore cursor and continue
    }
    
    // Restore cursor position to input line
    printf("\033[u");
    fflush(stdout);
#endif
}

void ChatInterface::clearSuggestions(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input) {
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

void ChatInterface::clearSuggestionsLinux(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input) {
#ifndef _WIN32
    if (!showingSuggestions) return;
    
    // Save current cursor position (input line)
    printf("\033[s");
    
    try {
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
    } catch (...) {
        // If anything goes wrong, just continue
    }
    
    // Restore cursor position to input line
    printf("\033[u");
    fflush(stdout);
    
    showingSuggestions = false;
#endif
}

void ChatInterface::clearCurrentInput(const std::string& input, bool& showingSuggestions, int suggestionStartRow, const std::string& prompt) {
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

void ChatInterface::forceClearSuggestions() {
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

void ChatInterface::displayHintText(const std::string& hintText, bool& showingHint) {
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

void ChatInterface::displayHintTextLinux(const std::string& hintText, bool& showingHint) {
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

void ChatInterface::clearHintText(bool& showingHint) {
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

void ChatInterface::clearHintTextLinux(const std::string& hintText, bool& showingHint) {
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
