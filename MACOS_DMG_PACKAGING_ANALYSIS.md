# macOS DMG Packaging Analysis and Improvements

## Overview

I've analyzed the CMakeLists.txt files for both kolosal-cli and kolosal-server and identified several critical issues with the current macOS packaging setup. The configuration was set up for PKG installers but you need a DMG package. I've made comprehensive improvements to ensure proper DMG creation with all required libraries.

## Key Issues Found and Fixed

### 1. **Wrong Package Generator**
- **Issue**: `CPACK_GENERATOR` was set to "productbuild" (PKG installer)
- **Fix**: Changed to "DragNDrop" for proper DMG creation

### 2. **Inconsistent Library Installation Structure**
- **Issue**: Libraries were installed to both `lib/` and `bin/` directories
- **Fix**: Standardized structure with `bin/` for executables, `lib/` for libraries, `etc/` for configuration

### 3. **Missing DMG-Specific Configuration**
- **Issue**: No DMG volume settings, background image, or AppleScript setup
- **Fix**: Added complete DMG configuration with volume name, compression, and layout script

### 4. **Incorrect RPATH Configuration**
- **Issue**: RPATH was set for PKG installation paths
- **Fix**: Updated RPATH to use `@executable_path/../lib` for self-contained DMG structure

### 5. **Missing Installation Launcher**
- **Issue**: No user-friendly installation method
- **Fix**: Created `kolosal-launcher` script for easy installation from DMG

## Updated DMG Structure

The new DMG will have this structure:
```
Kolosal CLI [VERSION] (DMG Volume)/
├── bin/
│   ├── kolosal                    # Main CLI executable
│   └── kolosal-server             # Server executable
├── lib/
│   ├── libkolosal_server.dylib    # Server library
│   ├── libllama-metal.dylib       # Metal inference engine
│   ├── libllama-vulkan.dylib      # Vulkan inference engine (if available)
│   ├── libllama-cuda.dylib        # CUDA inference engine (if available)
│   ├── libllama-cpu.dylib         # CPU inference engine
│   └── ggml-*.metal               # Metal shader files (if using Metal)
├── etc/
│   └── config.yaml                # Default configuration
├── kolosal-launcher               # Installation script
├── README.md                      # Documentation
└── LICENSE                       # License file
```

## Installation Flow

1. **User downloads DMG** (e.g., `kolosal-1.0.0-apple-silicon.dmg`)
2. **User mounts DMG** - Shows the structured contents
3. **User runs installer** - Executes `./kolosal-launcher`
4. **Automated installation**:
   - Detects admin privileges (system-wide vs. user installation)
   - Copies files to appropriate locations
   - Sets up command-line symlinks
   - Configures PATH in shell profiles
   - Creates user configuration directories
   - Sets proper permissions

## Key Improvements Made

### 1. **CMakeLists.txt Changes**
```cmake
# Changed from PKG to DMG
set(CPACK_GENERATOR "DragNDrop")

# Added DMG-specific settings
set(CPACK_DMG_VOLUME_NAME "Kolosal CLI ${PROJECT_VERSION}")
set(CPACK_DMG_FORMAT "UDBZ")  # Compressed format
set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK OFF)
```

### 2. **RPATH Fixes**
- Updated all RPATH settings to use `@executable_path/../lib`
- Added install-time RPATH fixing with `install_name_tool`
- Ensured all dylibs have proper install names

### 3. **Complete Library Discovery**
- Enhanced detection of inference engine libraries
- Added fallback paths for kolosal-server build output
- Ensured Metal shader files are included

### 4. **Installation Launcher Script**
- Smart detection of admin privileges
- User vs. system-wide installation options
- Automatic PATH configuration
- User directory setup
- Proper symlink creation

## Files Added/Modified

### New Files:
- `scripts/macos/setup_dmg.applescript` - DMG layout configuration
- `scripts/macos/build_dmg.sh` - Build script for DMG creation
- `resources/dmg_background.png.txt` - Placeholder for background image

### Modified Files:
- `CMakeLists.txt` - Complete overhaul of macOS packaging
- `kolosal-server/CMakeLists.txt` - RPATH improvements (existing settings were good)

## Build Instructions

1. **Use the provided build script**:
   ```bash
   ./scripts/macos/build_dmg.sh
   ```

2. **Or build manually**:
   ```bash
   mkdir build-dmg && cd build-dmg
   cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_METAL=ON
   make -j$(sysctl -n hw.ncpu)
   make install
   cpack
   ```

## Testing the DMG

1. Mount the created DMG file
2. Run `./kolosal-launcher` from the mounted volume
3. Test commands: `kolosal --help` and `kolosal-server --help`
4. Verify configuration at `~/Library/Application Support/Kolosal/config.yaml`

## Required Dependencies

Ensure all required libraries are properly bundled or linked:

### Core Libraries (Bundled):
- ✅ `libkolosal_server.dylib`
- ✅ `libllama-metal.dylib` (Metal acceleration)
- ✅ `libllama-vulkan.dylib` (Vulkan acceleration)
- ✅ `libllama-cpu.dylib` (CPU fallback)
- ✅ `yaml-cpp` (static linked)
- ✅ `zlib/minizip` (static linked)

### System Dependencies (User must have):
- macOS 10.14+ (configured deployment target)
- Metal framework (built-in on macOS 10.13+)
- Vulkan SDK (optional, for Vulkan acceleration)

## Next Steps

1. **Create DMG background image**: Replace `resources/dmg_background.png.txt` with actual PNG
2. **Test on different macOS versions**: Verify compatibility
3. **Add code signing**: For distribution outside of developer testing
4. **Optimize AppleScript**: Fine-tune DMG window layout

The packaging is now correctly configured for DMG creation with all required libraries properly bundled and RPATH settings optimized for self-contained distribution.
