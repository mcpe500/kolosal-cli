#include "interactive_list.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif

InteractiveList::InteractiveList(const std::vector<std::string> &listItems)
    : items(listItems), filteredItems(listItems), searchQuery(""), selectedIndex(0),
      viewportTop(0), maxDisplayItems(20), isSearchMode(false)
{
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
#else
    // Get terminal size
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    terminalWidth = w.ws_col;
    terminalHeight = w.ws_row;
    
    // Save original terminal settings
    tcgetattr(STDIN_FILENO, &originalTermios);
#endif

    // Calculate max display items dynamically based on actual content
    maxDisplayItems = calculateMaxDisplayItems();
}

void InteractiveList::hideCursor()
{
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    std::cout << "\033[?25l" << std::flush;
#endif
}

void InteractiveList::showCursor()
{
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    std::cout << "\033[?25h" << std::flush;
#endif
}

void InteractiveList::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    int result = system("clear");
    (void)result; // Suppress unused result warning
#endif
}

void InteractiveList::moveCursor(int x, int y)
{
#ifdef _WIN32
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
#endif
}

void InteractiveList::setColor(int color)
{
#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, color);
#else
    // Convert Windows color codes to ANSI escape sequences
    // Handle combined colors (bitwise OR operations)
    if (color == (BACKGROUND_GREEN | FOREGROUND_INTENSITY)) {
        // Green background with bright text (search mode)
        std::cout << "\033[42;97m";  // Green background, bright white text
    } else if (color == (BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)) {
        // Blue background with white text (highlighted selection)
        std::cout << "\033[44;97m";  // Blue background, bright white text
    } else if (color == FOREGROUND_INTENSITY) {
        // Bright/intense foreground
        std::cout << "\033[90m";     // Dark gray (dim)
    } else if (color == 112) {
        // Black on white (highlighted) - original Windows value
        std::cout << "\033[47;30m";
    } else {
        // Handle individual colors
        switch(color) {
            case 7:  // White on black (default)
                std::cout << "\033[0m";
                break;
            case 14: // Yellow
                std::cout << "\033[93m";
                break;
            case 10: // Green
                std::cout << "\033[92m";
                break;
            case 12: // Red
                std::cout << "\033[91m";
                break;
            case 9:  // Blue
                std::cout << "\033[94m";
                break;
            default:
                std::cout << "\033[0m";
                break;
        }
    }
    std::cout << std::flush;
#endif
}

void InteractiveList::resetColor()
{
#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, csbi.wAttributes);
#else
    std::cout << "\033[0m" << std::flush;
#endif
}

void InteractiveList::updateViewport()
{
    // Adjust viewport to keep selected item visible
    if (selectedIndex < viewportTop)
    {
        viewportTop = selectedIndex;
    }
    else if (selectedIndex >= viewportTop + maxDisplayItems)
    {
        viewportTop = selectedIndex - maxDisplayItems + 1;
    }

    // Ensure viewport doesn't go beyond bounds
    if (viewportTop + maxDisplayItems > filteredItems.size())
    {
        if (filteredItems.size() >= maxDisplayItems)
        {
            viewportTop = filteredItems.size() - maxDisplayItems;
        }
        else
        {
            viewportTop = 0;
        }
    }
}

void InteractiveList::applyFilter()
{
    filteredItems.clear();
    selectedIndex = 0;
    viewportTop = 0;

    if (searchQuery.empty())
    {
        filteredItems = items;
    }
    else
    {
        std::string lowerQuery = searchQuery;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        for (const auto &item : items)
        {
            std::string lowerItem = item;
            std::transform(lowerItem.begin(), lowerItem.end(), lowerItem.begin(), ::tolower);

            if (lowerItem.find(lowerQuery) != std::string::npos)
            {
                filteredItems.push_back(item);
            }
        }
    }
    
    // Recalculate max display items based on the new filtered set
    maxDisplayItems = calculateMaxDisplayItems();
}

int InteractiveList::calculateItemLines(const std::string& item) {
    // Check if item has memory info (format: "... [Memory: info]")
    size_t memoryStart = item.find(" [Memory: ");
    if (memoryStart != std::string::npos) {
        size_t memoryEnd = item.find("]", memoryStart);
        if (memoryEnd != std::string::npos) {
            return 2; // Filename line + memory line
        }
    }
    return 1; // Just filename line
}

