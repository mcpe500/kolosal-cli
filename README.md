# Kolosal CLI

**Cross-Platform Model Management** - Available for Windows and Linux  
**Model Discovery** - Automatically fetches Kolosal models from Hugging Face  
**Model Download & Deployment** - Download and run models locally with built-in server  
**Interactive Selection** - Navigate through models and files with keyboard controls  
**Smart Search** - Real-time filtering with search functionality  
**Quantization Info** - Detailed information about model quantization types  
**Chat Interface** - Direct chat interaction with loaded models  
**Server Management** - Start, stop, and manage background inference server  
**Smart Caching** - Intelligent caching system reduces API calls and improves performance  
**Configuration System** - Comprehensive YAML/JSON configuration support  

A cross-platform command-line interface for browsing, downloading, and running Kolosal language models from Hugging Face. Features an interactive console interface for selecting models, automatic downloading, and built-in inference server capabilities. **Now with full Linux support and local model execution!**

## Platform Support

- ✅ **Windows** - Fully supported with Visual Studio and MinGW
- ✅ **Linux** - Fully supported on Ubuntu, Debian, CentOS, Fedora, Arch Linux  
- ⚠️ **macOS** - Should work but not extensively tested

## Features

**Model Discovery** - Automatically fetches Kolosal models from Hugging Face  
**Model Download** - Download GGUF model files directly to your local machine  
**Server Management** - Built-in inference server with start/stop capabilities  
**Chat Interface** - Direct chat interaction with loaded models via CLI or API  
**Interactive Selection** - Navigate through models and files with keyboard controls  
**Smart Search** - Real-time filtering with search functionality  
**Quantization Info** - Detailed information about model quantization types  
**Configuration System** - Comprehensive YAML/JSON configuration support  
**Progress Monitoring** - Real-time download progress with size estimates  
**Background Operations** - Server runs in background for continuous availability

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
├── kolosal_server_client.h # Server management and communication
├── command_manager.h     # Command parsing and execution
├── loading_animation.h   # Progress indicators
└── kolosal_cli.h        # Main application logic

src/              # Implementation files
├── http_client.cpp
├── model_file.cpp
├── hugging_face_client.cpp
├── interactive_list.cpp
├── cache_manager.cpp
├── kolosal_server_client.cpp
├── command_manager.cpp
├── loading_animation.cpp
├── kolosal_cli.cpp
└── main.cpp             # Entry point

kolosal-server/   # Integrated inference server
├── src/                 # Server source code
├── include/             # Server headers
├── docs/                # Server documentation
└── config.yaml          # Server configuration
```

### Integrated Components

- **CLI Interface**: Interactive model browsing and selection
- **Download Engine**: Multi-threaded model file downloading
- **Inference Server**: Built-in HTTP server for model inference
- **Configuration System**: YAML/JSON-based configuration management
- **Cache Management**: Intelligent caching for offline capability

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed documentation.

## Smart Caching & Model Management

Kolosal CLI includes an intelligent caching and model management system that significantly improves performance:

### Caching Features
- **Two-Level Cache**: In-memory and persistent disk storage
- **Automatic TTL**: Models (1 hour), Model Files (30 minutes)
- **Download Cache**: Downloaded models are cached locally
- **Configuration Cache**: Server and model configurations are cached
- **Cross-Platform**: Works on Windows, Linux, and macOS

### Model Storage
- **Local Storage**: Downloaded models stored in user directory
- **Automatic Organization**: Models organized by repository and version
- **Space Management**: Built-in tools for managing disk usage
- **Integrity Checking**: Automatic validation of downloaded files

### Benefits
- **Faster Startup**: Cached data loads instantly
- **Reduced API Calls**: Less network traffic to Hugging Face
- **Offline Capability**: Access recently cached data without internet
- **Local Inference**: Run models locally without internet connection
- **Bandwidth Efficiency**: Resume interrupted downloads

### Cache Management
The application provides interactive cache management:
- **Continue to Model Selection** - Proceed with normal operation
- **Clear Cache** - Remove all cached data
- **View Cache Status** - See current cache usage and statistics
- **Model Management** - Remove specific downloaded models
- **Exit Application** - Clean shutdown

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
   # Ubuntu/Debian:
   sudo apt install build-essential cmake git libcurl4-openssl-dev libssl-dev zlib1g-dev
   
   # CentOS/RHEL/Fedora:
   sudo dnf install gcc-c++ cmake git libcurl-devel openssl-devel zlib-devel
   
   # Arch Linux:
   sudo pacman -S base-devel cmake git curl openssl zlib
   ```

