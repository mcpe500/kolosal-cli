# macOS Packaging Guide for Kolosal CLI

## Overview
The macOS packaging creates a DMG installer that contains a macOS app bundle (`Kolosal.app`). When users double-click the app, it offers to set up command line access automatically. The app bundle includes all necessary external dependencies for a completely self-contained installation.

## New Structure

### DMG Contents
```
Kolosal [VERSION] (DMG Volume)/
├── Kolosal.app/                         # macOS App Bundle
│   ├── Contents/
│   │   ├── Info.plist                   # Bundle metadata
│   │   ├── MacOS/
│   │   │   ├── kolosal                  # CLI executable
│   │   │   ├── kolosal-server           # Server executable
│   │   │   ├── kolosal-launcher         # Native launcher executable
│   │   │   └── kolosal-launcher.sh      # Setup launcher script
│   │   ├── Frameworks/                  # Shared libraries (self-contained)
│   │   │   ├── libkolosal_server.dylib  # Main server library
│   │   │   ├── libllama-metal.dylib     # Inference engine
│   │   │   ├── libssl.3.dylib           # OpenSSL SSL library
│   │   │   ├── libcrypto.3.dylib        # OpenSSL Crypto library
│   │   │   ├── libfreetype.6.dylib      # FreeType font library
│   │   │   ├── libfontconfig.1.dylib    # Font configuration library
│   │   │   ├── libpng16.16.dylib        # PNG image library
│   │   │   ├── libtiff.6.dylib          # TIFF image library
│   │   │   ├── libjpeg.8.dylib          # JPEG image library
│   │   │   ├── libzstd.1.dylib          # Zstandard compression
│   │   │   └── liblzma.5.dylib          # LZMA compression
│   │   └── Resources/                   # App resources
│   │       ├── config.yaml              # Default configuration
│   │       ├── kolosal.icns             # App icon
│   │       ├── README.md                # Documentation
│   │       └── LICENSE                  # License file
└── Applications -> /Applications        # Symlink for easy access
```

## Self-Contained Dependency Bundling

The macOS package is designed to be completely self-contained, including all necessary external libraries. This ensures the application works on any macOS system without requiring users to install additional dependencies.

### Bundled External Dependencies

The following external libraries are automatically bundled and their RPATHs fixed during packaging:

1. **OpenSSL Libraries** (`libssl.3.dylib`, `libcrypto.3.dylib`)
   - Required for: HTTPS connections, cryptographic operations, PDF signing (PoDoFo)
   - Source: Homebrew openssl@3 package

2. **Font and Text Libraries**
   - **FreeType** (`libfreetype.6.dylib`): Font rendering and typography
   - **FontConfig** (`libfontconfig.1.dylib`): Font configuration and matching
   - Source: Homebrew freetype and fontconfig packages

3. **Image Libraries**
   - **LibPNG** (`libpng16.16.dylib`): PNG image support for PDF processing
   - **LibTIFF** (`libtiff.6.dylib`): TIFF image support for PDF processing
   - **JPEG** (`libjpeg.8.dylib`): JPEG image support for PDF processing
   - Source: Homebrew libpng, libtiff, and jpeg-turbo packages

4. **Compression Libraries**
   - **Zstd** (`libzstd.1.dylib`): Zstandard compression (dependency of LibTIFF)
   - **LZMA** (`liblzma.5.dylib`): LZMA compression (dependency of LibTIFF)
   - Source: Homebrew zstd and xz packages

### RPATH Configuration

All bundled libraries have their RPATHs automatically fixed to use `@rpath` references, ensuring they can find each other within the app bundle:

- Main executables use `@executable_path/../Frameworks` to find libraries
- Libraries use `@rpath/libname.dylib` for inter-library dependencies
- All external paths (`/opt/homebrew/*`, `/usr/local/*`) are rewritten to use `@rpath`

### Build-Time Dependency Resolution

The packaging system automatically:
1. Detects which external libraries are linked by the main binaries
2. Copies them from Homebrew installation paths to the app bundle
3. Recursively processes their dependencies
4. Fixes all RPATHs to create a self-contained bundle

### Installation Process

1. **User downloads DMG file** (e.g., `kolosal-1.0.0-apple-silicon.dmg`)
2. **User opens DMG** - Mounts the disk image showing `Kolosal.app`
3. **User drags app to Applications** - Standard macOS app installation
4. **User double-clicks Kolosal.app** - Triggers command line setup

### Automatic Command Line Setup

When the user double-clicks `Kolosal.app`, the `kolosal-launcher.sh` script:

1. **Checks current setup** - Verifies if symlinks already exist
2. **Shows setup dialog** - Asks user if they want command line access
3. **Creates symlinks** - Places `kolosal` and `kolosal-server` in `/usr/local/bin/`
4. **Shows completion** - Offers to open Terminal to test commands

### User Experience

#### First Launch
- User sees dialog: "Welcome to Kolosal CLI! To use Kolosal commands from Terminal..."
- Options: "Skip" or "Set Up Command Line Access"
- If setup chosen: Prompts for admin password, creates symlinks
- Shows success dialog with option to open Terminal

#### Subsequent Launches
- If already set up: Shows "Commands are ready!" with option to open Terminal
- If setup incomplete: Re-offers setup process

### Terminal Access

After setup, users can use from any Terminal:
```bash
kolosal --help
kolosal-server --help
```

### Key Improvements

1. **Single DMG file** - No external scripts or complex installation procedures
2. **Standard macOS experience** - Drag-and-drop to Applications folder
3. **Automatic PATH setup** - Commands available in terminal immediately
4. **User-friendly** - Clear setup messages and fallback instructions
5. **Self-contained** - All dependencies bundled within the app
6. **Proper permissions** - Uses sudo only when necessary for symlink creation