size_t InteractiveList::calculateMaxDisplayItems() {
#ifdef _WIN32
    int totalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    // Update terminal size for Linux
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int totalHeight = w.ws_row;
#endif
    
    // Calculate reserved lines more precisely:
    // 1. Title (1 line)
    // 2. Instructions (1 line) 
    // 3. Search instructions with double newline (2 lines)
    // 4. Search bar (1 line)
    // 5. Empty line after search (1 line)
    // 6. Potential scroll indicator above (1 line max)
    // 7. Potential scroll indicator below (1 line max)
    // 8. Empty line before selection info (1 line)
    // 9. Selection info (1 line)
    // Total: 10 lines reserved
    int reservedLines = 10;
    
    int availableHeight = totalHeight - reservedLines;
    
    if (availableHeight <= 3) {
        return 2; // Minimum usable amount
    }
    
    // If we have no items yet, use a reasonable default
    if (filteredItems.empty()) {
        return static_cast<size_t>(availableHeight / 2); // Assume average 2 lines per item
    }
      // Calculate based on actual items in the current filtered set
    // Sample the first several items to estimate average lines per item
    int sampleSize = (static_cast<int>(filteredItems.size()) < 5) ? 
                     static_cast<int>(filteredItems.size()) : 5;
    int totalSampleLines = 0;
    
    for (int i = 0; i < sampleSize; ++i) {
        totalSampleLines += calculateItemLines(filteredItems[i]);
    }
    
    double avgLinesPerItem = static_cast<double>(totalSampleLines) / sampleSize;
    size_t maxItems = static_cast<size_t>(availableHeight / avgLinesPerItem);
    
    // Ensure we have at least a few items visible
    return (maxItems > 2) ? maxItems : 2;
}

