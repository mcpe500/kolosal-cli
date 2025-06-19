#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <windows.h>

class InteractiveList {
private:
    std::vector<std::string> items;
    size_t selectedIndex;
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

public:
    InteractiveList(const std::vector<std::string>& listItems) 
        : items(listItems), selectedIndex(0) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &csbi);
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }

    void showCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = true;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }

    void clearScreen() {
        system("cls");
    }

    void moveCursor(int x, int y) {
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(hConsole, coord);
    }

    void setColor(int color) {
        SetConsoleTextAttribute(hConsole, color);
    }

    void resetColor() {
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);
    }

    void displayList() {
        clearScreen();
        
        // Display title
        std::cout << "Kolosal CLI - Interactive List\n";
        std::cout << "Use UP/DOWN arrows to navigate, ENTER to select, ESC or Ctrl+C to exit\n\n";        // Display items
        for (size_t i = 0; i < items.size(); ++i) {
            if (i == selectedIndex) {
                // Highlight selected item
                setColor(BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                std::cout << "> " << items[i] << std::endl;
                resetColor();
            } else {
                std::cout << "  " << items[i] << std::endl;
            }
        }

        std::cout << "\nSelected: " << items[selectedIndex] << std::endl;
    }    int run() {
        hideCursor();
        
        while (true) {
            displayList();
            
            int key = _getch();
            
            // Handle special keys (arrow keys)
            if (key == 224) { // Special key prefix
                key = _getch();
                switch (key) {
                    case 72: // Up arrow
                        if (selectedIndex > 0) {
                            selectedIndex--;
                        } else {
                            selectedIndex = items.size() - 1; // Wrap to bottom
                        }
                        break;
                    case 80: // Down arrow
                        if (selectedIndex < items.size() - 1) {
                            selectedIndex++;
                        } else {
                            selectedIndex = 0; // Wrap to top
                        }
                        break;
                }
            } else {
                switch (key) {
                    case 13: // Enter
                        showCursor();
                        clearScreen();
                        std::cout << "You selected: " << items[selectedIndex] << std::endl;
                        return static_cast<int>(selectedIndex);
                    case 27: // Escape
                        showCursor();
                        clearScreen();
                        std::cout << "Operation cancelled." << std::endl;
                        return -1;
                    case 3: // Ctrl+C
                        showCursor();
                        clearScreen();
                        std::cout << "Interrupted by user." << std::endl;
                        return -1;
                }
            }
        }
    }
};

int main() {
    // Sample list items
    std::vector<std::string> menuItems = {
        "Deploy Local LLM",
        "Manage Models",
        "Chat with Model",
        "Configuration",
        "View Logs",
        "System Status",
        "Help & Documentation",
        "Exit Application"
    };

    std::cout << "Welcome to Kolosal CLI!\n";
    std::cout << "Initializing interactive menu...\n\n";
    
    // Small delay to show welcome message
    Sleep(1000);

    InteractiveList menu(menuItems);
    int result = menu.run();

    if (result >= 0) {
        std::cout << "Processing selection: " << menuItems[result] << std::endl;
        std::cout << "Feature implementation coming soon!" << std::endl;
    }

    std::cout << "\nPress any key to exit...";
    _getch();
    
    return 0;
}