2. **Build the project:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

3. **Run the application:**
   ```bash
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

### Creating a Windows Installer (Windows)

Kolosal CLI supports creating .exe installer packages for easy distribution and installation on Windows systems.

1. **Build the project for packaging:**
   ```powershell
   mkdir build-release
   cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```

2. **Create the installer package:**
   ```powershell
   cmake --build . --target package --config Release
   ```

   This will create an .exe installer file named `kolosal_1.0.0_win64.exe` (architecture may vary).

3. **Install the package:**
   ```powershell
   # Run as Administrator
   .\kolosal_1.0.0_win64.exe /S
   
   # Or double-click the .exe file for GUI installation
   ```

4. **Verify installation:**
   ```powershell
   kolosal --help
   ```

   After installation, you can run the CLI from anywhere using just `kolosal`.

### Windows Package Features

- **System Integration**: Installs to `C:\Program Files\Kolosal` with PATH integration
- **Dependencies**: Includes all required DLLs (libcurl, OpenSSL, etc.)
- **Configuration**: Installs default config to `%PROGRAMDATA%\Kolosal\config.yaml`
- **Start Menu Entry**: Adds shortcuts to Windows Start Menu
- **Registry Integration**: Proper Windows registry entries for Add/Remove Programs
- **Clean Uninstall**: Easy removal through Windows Settings or Control Panel

### Windows Uninstalling

To remove the installed package:

```powershell
# Uninstall via Control Panel
# Control Panel > Programs > Programs and Features > Kolosal > Uninstall

# Or use Windows Settings
# Settings > Apps > Apps & features > Kolosal > Uninstall

# Or run the uninstaller directly (if available)
# C:\Program Files\Kolosal\uninstall.exe
```

### Windows Manual Verification

Check installation status:
```powershell
# Check if installed via registry
Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | Where-Object {$_.DisplayName -eq "Kolosal"}

# Check PATH integration
where kolosal

