#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;

int main() {
    // Application state
    std::string input_text = "";
    std::vector<std::string> menu_options = {
        "View System Info",
        "Process Manager", 
        "File Explorer",
        "Network Tools",
        "Settings",
        "Exit"
    };
    int selected_menu = 0;
    std::vector<std::string> log_messages;
    
    // Add welcome message
    log_messages.push_back("Welcome to Kolosal CLI!");
    log_messages.push_back("Use arrow keys to navigate the menu.");

    // Components
    auto input = Input(&input_text, "Enter command...");
    
    auto menu = Menu(&menu_options, &selected_menu);
    
    auto execute_button = Button("Execute", [&] {
        if (!input_text.empty()) {
            log_messages.push_back("> " + input_text);
            
            // Simple command processing
            if (input_text == "help") {
                log_messages.push_back("Available commands: help, clear, version, status");
            } else if (input_text == "clear") {
                log_messages.clear();
                log_messages.push_back("Log cleared.");
            } else if (input_text == "version") {
                log_messages.push_back("Kolosal CLI v1.0.0");
            } else if (input_text == "status") {
                log_messages.push_back("System status: OK");
            } else {
                log_messages.push_back("Unknown command. Type 'help' for available commands.");
            }
            
            input_text = "";
        }
    });
    
    auto menu_action_button = Button("Select", [&] {
        switch (selected_menu) {
            case 0:
                log_messages.push_back("System Info: Windows OS detected");
                log_messages.push_back("Memory: Available");
                log_messages.push_back("CPU: Running normally");
                break;
            case 1:
                log_messages.push_back("Process Manager: Listing processes...");
                log_messages.push_back("Found 42 active processes");
                break;
            case 2:
                log_messages.push_back("File Explorer: Current directory listed");
                log_messages.push_back("Files: CMakeLists.txt, README.md, src/");
                break;
            case 3:
                log_messages.push_back("Network Tools: Checking connectivity...");
                log_messages.push_back("Network status: Connected");
                break;
            case 4:
                log_messages.push_back("Settings: Configuration panel opened");
                break;
            case 5:
                log_messages.push_back("Goodbye!");
                exit(0);
                break;
        }
    });
    
    auto clear_log_button = Button("Clear Log", [&] {
        log_messages.clear();
        log_messages.push_back("Log cleared.");
    });

    // Layout components
    auto input_container = Container::Horizontal({
        input,
        execute_button
    });
    
    auto menu_container = Container::Horizontal({
        menu,
        menu_action_button
    });
    
    auto button_container = Container::Horizontal({
        clear_log_button
    });
    
    auto main_container = Container::Vertical({
        input_container,
        menu_container,
        button_container
    });

    // Renderer
    auto renderer = Renderer(main_container, [&] {
        // Create log display
        Elements log_elements;
        int max_logs = 10; // Show last 10 log messages
        int start_idx = std::max(0, (int)log_messages.size() - max_logs);
        
        for (int i = start_idx; i < log_messages.size(); ++i) {
            log_elements.push_back(text(log_messages[i]));
        }
        
        auto log_box = vbox(log_elements) | 
                       border | 
                       size(HEIGHT, EQUAL, 12) | 
                       frame;

        return vbox({
            // Header
            text("🚀 KOLOSAL CLI") | bold | color(Color::Cyan) | center,
            separator(),
            
            // Command input section
            vbox({
                text("Command Input:") | bold,
                hbox({
                    input->Render() | flex,
                    separator(),
                    execute_button->Render()
                }) | border
            }),
            
            // Menu section
            vbox({
                text("Main Menu:") | bold,
                hbox({
                    menu->Render() | flex,
                    separator(),
                    menu_action_button->Render()
                }) | border
            }),
            
            // Controls
            hbox({
                clear_log_button->Render(),
                filler(),
                text("ESC to quit") | dim
            }),
            
            // Log section
            vbox({
                text("Output Log:") | bold,
                log_box
            })
        }) | border | size(WIDTH, GREATER_THAN, 80);
    });

    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(renderer);

    return 0;
}
