#ifndef INTERACTIVE_LIST_H
#define INTERACTIVE_LIST_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>

/**
 * @brief Interactive console-based list widget with search functionality
 */
class InteractiveList {
private:
    std::vector<std::string> items;
    std::vector<std::string> filteredItems;
    std::string searchQuery;
    size_t selectedIndex;
    size_t viewportTop;
    size_t maxDisplayItems;
    bool isSearchMode;
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

public:
    /**
     * @brief Constructor
     * @param listItems Vector of strings to display in the list
     */
    explicit InteractiveList(const std::vector<std::string>& listItems);
      /**
     * @brief Run the interactive list and wait for user selection
     * @return Index of selected item in original items vector, or -1 if cancelled
     */
    int run();

    /**
     * @brief Run the interactive list with periodic update callback
     * @param updateCallback Function called periodically to check for updates
     * @return Index of selected item in original items vector, or -1 if cancelled
     */
    int runWithUpdates(std::function<bool()> updateCallback);

    /**
     * @brief Update the items in the list without resetting the selection
     * @param newItems New vector of strings to display
     */
    void updateItems(const std::vector<std::string>& newItems);

private:
    /**
     * @brief Hide the console cursor
     */
    void hideCursor();
    
    /**
     * @brief Show the console cursor
     */
    void showCursor();
    
    /**
     * @brief Clear the console screen
     */
    void clearScreen();
    
    /**
     * @brief Move cursor to specific position
     * @param x X coordinate
     * @param y Y coordinate
     */
    void moveCursor(int x, int y);
    
    /**
     * @brief Set console text color
     * @param color Color attribute
     */
    void setColor(int color);
    
    /**
     * @brief Reset console text color to default
     */
    void resetColor();
    
    /**
     * @brief Update viewport to keep selected item visible
     */
    void updateViewport();
    
    /**
     * @brief Apply current search filter to items
     */
    void applyFilter();
    
    /**
     * @brief Display the current list state
     */
    void displayList();
    
    /**
     * @brief Calculate how many lines an item will take when displayed
     * @param item The item string to analyze
     * @return Number of lines the item will occupy
     */
    int calculateItemLines(const std::string& item);
    
    /**
     * @brief Calculate how many items can fit in the available space
     * @return Maximum number of items that can be displayed
     */
    size_t calculateMaxDisplayItems();
};

#endif // INTERACTIVE_LIST_H