# Check installation directory
dir "C:\Program Files\Kolosal"
```

### Creating Portable Windows Package

For users who prefer a portable version without installation:

1. **Build for portable deployment:**
   ```powershell
   mkdir build-portable
   cd build-portable
   cmake .. -DCMAKE_BUILD_TYPE=Release -DPORTABLE_BUILD=ON
   cmake --build . --config Release
   ```

2. **Create ZIP package:**
   ```powershell
   cmake --build . --target package --config Release
   ```

   This creates `kolosal_1.0.0_portable.zip` containing all necessary files.

3. **Deploy portable version:**
   ```powershell
   # Extract to desired location
   Expand-Archive kolosal_1.0.0_portable.zip -DestinationPath "C:\Tools\Kolosal"
   
   # Run directly
   C:\Tools\Kolosal\kolosal.exe
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
   # Make sure you have proper permissions for the build directory
   sudo chown -R $USER:$USER ./build
   ```

4. **Package installation fails with DEB control file errors:**
   ```bash
   # If you get "parsing file 'control' near line X" errors:
   # Clean the build directory and rebuild the package
   rm -rf build-release
   mkdir build-release && cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   make package
   
   # If the error persists, check CMake version and dependencies
   cmake --version  # Should be >= 3.14
   dpkg --version   # Check dpkg version
   ```

5. **Server fails to start:**
   ```bash
   # Check if port is available
   netstat -tlnp | grep :8080
   
   # Or use a different port in config.yaml
   server:
     port: "8081"
   ```

#### Server Issues

1. **Port already in use:**
   ```bash
   # Find process using the port
   lsof -i :8080
   
   # Kill the process or change port in config.yaml
   ```

2. **Model loading failures:**
   ```bash
   # Check model file integrity
   file ./models/model.gguf
   
   # Verify disk space
   df -h ./models/
   ```

3. **Memory issues:**
   ```bash
   # Monitor memory usage
   htop
   
   # Reduce model context size in config.yaml
   models:
     - load_params:
         n_ctx: 1024  # Reduce from 2048
   ```

4. **Permission denied errors:**
   ```bash
   # Fix file permissions
   chmod +x kolosal-cli
   chmod -R 755 ./models/
   ```

#### Windows Build Issues

1. **Missing Visual Studio Build Tools:**
   ```powershell
   # Install Visual Studio Build Tools
   # Download from: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
   
   # Or install via Chocolatey
   choco install visualstudio2022buildtools
   
   # Or use Visual Studio Installer
   # Install "Desktop development with C++" workload
   ```

2. **CMake not found:**
   ```powershell
   # Install CMake via installer
   # Download from: https://cmake.org/download/
   
   # Or install via Chocolatey
   choco install cmake
   
   # Or install via winget
   winget install Kitware.CMake
   ```

3. **Build fails with CURL errors:**
   ```powershell
   # Ensure libcurl is properly included in external/
   # Check if external/curl/ directory exists and contains headers
   
   # Force rebuild of external dependencies
   cmake .. -DFORCE_REBUILD_EXTERNALS=ON
   ```

4. **Installer packaging fails:**
   ```powershell
   # Install NSIS (Nullsoft Scriptable Install System) for .exe creation
   # Download from: https://nsis.sourceforge.io/Download
   
   # Or install via Chocolatey
   choco install nsis
   
   # Ensure NSIS is in PATH
   $env:PATH += ";C:\Program Files (x86)\NSIS"
   ```

5. **Permission errors during build:**
   ```powershell
   # Run PowerShell as Administrator
   # Or ensure build directory has proper permissions
   icacls build /grant "$($env:USERNAME):F" /t
   ```

6. **Server fails to start on Windows:**
   ```powershell
   # Check if port is available
   netstat -an | findstr :8080
   
   # Check Windows Firewall
   netsh advfirewall firewall show rule name="Kolosal CLI"
   
   # Add firewall rule if needed
   netsh advfirewall firewall add rule name="Kolosal CLI" dir=in action=allow protocol=TCP localport=8080
   ```

#### Windows-Specific Server Issues

1. **Port already in use:**
   ```powershell
   # Find process using the port
   netstat -ano | findstr :8080
   
   # Kill the process (replace PID with actual process ID)
   taskkill /PID <PID> /F
   
   # Or change port in config.yaml
   ```

2. **Windows Defender blocking:**
   ```powershell
   # Add exclusion for Kolosal directory
   Add-MpPreference -ExclusionPath "C:\Program Files\Kolosal"
   Add-MpPreference -ExclusionPath "$env:USERPROFILE\kolosal-cli"
   
   # Or temporarily disable real-time protection
   Set-MpPreference -DisableRealtimeMonitoring $true
   ```

3. **DLL not found errors:**
   ```powershell
   # Check for missing dependencies
   dumpbin /dependents kolosal.exe
   
   # Install Visual C++ Redistributables
   # Download from: https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads
   
   # Or copy required DLLs to application directory
   copy "C:\Windows\System32\vcruntime*.dll" .
   copy "C:\Windows\System32\msvcp*.dll" .
   ```

4. **Path not found errors:**
   ```powershell
   # Add Kolosal to system PATH
   $env:PATH += ";C:\Program Files\Kolosal"
   
   # Or add permanently via registry
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\Kolosal", [EnvironmentVariableTarget]::Machine)
   ```

## Usage

### Command Line Options

```bash
kolosal-cli [options] [repository_url_or_id]

Options:
  --help, -h        Show help message
  --stop-server     Stop the background Kolosal server

Examples:
  kolosal-cli                                    # Browse all kolosal models
  kolosal-cli microsoft/DialoGPT-medium          # Direct access to model
  kolosal-cli https://huggingface.co/microsoft/DialoGPT-medium
  kolosal-cli --stop-server                      # Stop the background server

