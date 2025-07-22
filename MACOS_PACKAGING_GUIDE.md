# macOS Packaging Guide for Kolosal CLI

## Overview
The macOS packaging creates a DMG installer that contains all the necessary executables and libraries. Users can extract the contents and run an installation script for easy terminal access.

## New Structure

### DMG Contents
```
Install Kolosal CLI (DMG Volume)/
├── bin/
│   ├── kolosal                          # CLI executable
│   ├── kolosal-server                   # Server executable
│   └── kolosal-launcher                 # Installation script
├── lib/
│   ├── libkolosal_server.dylib          # Server library
│   ├── libllama-metal.dylib             # Metal inference engine
│   ├── libllama-vulkan.dylib            # Vulkan inference engine
│   ├── libllama-cuda.dylib              # CUDA inference engine
│   └── libllama-cpu.dylib               # CPU inference engine
├── etc/
│   └── config.yaml                      # Default configuration
└── Applications -> /Applications        # Symlink for easy access
```

### Installation Process

1. **User downloads DMG file** (e.g., `kolosal-1.0.0-apple-silicon.dmg`)
2. **User opens DMG** - Mounts the disk image showing the contents
3. **User runs installation** - Either:
   - Run the automated installation script: `./bin/kolosal-launcher`
   - Or manually copy files and set up paths (see manual installation below)

### Automatic Installation

The `kolosal-launcher` script handles:
- Creates symlinks in `/usr/local/bin/` for `kolosal` and `kolosal-server`
- Updates shell profiles (`.zshrc`, `.bash_profile`, etc.) to include `/usr/local/bin` in PATH
- Creates user directories in `~/Library/Application Support/Kolosal/`
- Copies config file to user directory
- Shows completion message with usage instructions

### Terminal Access

After installation, users can immediately use:
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
