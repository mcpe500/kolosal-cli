# Kolosal CLI

**Cross-Platform Model Management** - Available for Windows and Linux  
**Model Discovery** - Automatically fetches Kolosal models from Hugging Face  
**Interactive Selection** - Navigate through models and files with keyboard controls  
**Smart Search** - Real-time filtering with search functionality  
**Quantization Info** - Detailed information about model quantization types  
**Fast Navigation** - Efficient viewport handling for large model lists  
**User-Friendly** - Clear visual indicators and helpful instructions  
**Smart Caching** - Intelligent caching system reduces API calls and improves performance  

A cross-platform command-line interface for browsing and managing Kolosal language models from Hugging Face. Features an interactive console interface for selecting models and quantized .gguf files. **Now with full Linux support!**

## Platform Support

- ✅ **Windows** - Fully supported with Visual Studio and MinGW
- ✅ **Linux** - Fully supported on Ubuntu, Debian, CentOS, Fedora, Arch Linux  
- ⚠️ **macOS** - Should work but not extensively tested

## Features

**Model Discovery** - Automatically fetches Kolosal models from Hugging Face
**Interactive Selection** - Navigate through models and files with keyboard controls
**Smart Search** - Real-time filtering with search functionality
**Quantization Info** - Detailed information about model quantization types
**Fast Navigation** - Efficient viewport handling for large model lists
**User-Friendly** - Clear visual indicators and helpful instructions

## Supported Quantization Types

The CLI automatically detects and provides descriptions for various quantization formats:

- **Q8_0** - 8-bit quantization, good balance of quality and size
- **Q6_K** - 6-bit quantization, high quality with smaller size  
- **Q5_K_M/S** - 5-bit quantization (medium/small variants)
- **Q4_K_M/S** - 4-bit quantization (medium/small variants)
- **Q3_K_L/M/S** - 3-bit quantization (large/medium/small variants)
- **Q2_K** - 2-bit quantization, extremely small but lower quality
- **F16/F32** - Floating point formats (16-bit/32-bit)

## Architecture

The project follows a clean, modular architecture with clear separation of concerns:

```
include/           # Header files
├── http_client.h         # HTTP communication with libcurl
├── model_file.h          # Model file data structures
├── hugging_face_client.h # Hugging Face API client
├── interactive_list.h    # Console UI component
├── cache_manager.h       # Smart caching system
└── kolosal_cli.h        # Main application logic

src/              # Implementation files
├── http_client.cpp
├── model_file.cpp
├── hugging_face_client.cpp
├── interactive_list.cpp
├── cache_manager.cpp
├── kolosal_cli.cpp
└── main.cpp             # Entry point
```

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed documentation.

## Smart Caching System

Kolosal CLI includes an intelligent caching mechanism that significantly improves performance:

### Features
- **Two-Level Cache**: In-memory and persistent disk storage
- **Automatic TTL**: Models (1 hour), Model Files (30 minutes)
- **Cache Management**: Interactive menu for cache operations
- **Cross-Platform**: Works on Windows, Linux, and macOS

### Benefits
- **Faster Startup**: Cached data loads instantly
- **Reduced API Calls**: Less network traffic to Hugging Face
- **Offline Capability**: Access recently cached data without internet
- **User Control**: Clear cache when needed

### Cache Management Menu
- Continue to Model Selection
- Clear Cache
- View Cache Status
- Exit Application

See [CACHING.md](docs/CACHING.md) for detailed information.

## Building from Source

### Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler
- Platform-specific dependencies (see below)

#### Windows
- libcurl (included in external/)
- nlohmann/json (included in external/)
- Visual Studio 2019 or later (recommended)

#### Linux
- libcurl4-openssl-dev (Ubuntu/Debian) or libcurl-devel (RHEL/CentOS)
- libssl-dev and zlib1g-dev
- libyaml-cpp-dev (optional, built from source if not available)
- build-essential or equivalent

### Quick Start (Linux)

1. **Install dependencies:**
   ```bash
   # Run the automated installer
   chmod +x install-linux-deps.sh
   ./install-linux-deps.sh
   
   # Or install manually:
   # Ubuntu/Debian:
   sudo apt install build-essential cmake git libcurl4-openssl-dev libssl-dev zlib1g-dev
   
   # CentOS/RHEL/Fedora:
   sudo dnf install gcc-c++ cmake git libcurl-devel openssl-devel zlib-devel
   
   # Arch Linux:
   sudo pacman -S base-devel cmake git curl openssl zlib
   ```

2. **Build the project:**
   ```bash
   # Run the automated build script
   chmod +x build-linux.sh
   ./build-linux.sh
   
   # Or build manually:
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

3. **Run the application:**
   ```bash
   cd build
   ./kolosal-cli
   ```

### Manual Build Instructions

#### Linux/macOS

1. Navigate to the project directory:
   ```bash
   cd kolosal-cli
   ```

2. Create and enter build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure with CMake:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

4. Build the project:
   ```bash
   make -j$(nproc)
   ```

#### Windows

1. Navigate to the project directory:
   ```bash
   cd kolosal-cli
   ```

2. Create and enter build directory:
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
   # Windows
   .\Release\kolosal-cli.exe
   
   # Linux/macOS
   ./kolosal-cli
   ```

## Packaging

Kolosal CLI supports creating distributable packages for easy installation on Linux systems.

### Creating a DEB Package (Linux)

1. **Build the project for packaging:**
   ```bash
   mkdir build-release
   cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

2. **Create the DEB package:**
   ```bash
   make package
   ```

   This will create a `.deb` file named `kolosal_1.0.0_amd64.deb` (architecture may vary).

3. **Install the package:**
   ```bash
   sudo dpkg -i kolosal_1.0.0_amd64.deb
   ```

4. **Verify installation:**
   ```bash
   kolosal --help
   ```

   After installation, you can run the CLI from anywhere using just `kolosal`.

### Package Features

- **System Integration**: Installs to `/usr/bin/kolosal` for system-wide access
- **Dependencies**: Automatically handles system dependencies (libcurl, OpenSSL, etc.)
- **Configuration**: Installs default config to `/etc/kolosal/config.yaml`
- **Desktop Entry**: Includes desktop file for GUI environments
- **Clean Uninstall**: Easy removal with `sudo dpkg -r kolosal`

### Uninstalling

To remove the installed package:

```bash
# Remove package but keep configuration files
sudo dpkg -r kolosal

# Remove package and all configuration files
sudo dpkg -P kolosal

# Alternative using apt (if available)
sudo apt remove kolosal
# or
sudo apt purge kolosal
```

### Manual Verification

Check what files were installed:
```bash
dpkg -L kolosal
```

Verify package status:
```bash
dpkg -l | grep kolosal
```

### Platform Support

- ✅ **Windows** - Fully supported with Visual Studio and MinGW
- ✅ **Linux** - Fully supported on Ubuntu, Debian, CentOS, Fedora, Arch Linux
- ⚠️ **macOS** - Should work but not extensively tested

### Troubleshooting

#### Linux Build Issues

1. **Missing CURL development headers:**
   ```bash
   # Ubuntu/Debian
   sudo apt install libcurl4-openssl-dev
   
   # CentOS/RHEL/Fedora
   sudo dnf install libcurl-devel
   ```

2. **CMake version too old:**
   ```bash
   # Install newer CMake from official repository
   wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
   sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
   sudo apt update && sudo apt install cmake
   ```

3. **Build fails with permission errors:**
   ```bash
   # Ensure scripts are executable
   chmod +x build-linux.sh install-linux-deps.sh
   ```

## Usage

### Navigation Controls

- **↑/↓ Arrow Keys** - Navigate through items
- **Enter** - Select highlighted item  
- **Escape** - Cancel operation or exit search mode
- **/** - Enter search mode
- **Backspace** - Edit search query or clear search
- **Ctrl+C** - Exit application

### Basic Workflow

1. **Model Selection**: Browse available Kolosal models from Hugging Face
2. **File Selection**: Choose from available .gguf files for the selected model
3. **Information Display**: View details about the selected model and file
4. **Download URL**: Get the direct download link for the model file

### Search Functionality

- Press `/` to enter search mode
- Type to filter models/files in real-time
- Press `Backspace` to edit or clear search
- Press `Enter` or arrow keys to exit search mode

## Error Handling

The application gracefully handles various error conditions:

- **Network Issues**: Falls back to sample data when API requests fail
- **Empty Results**: Provides helpful messages and suggestions
- **API Errors**: Clear error reporting with debug information

## Dependencies

### Windows
- [libcurl](https://curl.se/libcurl/) - HTTP client library (bundled)
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing library (bundled)
- Windows Console API - For interactive terminal features

### Linux  
- [libcurl](https://curl.se/libcurl/) - HTTP client library (system package)
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing library (bundled)
- OpenSSL/TLS - For secure connections
- Standard POSIX libraries - For terminal and system operations

## Linux-Specific Features

- **Native ANSI Terminal Support** - Full color and cursor control
- **POSIX-Compliant File Operations** - Proper Linux permissions and paths
- **System Package Integration** - Uses system libcurl and OpenSSL
- **XDG Base Directory Support** - Follows Linux configuration standards
- **Signal Handling** - Proper SIGINT/SIGTERM handling for graceful shutdown
- **Performance Optimized** - Native Linux networking and file I/O

For detailed Linux setup and usage instructions, see [LINUX.md](LINUX.md).

## Future Features

- **File Download** - Direct downloading of selected model files
- **Configuration** - Customizable settings and preferences
- **Progress Bars** - Visual progress indicators for downloads
- **Model Management** - Local model organization and management
- **Cross-Platform UI** - Enhanced compatibility across operating systems

## Contributing

Contributions are welcome! The modular architecture makes it easy to add new features:

- Add new API endpoints in `HuggingFaceClient`
- Enhance UI components in `InteractiveList`
- Extend model file utilities in `ModelFileUtils`
- Improve error handling and user experience

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.