void InteractiveList::displayList()
{
    clearScreen();

    // Display title
    std::cout << "Kolosal CLI - Select Model\n";
    std::cout << "Use UP/DOWN arrows to navigate, ENTER to select, ESC or Ctrl+C to exit\n";
    std::cout << "Press '/' to search, BACKSPACE to clear search\n\n";

    // Display search bar
    if (isSearchMode)
    {
        setColor(BACKGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << "Search: " << searchQuery << "_" << std::endl;
        resetColor();
    }
    else
    {
        std::cout << "Search: " << searchQuery;
        if (!searchQuery.empty())
        {
            setColor(FOREGROUND_INTENSITY);
            std::cout << " (Press '/' to edit)";
            resetColor();
        }
        else
        {
            setColor(FOREGROUND_INTENSITY);
            std::cout << " (Press '/' to search)";
            resetColor();
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    if (filteredItems.empty())
    {
        if (searchQuery.empty())
        {
            std::cout << "No models available.\n";
        }
        else
        {
            std::cout << "No models found matching: \"" << searchQuery << "\"\n";
        }
        return;
    }

    updateViewport();

    // Calculate display range
    size_t displayStart = viewportTop;
    size_t displayEnd = (viewportTop + maxDisplayItems < filteredItems.size()) ? viewportTop + maxDisplayItems : filteredItems.size();

    // Show scroll indicator at top if there are items above
    if (viewportTop > 0)
    {
        setColor(FOREGROUND_INTENSITY);
        std::cout << "  ... " << viewportTop << " more above\n";
        resetColor();
    }    // Display visible items
    for (size_t i = displayStart; i < displayEnd; ++i)
    {
        std::string item = filteredItems[i];
        std::string filename = item;
        std::string quantDescription = "";
        std::string memoryInfo = "";

        // Parse memory info if present (format: "... [Memory: info]")
        size_t memoryStart = item.find(" [Memory: ");
        if (memoryStart != std::string::npos)
        {
            size_t memoryEnd = item.find("]", memoryStart);
            if (memoryEnd != std::string::npos)
            {
                memoryInfo = item.substr(memoryStart + 10, memoryEnd - memoryStart - 10); // Extract content after " [Memory: "
                item = item.substr(0, memoryStart) + item.substr(memoryEnd + 1); // Remove memory part for further parsing
            }
        }

        // Parse quantization info if present (format: "filename (TYPE: description)")
        size_t parenPos = item.find(" (");
        if (parenPos != std::string::npos && item.back() == ')')
        {
            filename = item.substr(0, parenPos);
            size_t colonPos = item.find(": ", parenPos);
            if (colonPos != std::string::npos)
            {
                quantDescription = item.substr(colonPos + 2);
                quantDescription = quantDescription.substr(0, quantDescription.length() - 1); // Remove closing paren
            }
        }        if (i == selectedIndex)
        {
            // Highlight selected item
            setColor(BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            std::cout << "> ";

            // Print filename with fixed width for alignment
            std::cout << std::left << std::setw(50) << filename;
            resetColor();

            // Add quantization description with softer color if present
            if (!quantDescription.empty())
            {
                setColor(FOREGROUND_INTENSITY); // Softer gray color
                std::cout << " " << quantDescription;
                resetColor();
            }
            std::cout << std::endl;
            
            // Show memory info on next line if available
            if (!memoryInfo.empty())
            {
                setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Green color for memory info
                std::cout << "    Memory: " << memoryInfo << std::endl;
                resetColor();
            }
        }
        else
        {
            std::cout << "  ";

            // Print filename with fixed width for alignment
            std::cout << std::left << std::setw(50) << filename;

            // Add quantization description with softer color if present
            if (!quantDescription.empty())
            {
                setColor(FOREGROUND_INTENSITY); // Softer gray color
                std::cout << " " << quantDescription;
                resetColor();
            }
            std::cout << std::endl;
            
            // Show memory info on next line if available (dimmer for non-selected items)
            if (!memoryInfo.empty())
            {
                setColor(FOREGROUND_INTENSITY); // Dimmer gray for non-selected items
                std::cout << "    Memory: " << memoryInfo << std::endl;
                resetColor();
            }
        }
    }

    // Show scroll indicator at bottom if there are items below
    if (displayEnd < filteredItems.size())
    {
        setColor(FOREGROUND_INTENSITY);
        std::cout << "  ... " << (filteredItems.size() - displayEnd) << " more below\n";
        resetColor();
    }

    // Display current selection info
    if (!filteredItems.empty())
    {
        std::string selectedItem = filteredItems[selectedIndex];
        std::string selectedFilename = selectedItem;

        // Extract just the filename for display
        size_t parenPos = selectedItem.find(" (");
        if (parenPos != std::string::npos)
        {
            selectedFilename = selectedItem.substr(0, parenPos);
        }

        std::cout << "\nSelected: " << selectedFilename;
        std::cout << " (" << (selectedIndex + 1) << "/" << filteredItems.size() << ")";
        if (!searchQuery.empty())
        {
            std::cout << " | Filtered from " << items.size() << " total";
        }
        std::cout << std::endl;
    }
}

int InteractiveList::run()
{
    // Use the update version with a no-op callback
    return runWithUpdates([]() { return false; });
}

int InteractiveList::runWithUpdates(std::function<bool()> updateCallback)
{
    hideCursor();

    // Handle empty list case
    if (items.empty())
    {
        displayList();
        std::cout << "\nPress any key to exit...";
#ifdef _WIN32
        _getch();
#else
        getch();
#endif
        showCursor();
        return -1;
    }

    while (true)
    {
        displayList();

        // Check for keyboard input with timeout
        bool hasInput = false;
        int key = 0;
        
        // Check for input periodically, allowing for updates
#ifdef _WIN32
        auto startTime = GetTickCount();
        const DWORD UPDATE_INTERVAL_MS = 200; // Check for updates every 200ms
#else
        auto startTime = getTickCount();
        const unsigned long UPDATE_INTERVAL_MS = 200; // Check for updates every 200ms
#endif
        
        while (true)
        {
            // Check if key is available without blocking
#ifdef _WIN32
            if (_kbhit())
            {
                key = _getch();
                hasInput = true;
                break;
            }
#else
            if (kbhit())
            {
                key = getch();
                hasInput = true;
                break;
            }
#endif
            
            // Check if it's time for an update
#ifdef _WIN32
            DWORD currentTime = GetTickCount();
#else
            unsigned long currentTime = getTickCount();
#endif
            if (currentTime - startTime >= UPDATE_INTERVAL_MS)
            {
                // Call update callback and refresh display if needed
                if (updateCallback())
                {
                    // Update occurred, redraw screen and reset timer
                    break; // Exit input loop to redraw
                }
                startTime = currentTime; // Reset timer
            }
            
            // Small sleep to prevent busy waiting
#ifdef _WIN32
            Sleep(10);
#else
            msleep(10);
#endif
        }
        
        // If no input was received, continue to redraw
        if (!hasInput)
        {
            continue;
        }

        // Handle special keys (arrow keys)
#ifdef _WIN32
        if (key == 224)
        { // Special key prefix on Windows
            key = _getch();
            switch (key)
            {
            case 72: // Up arrow
                if (isSearchMode)
                {
                    // Do nothing when in search mode and pressing up
                }
                else if (!filteredItems.empty())
                {
                    if (selectedIndex > 0)
                    {
                        selectedIndex--;
                    }
                    else
                    {
                        // When at top of list, go to search input
                        isSearchMode = true;
                    }
                }
                else
                {
                    // If no items, go to search input
                    isSearchMode = true;
                }
                break;
            case 80: // Down arrow
                if (isSearchMode)
                {
                    // Exit search mode and go to first item
                    isSearchMode = false;
                    if (!filteredItems.empty())
                    {
                        selectedIndex = 0;
                    }
                }
                else if (!filteredItems.empty())
                {
                    if (selectedIndex < filteredItems.size() - 1)
                    {
                        selectedIndex++;
                    }
                    // No wrapping - stay at the last item when at the end
                }
                break;
            }
        }
#else
        if (key == 27) // ESC sequence on Linux
        {
            // Check if there's more input available (arrow key sequence)
            if (kbhit())
            {
                key = getch();
                if (key == '[' && kbhit())
                {
                    key = getch();
                    switch (key)
                    {
                    case 'A': // Up arrow
                        if (isSearchMode)
                        {
                            // Do nothing when in search mode and pressing up
                        }
                        else if (!filteredItems.empty())
                        {
                            if (selectedIndex > 0)
                            {
                                selectedIndex--;
                            }
                            else
                            {
                                // When at top of list, go to search input
                                isSearchMode = true;
                            }
                        }
                        else
                        {
                            // If no items, go to search input
                            isSearchMode = true;
                        }
                        break;
                    case 'B': // Down arrow
                        if (isSearchMode)
                        {
                            // Exit search mode and go to first item
                            isSearchMode = false;
                            if (!filteredItems.empty())
                            {
                                selectedIndex = 0;
                            }
                        }
                        else if (!filteredItems.empty())
                        {
                            if (selectedIndex < filteredItems.size() - 1)
                            {
                                selectedIndex++;
                            }
                            // No wrapping - stay at the last item when at the end
                        }
                        break;
                    }
                }
                else
                {
                    // Not an arrow key sequence, treat as standalone ESC
                    goto handle_escape;
                }
            }
            else
            {
                // Standalone ESC key
                handle_escape:
                if (isSearchMode)
                {
                    // Exit search mode
                    isSearchMode = false;
                }
                else
                {
                    showCursor();
                    clearScreen();
                    return -1;
                }
            }
        }
#endif
        else
        {
            switch (key)
            {
#ifdef _WIN32
            case 13: // Enter (Windows)
#else
            case 10: // Enter (Linux - Line Feed)
            case 13: // Enter (also handle Carriage Return for compatibility)
#endif
                if (isSearchMode)
                {
                    // Exit search mode
                    isSearchMode = false;
                }
                else if (!filteredItems.empty())
                {
                    // Select item
                    showCursor();
                    clearScreen();
                    // Find the original index in the items array
                    auto it = std::find(items.begin(), items.end(), filteredItems[selectedIndex]);
                    if (it != items.end())
                    {
                        return static_cast<int>(std::distance(items.begin(), it));
                    }
                    return -1;
                }
                break;
            case 27: // Escape - only handle for Windows since Linux handles it above
#ifdef _WIN32
                if (isSearchMode)
                {
                    // Exit search mode
                    isSearchMode = false;
                }
                else
                {
                    showCursor();
                    clearScreen();
                    return -1;
                }
#endif
                break;
            case 3: // Ctrl+C
                showCursor();
                clearScreen();
                return -1;
            case 8:   // Backspace (BS)
            case 127: // Backspace (DEL) - common on Linux
                if (isSearchMode && !searchQuery.empty())
                {
                    searchQuery.pop_back();
                    applyFilter();
                }
                else if (!isSearchMode && !searchQuery.empty())
                {
                    // Clear search when not in search mode
                    searchQuery.clear();
                    applyFilter();
                }
                break;
            case '/': // Enter search mode
                isSearchMode = true;
                break;
            default:
                if (isSearchMode && key >= 32 && key <= 126)
                { // Printable characters
                    searchQuery += static_cast<char>(key);
                    applyFilter();
                }
                break;
            }
        }
    }
}

void InteractiveList::updateItems(const std::vector<std::string>& newItems)
{
    items = newItems;
    
    // Preserve current search if any
    if (!searchQuery.empty()) {
        applyFilter();
    } else {
        filteredItems = items;
    }
    
    // Adjust selected index if it's now out of bounds
    if (selectedIndex >= filteredItems.size() && !filteredItems.empty()) {
        selectedIndex = filteredItems.size() - 1;
    }
    
    // Recalculate display parameters
    maxDisplayItems = calculateMaxDisplayItems();
}

#ifndef _WIN32
// Linux-specific helper functions
int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void msleep(int milliseconds) {
    usleep(milliseconds * 1000);
}

unsigned long getTickCount() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif
