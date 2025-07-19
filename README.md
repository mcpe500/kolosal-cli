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
- ✅ **macOS** - Fully supported with detailed build instructions (Intel and Apple Silicon)

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

#### macOS
- Xcode Command Line Tools or Xcode (for clang compiler)
- Homebrew (recommended for dependency management)
- cmake (via Homebrew or official installer)
- curl (usually pre-installed, or via Homebrew)
- openssl (via Homebrew for secure connections)

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

### Quick Start (macOS)

1. **Install dependencies:**
   ```bash
   # Install Xcode Command Line Tools (if not already installed)
   xcode-select --install
   
   # Install Homebrew (if not already installed)
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Install required dependencies
   brew install cmake curl openssl git
   ```

2. **Build the project:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(sysctl -n hw.ncpu)
   ```

3. **Run the application:**
   ```bash
   ./kolosal-cli
   ```

### Manual Build Instructions

#### Linux

1. **Install dependencies:**
   ```bash
   # Ubuntu/Debian:
   sudo apt update
   sudo apt install build-essential cmake git libcurl4-openssl-dev libssl-dev zlib1g-dev
   
   # CentOS/RHEL/Fedora:
   sudo dnf install gcc-c++ cmake git libcurl-devel openssl-devel zlib-devel
   
   # Arch Linux:
   sudo pacman -S base-devel cmake git curl openssl zlib
   ```

2. **Navigate to the project directory:**
   ```bash
   cd kolosal-cli
   ```

3. **Create and enter build directory:**
   ```bash
   mkdir build
   cd build
   ```

4. **Configure with CMake:**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

5. **Build the project:**
   ```bash
   make -j$(nproc)
   ```

6. **Run the application:**
   ```bash
   ./kolosal-cli
   ```

#### macOS

1. **Install Xcode Command Line Tools:**
   ```bash
   # Install if not already available
   xcode-select --install
   
   # Verify installation
   xcode-select -p
   ```

2. **Install Homebrew (if not already installed):**
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Add Homebrew to PATH (if needed)
   echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
   source ~/.zshrc
   ```

3. **Install dependencies:**
   ```bash
   # Install required packages
   brew install cmake curl openssl git
   
   # Verify installations
   cmake --version    # Should be >= 3.14
   curl --version
   openssl version
   ```

4. **Navigate to the project directory:**
   ```bash
   cd kolosal-cli
   ```

5. **Create and enter build directory:**
   ```bash
   mkdir build
   cd build
   ```

6. **Configure with CMake:**
   ```bash
   # Basic configuration
   cmake .. -DCMAKE_BUILD_TYPE=Release
   
   # Or with OpenSSL path specification (if needed)
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
   ```

7. **Build the project:**
   ```bash
   # Use all available CPU cores
   make -j$(sysctl -n hw.ncpu)
   
   # Or specify number of cores manually
   # make -j8
   ```

8. **Run the application:**
   ```bash
   ./kolosal-cli
   ```

#### macOS Advanced Build Options

**For Apple Silicon (M1/M2) optimization:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=arm64 \
         -DUSE_METAL=ON
make -j$(sysctl -n hw.ncpu)
```

**For Intel Macs:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=x86_64
make -j$(sysctl -n hw.ncpu)
```

**For Universal Binary (both Intel and Apple Silicon):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
make -j$(sysctl -n hw.ncpu)
```

**For macOS Deployment Target:**
```bash
# Target macOS 11.0 and later (recommended for Apple Silicon features)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

# Or target macOS 10.15 for broader compatibility
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
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

### Creating a DMG Package (macOS)

Kolosal CLI supports creating .dmg disk image packages for easy distribution and installation on macOS systems.

1. **Build the project for packaging:**
   ```bash
   mkdir build-release
   cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(sysctl -n hw.ncpu)
   ```

2. **Create the DMG package:**
   ```bash
   make package
   ```
   
   **Note:** If you encounter permission errors about creating `/usr/local/etc/kolosal` or other system directories, you have several options:
   
   ```bash
   # Option 1: Use sudo (requires admin privileges)
   sudo make package
   
   # Option 2: Build for user installation (no admin privileges needed)
   cd ..
   rm -rf build-release
   mkdir build-release && cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
   make -j$(sysctl -n hw.ncpu)
   make package  # No sudo required
   ```

   This will create a compressed DMG file named `kolosal-1.0.0-apple-silicon.dmg` (for Apple Silicon) or `kolosal-1.0.0-intel-mac.dmg` (for Intel Macs).

3. **Install the package:**
   ```bash
   # Method 1: GUI Installation (Recommended)
   # Double-click the DMG file to mount it
   open kolosal-1.0.0-apple-silicon.dmg
   
   # This will open Finder with the DMG contents
   # Drag the Kolosal CLI application to your Applications folder
   # Then run Kolosal CLI from Applications to automatically set up command line access
   
   # Method 2: Command Line Installation with Automatic PATH Setup
   # Mount the DMG first
   hdiutil attach kolosal-1.0.0-apple-silicon.dmg
   
   # Copy to Applications and set up command line access
   sudo cp -R "/Volumes/Kolosal CLI/Kolosal CLI.app" /Applications/
   
   # Run the installer script to automatically create symlinks
   "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal_installer.sh"
   
   # Method 3: Manual Installation from DMG
   # After mounting the DMG, copy the files manually
   sudo cp -R "/Volumes/Kolosal CLI/Kolosal CLI.app" /Applications/
   
   # Manually create symlinks for command line access
   sudo ln -sf "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal" /usr/local/bin/kolosal
   sudo ln -sf "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal-server" /usr/local/bin/kolosal-server
   
   # Unmount the DMG when done
   hdiutil detach "/Volumes/Kolosal CLI"
   ```

4. **Verify installation:**
   ```bash
   # The kolosal command should now be automatically available
   kolosal --help
   
   # If the command is not found, the installation may need manual setup:
   # Option 1: Run the installer script manually
   "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal_installer.sh"
   
   # Option 2: Create symlinks manually
   sudo ln -sf "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal" /usr/local/bin/kolosal
   
   # Option 3: Add to your shell profile (if installed to user directory)
   echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
   source ~/.zshrc
   
   # Test the installation
   kolosal --version
   kolosal --help
   ```

   **Note:** The new DMG installer automatically creates symlinks to `/usr/local/bin/kolosal` when you run the application from the Applications folder, making the command available system-wide without manual PATH configuration.

   After installation, you can run the CLI from anywhere using just `kolosal`.

### macOS Package Features

- **System Integration**: Installs to `/usr/local/bin/kolosal` for system-wide access
- **Application Bundle**: Creates proper macOS application bundle structure
- **Native Libraries**: Includes all required dylib files (libkolosal_server.dylib, inference engines)
- **Configuration**: Installs default config to `/usr/local/etc/kolosal/config.yaml`
- **User Configuration**: Creates user config directory at `~/Library/Application Support/Kolosal/`
- **Homebrew Compatible**: Follows Homebrew conventions for easy integration
- **Metal Support**: Includes Apple Metal inference engine for Apple Silicon optimization
- **Universal Binary**: Supports both Intel and Apple Silicon architectures (if built universally)
- **Code Signing Ready**: Prepared for code signing and notarization

### macOS Advanced Packaging Options

**For Universal Binary (Intel + Apple Silicon):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
         -DUSE_METAL=ON
make -j$(sysctl -n hw.ncpu)
make package
```

**For Apple Silicon Only:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=arm64 \
         -DUSE_METAL=ON
make -j$(sysctl -n hw.ncpu)
make package
```

**For Intel Mac Only:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_OSX_ARCHITECTURES=x86_64
make -j$(sysctl -n hw.ncpu)
make package
```

**With Custom DMG Background (if available):**
```bash
# Place custom background image at docs/dmg_background.png
# DMG will use custom background for enhanced presentation
make package
```

**Quick Fix for Common DMG Issues:**
```bash
# If you encounter DMG creation errors, try this streamlined approach:
# 1. Create missing directories and files
mkdir -p docs

# 2. Create or disable background image
echo "Creating simple background..." 
# Either create a simple background or disable it entirely
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_DMG_BACKGROUND_IMAGE=""

# 3. Build and package
make -j$(sysctl -n hw.ncpu)
make package
```

### macOS Installation Methods

**Method 1: DMG Installation (Recommended)**
```bash
# Download or build the DMG package
# Double-click the DMG file in Finder
# Drag Kolosal CLI to Applications folder
# Optional: Add to PATH by symlinking to /usr/local/bin
sudo ln -sf /Applications/Kolosal\ CLI.app/Contents/MacOS/kolosal /usr/local/bin/kolosal
```

**Method 2: Direct System Installation**
```bash
# Build and install directly to system directories
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install
```

**Method 3: User-Local Installation (No sudo required)**
```bash
# Install to user directory (recommended for development)
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local"
make -j$(sysctl -n hw.ncpu)
make install  # No sudo needed

# Add to PATH in shell profile
echo 'export PATH="$HOME/local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

**Method 4: Homebrew-style Installation**
```bash
# Install to Homebrew-compatible locations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
         -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### macOS Packaging for Distribution

**For Personal Use (No admin privileges required):**
```bash
# Build package that doesn't require system directories
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
make -j$(sysctl -n hw.ncpu)
make package  # No sudo required
```

**For System-Wide Distribution (Requires admin privileges):**
```bash
# Build standard system package
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make package  # Requires sudo for system directories
```

**For Portable Distribution:**
```bash
# Create standalone package with all dependencies
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_RPATH="@executable_path" \
         -DCPACK_BUNDLE_STARTUP_COMMAND="bin/kolosal"
make -j$(sysctl -n hw.ncpu)
make package
```

### macOS Uninstalling

**If installed via DMG to Applications:**
```bash
# Remove application bundle
sudo rm -rf "/Applications/Kolosal CLI.app"

# Remove symlink if created
sudo rm -f /usr/local/bin/kolosal

# Remove user configuration (optional)
rm -rf ~/Library/Application\ Support/Kolosal
rm -rf ~/Library/Logs/Kolosal
rm -rf ~/Library/Caches/Kolosal
```

**If installed via system installation:**
```bash
# Remove binaries
sudo rm -f /usr/local/bin/kolosal
sudo rm -f /usr/local/bin/kolosal-server

# Remove libraries
sudo rm -f /usr/local/lib/libkolosal_server.dylib
sudo rm -f /usr/local/lib/libllama-*.dylib

# Remove configuration
sudo rm -rf /usr/local/etc/kolosal

# Remove user data (optional)
rm -rf ~/Library/Application\ Support/Kolosal
rm -rf ~/Library/Logs/Kolosal
rm -rf ~/Library/Caches/Kolosal
```

### macOS Manual Verification

Check installation status and files:
```bash
# Check if application bundle exists
ls -la "/Applications/Kolosal CLI.app"

# Check system binaries
which kolosal
which kolosal-server

# Check library dependencies
otool -L /usr/local/bin/kolosal 2>/dev/null || echo "Binary not found in system path"

# Check system configuration
ls -la /usr/local/etc/kolosal/

# Check user configuration directory
ls -la ~/Library/Application\ Support/Kolosal/

# Check user logs
ls -la ~/Library/Logs/Kolosal/

# Check if service is running (if installed)
sudo launchctl list | grep kolosal

# Verify version and functionality
kolosal --version
kolosal --help
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
- ✅ **macOS** - Fully supported with detailed build instructions (Intel and Apple Silicon)

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

#### macOS Build Issues

1. **Xcode Command Line Tools not installed:**
   ```bash
   # Install Xcode Command Line Tools
   xcode-select --install
   
   # If already installed but having issues, reset it
   sudo xcode-select --reset
   sudo xcode-select --install
   
   # Verify installation
   xcode-select -p  # Should output: /Applications/Xcode.app/Contents/Developer or /Library/Developer/CommandLineTools
   ```

2. **Homebrew not found or not in PATH:**
   ```bash
   # Install Homebrew
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Add to PATH for Apple Silicon Macs
   echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
   source ~/.zshrc
   
   # Add to PATH for Intel Macs
   echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zshrc
   source ~/.zshrc
   
   # Verify installation
   brew --version
   ```

3. **CMake version too old:**
   ```bash
   # Update Homebrew and install latest CMake
   brew update
   brew install cmake
   
   # Or install from official CMake website
   # Download from: https://cmake.org/download/
   
   # Verify version
   cmake --version  # Should be >= 3.14
   ```

4. **OpenSSL linking issues:**
   ```bash
   # Install OpenSSL via Homebrew
   brew install openssl
   
   # Configure CMake with OpenSSL path
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
            -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib
   
   # For persistent configuration, add to shell profile
   echo 'export OPENSSL_ROOT_DIR=$(brew --prefix openssl)' >> ~/.zshrc
   source ~/.zshrc
   ```

5. **Metal framework not found (Apple Silicon):**
   ```bash
   # Ensure you're on macOS 10.13+ with Metal support
   system_profiler SPSoftwareDataType | grep "System Version"
   
   # For Apple Silicon Macs, enable Metal acceleration
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DUSE_METAL=ON \
            -DCMAKE_OSX_ARCHITECTURES=arm64
   
   # If Metal still not found, disable it
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DUSE_METAL=OFF
   ```

6. **Architecture mismatch issues:**
   ```bash
   # Check your Mac's architecture
   uname -m  # arm64 for Apple Silicon, x86_64 for Intel
   
   # For Apple Silicon Macs
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_ARCHITECTURES=arm64
   
   # For Intel Macs
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_ARCHITECTURES=x86_64
   
   # Clean build directory if switching architectures
   rm -rf build && mkdir build && cd build
   ```

7. **Permission denied errors:**
   ```bash
   # Fix permissions for build directory
   chmod -R 755 ./build
   
   # Fix permissions for the executable
   chmod +x ./build/kolosal-cli
   
   # If installing system-wide, you may need sudo
   sudo make install
   ```

8. **Server fails to start on macOS:**
   ```bash
   # Check if port is available
   lsof -i :8080
   
   # Check macOS firewall settings
   sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate
   
   # Allow kolosal-cli through firewall if needed
   sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add ./kolosal-cli
   sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock ./kolosal-cli
   ```

9. **Rosetta 2 issues (Apple Silicon running Intel code):**
   ```bash
   # Install Rosetta 2 if needed
   softwareupdate --install-rosetta
   
   # Force Intel build on Apple Silicon if needed
   arch -x86_64 cmake .. -DCMAKE_BUILD_TYPE=Release \
                         -DCMAKE_OSX_ARCHITECTURES=x86_64
   arch -x86_64 make -j$(sysctl -n hw.ncpu)
   ```

10. **macOS version compatibility issues:**
    ```bash
    # Check macOS version
    sw_vers -productVersion
    
    # Set deployment target for older macOS versions
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15  # Adjust as needed
    
    # For macOS 11.0+ (recommended for full feature support)
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    ```

#### macOS Packaging Issues

**Common Issues and Quick Solutions:**

If you're experiencing multiple packaging errors, try this comprehensive fix first:

```bash
# Complete clean and rebuild with error-free configuration
cd ..
rm -rf build build-release

# Create docs directory and disable problematic features
mkdir -p docs

# Start fresh with minimal configuration
mkdir build && cd build

# Configure for user installation with all problematic features disabled
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local" \
         -DCMAKE_OSX_ARCHITECTURES=$(uname -m) \
         -DCPACK_DMG_BACKGROUND_IMAGE="" \
         -DCPACK_BUNDLE_STARTUP_COMMAND=""

# Suppress ranlib warnings and build
export RANLIB="ranlib -no_warning_for_no_symbols"
make -j$(sysctl -n hw.ncpu)
make package
```

**Specific Error Solutions:**

**Config File Path Error (`Error copying file...config.apple.yaml`):**
```bash
# This error occurs when the config file path is incorrectly constructed
# The CMakeLists.txt has been updated to use the config.yaml already built in bin/

# Simply rebuild - the config file issue is now resolved in the CMakeLists.txt
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
make -j$(sysctl -n hw.ncpu)
make package
```

**DMG Background Image Error (`Error copying disk volume background image`):**
```bash
# This error occurs when the DMG background image is missing or inaccessible
# Quick fix: Disable the background image entirely

# Method 1: Disable background image
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCPACK_DMG_BACKGROUND_IMAGE="" \
         -DCMAKE_INSTALL_PREFIX="$HOME/local"
make -j$(sysctl -n hw.ncpu)
make package

# Method 2: Create the missing docs directory and background image
mkdir -p docs
# Create a simple background image (requires ImageMagick or skip this step)
convert -size 600x400 xc:lightblue docs/dmg_background.png 2>/dev/null || \
echo "Background image creation skipped - DMG will use default background"

**Directory Compression Error (`Problem compressing the directory`):**
```bash
# This error often follows from permission issues or corrupted package structure
# Complete reset and rebuild approach:

# Step 1: Clean everything
cd ..
sudo rm -rf build build-release _CPack_Packages
git clean -fdx build*/  # Remove any build artifacts

# Step 2: Check disk space and permissions
df -h .  # Ensure sufficient disk space (at least 2GB free)
ls -la . # Check directory permissions

# Step 3: Rebuild with fresh configuration
mkdir build && cd build

# Use the most compatible configuration
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_DMG_BACKGROUND_IMAGE="" \
         -DCPACK_GENERATOR="DragNDrop" \
         -DCPACK_DMG_FORMAT="UDZO"

# Step 4: Build and package with verbose output
make -j$(sysctl -n hw.ncpu) VERBOSE=1
make package VERBOSE=1

# Alternative: Create ZIP package instead of DMG
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/local" \
         -DCPACK_GENERATOR="ZIP"
make -j$(sysctl -n hw.ncpu)
make package
```

**Permission Issues During Packaging:**
```bash
# If you get permission denied errors during packaging:

# Option 1: Fix permissions recursively
chmod -R 755 build/
sudo chown -R $USER:staff build/

# Option 2: Use completely user-local installation
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
         -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/.local" \
         -DCPACK_SET_DESTDIR=OFF

# Option 3: Skip system directories entirely
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DSKIP_INSTALL_FILES=ON \
         -DCPACK_COMPONENTS_ALL="Runtime"
make package

# Option 4: Use sudo only for packaging (not recommended for development)
make -j$(sysctl -n hw.ncpu)
sudo make package
```

**Individual Issue Solutions:**

1. **DMG creation fails due to permission errors:**
   ```bash
   # If you get "Maybe need administrative privileges" errors:
   # Use sudo for packaging when system directories are involved
   sudo make package
   
   # Or configure for user-only installation (recommended for development)
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
   make -j$(sysctl -n hw.ncpu)
   make package
   
   # Alternative: Build without system integration
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DSKIP_SYSTEM_INSTALL=ON
   make -j$(sysctl -n hw.ncpu)
   make package
   ```

2. **DMG creation fails due to missing dependencies:**
   ```bash
   # Ensure all dependencies are properly linked
   otool -L ./kolosal-cli
   
   # Check for missing dylib files
   ldd ./kolosal-cli 2>/dev/null || echo "ldd not available on macOS, use otool -L instead"
   
   # Rebuild with proper library paths
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE
   make -j$(sysctl -n hw.ncpu)
   make package
   ```

3. **Ranlib warnings about "no symbols" and architecture mismatch:**
   ```bash
   # These warnings are generally harmless but can be suppressed
   # They occur when object files are empty for certain architectures
   # The x86_64 warnings while building arm64 are due to universal binary remnants
   
   # To suppress warnings and fix architecture issues:
   export RANLIB="ranlib -no_warning_for_no_symbols"
   
   # Ensure clean architecture-specific build
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_ARCHITECTURES=$(uname -m) \
            -DCMAKE_INSTALL_PREFIX="$HOME/local"
   make clean  # Clean any mixed architecture artifacts
   make -j$(sysctl -n hw.ncpu)
   make package
   
   # Alternative: Force architecture and suppress warnings completely
   rm -rf CMakeFiles CMakeCache.txt
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -DCMAKE_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
   AR="ar" RANLIB="ranlib -no_warning_for_no_symbols" make -j$(sysctl -n hw.ncpu)
   make package
   ```

4. **DMG background image errors:**
   ```bash
   # If you get "Error copying disk volume background image" errors:
   # Create missing docs directory and background image
   mkdir -p docs
   
   # Create a simple background image or disable it
   # Option 1: Create a simple solid color background
   convert -size 600x400 xc:lightgray docs/dmg_background.png 2>/dev/null || \
   sips -s format png --setProperty pixelWidth 600 --setProperty pixelHeight 400 \
        --setProperty hasAlpha no --fillColor lightgray \
        /System/Library/ColorSync/Profiles/Generic\ Gray\ Gamma\ 2.2\ Profile.icc \
        docs/dmg_background.png 2>/dev/null || \
   echo "Background image creation failed, will disable DMG background"
   
   # Option 2: Disable DMG background image
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCPACK_DMG_BACKGROUND_IMAGE=""
   make -j$(sysctl -n hw.ncpu)
   make package
   
   # Option 3: Use a different image format or path
   # Place any PNG image (600x400 recommended) at docs/dmg_background.png
   ```

5. **Configuration file path issues:**
   ```bash
   # If you get config file copying errors with wrong paths:
   # Clean and rebuild with proper path configuration
   rm -rf build
   mkdir build && cd build
   
   # Ensure config.apple.yaml exists in the source directory
   ls -la ../config.apple.yaml || echo "Warning: config.apple.yaml not found"
   
   # Configure with explicit paths
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local"
   make -j$(sysctl -n hw.ncpu)
   make package
   ```

6. **Complete clean rebuild for persistent issues:**
   ```bash
   # If packaging continues to fail, do a complete clean rebuild
   cd ..
   rm -rf build build-release
   git clean -fdx  # Warning: removes all untracked files
   
   # Create fresh build
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_PACKAGING_INSTALL_PREFIX="$HOME/local" \
            -DCPACK_DMG_BACKGROUND_IMAGE=""  # Disable background
   make -j$(sysctl -n hw.ncpu)
   make package
   ```

2. **Code signing issues (for distribution):**
   ```bash
   # Check if you have a developer certificate
   security find-identity -v -p codesigning
   
   # Sign the application manually if needed
   codesign --force --deep --sign "Developer ID Application: Your Name" ./kolosal-cli
   
   # Verify signature
   codesign --verify --verbose ./kolosal-cli
   
   # For distribution, notarize with Apple
   xcrun notarytool submit kolosal-1.0.0-apple-silicon.dmg \
                           --apple-id your-email@example.com \
                           --password your-app-specific-password \
                           --team-id YOUR_TEAM_ID
   ```

3. **Bundle structure issues:**
   ```bash
   # Check bundle structure
   ls -la /Applications/Kolosal\ CLI.app/Contents/
   
   # Verify Info.plist
   plutil -p /Applications/Kolosal\ CLI.app/Contents/Info.plist
   
   # Fix bundle if needed
   mkdir -p MyApp.app/Contents/{MacOS,Resources}
   cp kolosal-cli MyApp.app/Contents/MacOS/
   cp Info.plist MyApp.app/Contents/
   ```

4. **Library dependency issues:**
   ```bash
   # Check all library dependencies
   otool -L ./kolosal-cli
   
   # Fix library paths using install_name_tool
   install_name_tool -change old_path new_path ./kolosal-cli
   
   # Example: Fix libkolosal_server.dylib path
   install_name_tool -change libkolosal_server.dylib \
                           @executable_path/libkolosal_server.dylib \
                           ./kolosal-cli
   ```

5. **Universal binary issues:**
   ```bash
   # Check architecture of binary
   lipo -info ./kolosal-cli
   
   # Create universal binary manually if needed
   lipo -create -arch arm64 ./kolosal-cli-arm64 \
               -arch x86_64 ./kolosal-cli-x86_64 \
               -output ./kolosal-cli-universal
   
   # Verify universal binary
   lipo -detailed_info ./kolosal-cli-universal
   ```

6. **DMG mounting issues:**
   ```bash
   # Check DMG integrity
   hdiutil verify kolosal-1.0.0-apple-silicon.dmg
   
   # Manually mount DMG for debugging
   hdiutil attach kolosal-1.0.0-apple-silicon.dmg -readonly
   
   # Check mounted volume
   ls -la /Volumes/Kolosal\ CLI/
   
   # Unmount when done
   hdiutil detach /Volumes/Kolosal\ CLI/
   ```

7. **Gatekeeper and quarantine issues:**
   ```bash
   # Remove quarantine attribute if needed
   xattr -rd com.apple.quarantine /Applications/Kolosal\ CLI.app
   
   # Check Gatekeeper status
   spctl --status
   
   # Test Gatekeeper assessment
   spctl --assess --type execute /Applications/Kolosal\ CLI.app
   
   # Temporarily disable Gatekeeper (not recommended for production)
   sudo spctl --master-disable
   ```

8. **Installation permission issues:**
   ```bash
   # Install to user Applications folder instead of system
   cp -R Kolosal\ CLI.app ~/Applications/
   
   # Or install with proper permissions
   sudo cp -R Kolosal\ CLI.app /Applications/
   sudo chown -R root:admin /Applications/Kolosal\ CLI.app
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