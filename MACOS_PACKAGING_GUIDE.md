# macOS Packaging Guide for Kolosal CLI

## Overview
The macOS packaging creates a DMG installer that contains a macOS app bundle (`Kolosal.app`). When users double-click the app, it offers to set up command line access automatically.

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
│   │   │   └── kolosal-launcher.sh      # Setup launcher script
│   │   ├── Frameworks/                  # Shared libraries
│   │   │   ├── libkolosal_server.dylib
│   │   │   ├── libllama-metal.dylib
│   │   │   ├── libllama-vulkan.dylib
│   │   │   ├── libllama-cuda.dylib
│   │   │   └── libllama-cpu.dylib
│   │   └── Resources/                   # App resources
│   │       ├── config.yaml              # Default configuration
│   │       ├── kolosal.icns             # App icon
│   │       ├── README.md                # Documentation
│   │       └── LICENSE                  # License file
└── Applications -> /Applications        # Symlink for easy access
```

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