Arguments:
  repository_url_or_id  Hugging Face repository URL or ID (e.g., owner/model-name)
```

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
3. **Download**: Automatic download of selected model file to local storage
4. **Server Management**: Option to start/stop the background inference server
5. **Chat Interface**: Direct interaction with loaded models

### Search Functionality

- Press `/` to enter search mode
- Type to filter models/files in real-time
- Press `Backspace` to edit or clear search
- Press `Enter` or arrow keys to exit search mode
- Search works across model names, descriptions, and file types

### Download Progress

When downloading models, the CLI displays:
- **Real-time Progress** - Current download status and speed
- **Size Information** - Downloaded vs. total file size
- **Time Estimates** - Estimated time remaining
- **Background Operation** - Downloads continue while browsing other models
- **Cancellation** - Ability to cancel active downloads

### Server Configuration

The application includes a built-in inference server that can be configured via `config.yaml`:

```yaml
server:
  port: "8080"
  host: "0.0.0.0"

logging:
  level: "INFO"
  file: "server.log"

models:
  - id: "example-model"
    path: "./models/model.gguf"
    load_immediately: false
```

For complete configuration options, see the [server documentation](kolosal-server/docs/CONFIGURATION.md).

## Error Handling & Reliability

The application gracefully handles various error conditions:

### Network & API Issues
- **Connection Failures**: Automatic retry with exponential backoff
- **API Errors**: Clear error reporting with debug information
- **Rate Limiting**: Respects Hugging Face API limits with automatic throttling
- **Timeout Handling**: Configurable timeouts for downloads and API calls

### Download Management
- **Resume Support**: Interrupted downloads can be resumed
- **Corruption Detection**: Automatic validation of downloaded files
- **Disk Space**: Pre-download space checking and management
- **Permission Errors**: Clear guidance for file system issues

### Server Operations
- **Port Conflicts**: Automatic port detection and alternative suggestions
- **Model Loading**: Graceful fallback when models fail to load
- **Memory Management**: Automatic cleanup and memory optimization
- **Configuration Errors**: Detailed validation with helpful error messages

### Fallback Mechanisms
- **Sample Data**: Falls back to sample data when API requests fail
- **Offline Mode**: Cached data remains available without internet
- **Default Configuration**: Sensible defaults when configuration is missing
- **Emergency Stop**: Safe shutdown procedures for all operations

## Dependencies

### Windows
- [libcurl](https://curl.se/libcurl/) - HTTP client library (bundled)
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing library (bundled)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) - YAML configuration parsing (bundled)
- Windows Console API - For interactive terminal features

### Linux  
- [libcurl](https://curl.se/libcurl/) - HTTP client library (system package)
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing library (bundled)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) - YAML configuration parsing (bundled)
- OpenSSL/TLS - For secure connections
- Standard POSIX libraries - For terminal and system operations

### Server Components
- **llama.cpp** - High-performance LLM inference engine (integrated)
- **HTTP Server** - Custom lightweight HTTP server for API endpoints
- **Model Management** - Automatic model loading and memory management
- **Authentication** - API key and rate limiting support

## Linux-Specific Features

- **Native ANSI Terminal Support** - Full color and cursor control
- **POSIX-Compliant File Operations** - Proper Linux permissions and paths
- **System Package Integration** - Uses system libcurl and OpenSSL
- **XDG Base Directory Support** - Follows Linux configuration standards
- **Signal Handling** - Proper SIGINT/SIGTERM handling for graceful shutdown
- **Performance Optimized** - Native Linux networking and file I/O

For detailed Linux setup and usage instructions, see [LINUX.md](LINUX.md).

## Current Features

- **Model Discovery & Download** - Browse and download GGUF model files from Hugging Face
- **Integrated Inference Server** - Built-in HTTP server for local model inference
- **Chat Interface** - Direct chat interaction with loaded models
- **Server Management** - Start, stop, and configure background server operations
- **Configuration System** - Comprehensive YAML/JSON configuration support
- **Progress Monitoring** - Real-time download progress with detailed information
- **Authentication** - API key support and rate limiting for server endpoints
- **CORS Support** - Cross-origin resource sharing for web integration

## Roadmap & Future Features

- **Model Management UI** - Web-based interface for model administration
- **Multiple Model Support** - Concurrent loading and switching between models
- **Plugin System** - Extensible architecture for custom model providers
- **Docker Integration** - Containerized deployment options
- **Performance Metrics** - Real-time inference performance monitoring
- **Model Quantization** - Built-in quantization tools for model optimization

## Contributing

Contributions are welcome! The modular architecture makes it easy to add new features:

- **Add new API endpoints** in `KolosalServerClient` and server components
- **Enhance UI components** in `InteractiveList` and command management
- **Extend model file utilities** in `ModelFileUtils` and download management
- **Improve server functionality** in the `kolosal-server` submodule
- **Add configuration options** in the YAML/JSON configuration system
- **Enhance error handling** and user experience across all components

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style and architecture
4. Add tests for new functionality
5. Update documentation as needed
6. Submit a pull request

For server development, see [kolosal-server/docs/DEVELOPER_GUIDE.md](kolosal-server/docs/DEVELOPER_GUIDE.md).

## License

This project is licensed under the APACHE 2.0 License - see the [LICENSE](LICENSE) file for details.

## Kolosal Server Integration

Kolosal CLI includes a fully integrated inference server that provides:

### Server Features
- **HTTP API** - RESTful API for model inference and management
- **Model Loading** - Automatic loading and management of GGUF models
- **Chat Completions** - OpenAI-compatible chat completion endpoints
- **Streaming Responses** - Real-time streaming for interactive applications
- **Multi-Model Support** - Load and manage multiple models simultaneously
- **GPU Acceleration** - CUDA support for high-performance inference
- **Rate Limiting** - Configurable rate limiting and authentication
- **CORS Support** - Cross-origin requests for web applications

### API Endpoints

The server provides several key endpoints:

```
GET  /health                     # Server health check
GET  /v1/engines                 # List available models
POST /v1/engines                 # Add new model/engine
POST /v1/chat/completions        # Chat completion (OpenAI compatible)
GET  /v1/downloads               # List all active downloads
GET  /v1/downloads/{id}          # Get progress for specific download
POST /v1/downloads/{id}/cancel   # Cancel specific download
POST /v1/downloads/{id}/pause    # Pause specific download
POST /v1/downloads/{id}/resume   # Resume specific download
POST /v1/downloads/cancel        # Cancel all downloads
DELETE /v1/downloads/{id}        # Cancel specific download (alternative)
DELETE /v1/downloads             # Cancel all downloads (alternative)
```

### Configuration

Server behavior is controlled via `config.yaml`:

```yaml
server:
  port: "8080"
  host: "0.0.0.0"

logging:
  level: "INFO"
  file: "server.log"
  access_log: false

auth:
  enabled: true
  require_api_key: false
  rate_limit:
    enabled: true
    max_requests: 100
    window_size: 60

models:
  - id: "default-model"
    path: "./models/model.gguf"
    load_immediately: false
    load_params:
      n_ctx: 2048
      n_gpu_layers: 100
```

For complete configuration documentation, see [kolosal-server/docs/CONFIGURATION.md](kolosal-server/docs/CONFIGURATION.md).

### Server Management

The CLI provides commands to manage the server:

```bash
# Start with model browsing (server starts automatically)
kolosal-cli

# Stop the background server
kolosal-cli --stop-server

# Direct model specification
kolosal-cli owner/model-name
```

### Integration Examples

**Curl Example:**
```bash
# Chat with a loaded model
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "engine": "model-id",
    "messages": [{"role": "user", "content": "Hello!"}]
  }'
```

**Python Example:**
```python
import requests

response = requests.post(
    "http://localhost:8080/v1/chat/completions",
    json={
        "engine": "model-id",
        "messages": [{"role": "user", "content": "Hello!"}]
    }
)
print(response.json())
```