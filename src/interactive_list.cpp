#include "interactive_list.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <conio.h>

InteractiveList::InteractiveList(const std::vector<std::string> &listItems)
    : items(listItems), filteredItems(listItems), searchQuery(""), selectedIndex(0),
      viewportTop(0), maxDisplayItems(20), isSearchMode(false)
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Calculate max display items dynamically based on actual content
    maxDisplayItems = calculateMaxDisplayItems();
}

void InteractiveList::hideCursor()
{
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void InteractiveList::showCursor()
{
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void InteractiveList::clearScreen()
{
    system("cls");
}

void InteractiveList::moveCursor(int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void InteractiveList::setColor(int color)
{
    SetConsoleTextAttribute(hConsole, color);
}

void InteractiveList::resetColor()
{
    SetConsoleTextAttribute(hConsole, csbi.wAttributes);
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
    int totalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int reservedLines = 8; // UI elements (title, search, info, scroll indicators)
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
    hideCursor();

    // Handle empty list case
    if (items.empty())
    {
        displayList();
        std::cout << "\nPress any key to exit...";
        _getch();
        showCursor();
        return -1;
    }

    while (true)
    {
        displayList();

        int key = _getch();

        // Handle special keys (arrow keys)
        if (key == 224)
        { // Special key prefix
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
        else
        {
            switch (key)
            {
            case 13: // Enter
                if (isSearchMode)
                {
                    // Exit search mode
                    isSearchMode = false;
                }
                else if (!filteredItems.empty())
                { // Select item
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
            case 27: // Escape
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
                break;
            case 3: // Ctrl+C
                showCursor();
                clearScreen();
                return -1;
            case 8: // Backspace
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
