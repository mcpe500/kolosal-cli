# Kolosal CLI

A modern, interactive command-line interface built with FTXUI (C++ Terminal User Interface library).

## Features

🚀 **Interactive Terminal UI** - Beautiful, responsive interface using FTXUI
📝 **Command Input** - Execute custom commands with built-in help system
🎯 **Menu Navigation** - Easy navigation through different tool categories
📊 **Real-time Output** - Live log display with scrollable history
⚡ **Cross-Platform** - Works on Windows, Linux, and macOS

## Available Tools

- **System Info** - Display system information and status
- **Process Manager** - View and manage running processes
- **File Explorer** - Browse and navigate file system
- **Network Tools** - Network connectivity and diagnostic tools
- **Settings** - Configuration and preferences

## Built-in Commands

- `help` - Show available commands
- `clear` - Clear the output log
- `version` - Display version information
- `status` - Show system status

## Building from Source

### Prerequisites

- CMake 3.12 or higher
- C++17 compatible compiler
- Git (to clone FTXUI submodule)

### Build Instructions

1. Clone the repository:
   ```bash
   git clone <your-repo-url>
   cd kolosal-cli
   ```

2. Create build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build . --config Release
   ```

5. Run the application:
   ```bash
   # On Windows
   .\Release\kolosal-cli.exe
   
   # On Linux/macOS
   ./kolosal-cli
   ```

## Usage

1. **Navigation**: Use arrow keys to navigate through menu items
2. **Command Input**: Type commands in the input field and press "Execute"
3. **Menu Actions**: Select menu items and press "Select" to perform actions
4. **Exit**: Choose "Exit" from the menu or press ESC to quit

## Dependencies

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - Modern C++ Terminal User Interface library

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.