### Build Commands

To build the macOS package:
```bash
# Configure for macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Create DMG package
cd build
cpack
```

This will generate:
- `kolosal-1.0.0-apple-silicon.dmg` (on Apple Silicon Macs)
- `kolosal-1.0.0-intel-mac.dmg` (on Intel Macs)

### Command Line Installation

You can also install the DMG directly from the terminal:

```bash
# Mount the DMG file
hdiutil mount kolosal-1.0.0-apple-silicon.dmg

# Check what's in the mounted volume
ls -la "/Volumes/Install Kolosal CLI/"

# Run the automated installation script
"/Volumes/Install Kolosal CLI/bin/kolosal-launcher"

# Unmount the DMG
hdiutil unmount "/Volumes/Install Kolosal CLI"
```

**Manual Installation** (if you prefer full control):

```bash
# Mount the DMG file
hdiutil mount kolosal-1.0.0-apple-silicon.dmg

# Create installation directory
sudo mkdir -p /usr/local/kolosal

# Copy all files to installation directory
sudo cp -R "/Volumes/Install Kolosal CLI/bin" /usr/local/kolosal/
sudo cp -R "/Volumes/Install Kolosal CLI/lib" /usr/local/kolosal/
sudo cp -R "/Volumes/Install Kolosal CLI/etc" /usr/local/kolosal/

# Create symlinks for terminal access
sudo ln -sf /usr/local/kolosal/bin/kolosal /usr/local/bin/kolosal
sudo ln -sf /usr/local/kolosal/bin/kolosal-server /usr/local/bin/kolosal-server

# Add to PATH (if /usr/local/bin isn't already in PATH)
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Create user directories
mkdir -p ~/Library/Application\ Support/Kolosal/{models,logs,cache}

# Copy config file
cp /usr/local/kolosal/etc/config.yaml ~/Library/Application\ Support/Kolosal/

# Unmount the DMG
hdiutil unmount "/Volumes/Install Kolosal CLI"
```

## Packaging Tools and Scripts

Two helper scripts are provided to assist with macOS packaging:

### 1. Automated Packaging Script

`scripts/package_macos.sh` - Complete build and packaging automation:

```bash
# Full build and package with dependency bundling
./scripts/package_macos.sh

# Options:
./scripts/package_macos.sh -t Debug          # Debug build
./scripts/package_macos.sh -c               # Clean build
./scripts/package_macos.sh -n               # No DMG creation
./scripts/package_macos.sh --skip-build     # Package existing build
```

Features:
- Checks for required dependencies and tools
- Builds the project with proper configuration
- Creates self-contained app bundle with all dependencies
- Fixes RPATHs automatically
- Creates distributable DMG file
- Provides comprehensive validation

### 2. Dependency Verification Script

`scripts/check_macos_dependencies.sh` - Validate dependency bundling:

```bash
# Check dependencies in installed app bundle
./scripts/check_macos_dependencies.sh

# Check specific directories
./scripts/check_macos_dependencies.sh -f /path/to/Frameworks -i /path/to/install
```

Features:
- Lists all bundled external libraries
- Checks for missing dependencies
- Validates RPATH configurations
- Identifies remaining external dependencies
- Provides detailed dependency tree analysis

### Build Requirements

For successful packaging, ensure you have:

```bash
# Install required dependencies via Homebrew
brew install openssl@3 freetype fontconfig libpng libtiff jpeg-turbo zstd xz

# Install build tools
xcode-select --install  # For system development tools
brew install cmake      # Build system
```

### Testing the Package

After building, test the self-contained nature:

```bash
# Test that the app works without Homebrew dependencies
# (temporarily rename Homebrew directory)
sudo mv /opt/homebrew /opt/homebrew-disabled

# Test the app
open install/Kolosal.app
# or
install/Kolosal.app/Contents/MacOS/kolosal --version

# Restore Homebrew
sudo mv /opt/homebrew-disabled /opt/homebrew
```

**Troubleshooting**: Check what's actually in the DMG:
```bash
ls -la "/Volumes/Install Kolosal CLI/"
# This will show you the bin/, lib/, and etc/ directories
```

Alternatively, you can run the setup manually:

```bash
# After copying to Applications, run the launcher directly
"/Applications/Kolosal CLI.app/Contents/MacOS/kolosal-launcher"
```

Or install everything manually without the launcher:

```bash
# Create symlinks manually
sudo ln -sf "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal" /usr/local/bin/kolosal
sudo ln -sf "/Applications/Kolosal CLI.app/Contents/MacOS/kolosal-server" /usr/local/bin/kolosal-server

# Add to PATH in your shell profile
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Create user directories
mkdir -p ~/Library/Application\ Support/Kolosal
mkdir -p ~/Library/Application\ Support/Kolosal/models
mkdir -p ~/Library/Logs/Kolosal
mkdir -p ~/Library/Caches/Kolosal/models

# Copy config file
cp "/Applications/Kolosal CLI.app/Contents/Resources/config.yaml" ~/Library/Application\ Support/Kolosal/
```

### Testing

After building, test the DMG by:
1. Mounting the DMG file
2. Dragging the app to Applications (or using command line installation above)
3. Launching the app to verify setup works
4. Testing terminal commands: `kolosal --help` and `kolosal-server --help`

### Compatibility

- **Minimum macOS version**: 10.13 (High Sierra)
- **Architecture support**: Universal (Intel and Apple Silicon)
- **Shell support**: zsh, bash, and other POSIX-compliant shells
