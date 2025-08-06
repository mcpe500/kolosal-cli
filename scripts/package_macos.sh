#!/bin/bash

# Script to create a self-contained macOS package for Kolosal CLI
# This script handles the complete build and packaging process including dependency bundling

set -e

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
SKIP_BUILD=false
CREATE_DMG=true
INSTALL_DIR="install"
BUILD_DIR="build"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Build and package Kolosal CLI for macOS with all dependencies"
    echo ""
    echo "Options:"
    echo "  -t, --build-type TYPE    Build type: Release or Debug (default: Release)"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -s, --skip-build        Skip building, only package existing build"
    echo "  -n, --no-dmg            Don't create DMG, only create app bundle"
    echo "  -b, --build-dir DIR     Build directory (default: build)"
    echo "  -i, --install-dir DIR   Install directory (default: install)"
    echo "  -h, --help              Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -s|--skip-build)
            SKIP_BUILD=true
            shift
            ;;
        -n|--no-dmg)
            CREATE_DMG=false
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -i|--install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

echo "=== Kolosal CLI macOS Packaging Script ==="
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"
echo "Install directory: $INSTALL_DIR"
echo "Clean build: $CLEAN_BUILD"
echo "Skip build: $SKIP_BUILD"
echo "Create DMG: $CREATE_DMG"
echo ""

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "Error: This script is for macOS only."
    exit 1
fi

# Check for required tools
echo "=== Checking for required tools ==="
required_tools=("cmake" "make" "otool" "install_name_tool")
for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
        echo "Error: $tool is not installed or not in PATH"
        exit 1
    else
        echo "✓ $tool found"
    fi
done

# Check for optional tools
if $CREATE_DMG; then
    if ! command -v "hdiutil" &> /dev/null; then
        echo "Warning: hdiutil not found, disabling DMG creation"
        CREATE_DMG=false
    else
        echo "✓ hdiutil found"
    fi
fi

echo ""

# Check for required dependencies
echo "=== Checking for required dependencies ==="
required_libs=(
    "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib"
    "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib"
    "/opt/homebrew/opt/freetype/lib/libfreetype.6.dylib"
    "/opt/homebrew/opt/fontconfig/lib/libfontconfig.1.dylib"
    "/opt/homebrew/opt/libpng/lib/libpng16.16.dylib"
    "/opt/homebrew/opt/libtiff/lib/libtiff.6.dylib"
    "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib"
)

missing_deps=()
for lib in "${required_libs[@]}"; do
    if [[ -f "$lib" ]]; then
        echo "✓ $(basename "$lib") found"
    else
        missing_deps+=("$lib")
        echo "✗ $(basename "$lib") missing"
    fi
done

if [[ ${#missing_deps[@]} -gt 0 ]]; then
    echo ""
    echo "Error: Missing dependencies. Please install with:"
    echo "  brew install openssl@3 freetype fontconfig libpng libtiff jpeg-turbo zstd xz"
    exit 1
fi

echo ""

# Clean build if requested
if $CLEAN_BUILD && [[ -d "$BUILD_DIR" ]]; then
    echo "=== Cleaning build directory ==="
    rm -rf "$BUILD_DIR"
    echo "Build directory cleaned"
    echo ""
fi

# Build the project
if ! $SKIP_BUILD; then
    echo "=== Building Kolosal CLI ==="
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure
    echo "Configuring with CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR" \
        -DUSE_METAL=ON \
        -DBUILD_INFERENCE_ENGINE=ON
    
    # Build
    echo "Building..."
    make -j$(sysctl -n hw.ncpu)
    
    cd ..
    echo "Build completed"
    echo ""
else
    echo "=== Skipping build (--skip-build specified) ==="
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "Error: Build directory does not exist. Remove --skip-build or run build first."
        exit 1
    fi
    echo ""
fi

# Install/Package
echo "=== Installing and packaging ==="

# Clean install directory
if [[ -d "$INSTALL_DIR" ]]; then
    rm -rf "$INSTALL_DIR"
fi

# Install
cd "$BUILD_DIR"
make install
cd ..

echo "Installation completed"
echo ""

# Verify the installation
echo "=== Verifying installation ==="
# Check if DMG staging directory exists (macOS CMake sets install prefix to staging area)
if [[ -d "/tmp/kolosal-dmg-staging/Kolosal.app" ]]; then
    app_bundle="/tmp/kolosal-dmg-staging/Kolosal.app"
    echo "✓ Found app bundle in DMG staging area: $app_bundle"
elif [[ -d "$INSTALL_DIR/Kolosal.app" ]]; then
    app_bundle="$INSTALL_DIR/Kolosal.app"
    echo "✓ Found app bundle in install directory: $app_bundle"
else
    echo "Error: App bundle not found in expected locations:"
    echo "  - /tmp/kolosal-dmg-staging/Kolosal.app"
    echo "  - $INSTALL_DIR/Kolosal.app"
    exit 1
fi

frameworks_dir="$app_bundle/Contents/Frameworks"
macos_dir="$app_bundle/Contents/MacOS"

echo "✓ App bundle created at $app_bundle"

# Check for main executables
main_executables=("kolosal" "kolosal-server" "kolosal-launcher")
for exe in "${main_executables[@]}"; do
    if [[ -f "$macos_dir/$exe" ]]; then
        echo "✓ $exe found in app bundle"
    else
        echo "✗ $exe missing from app bundle"
    fi
done

# Check for main libraries
main_libraries=("libkolosal_server.dylib" "libllama-metal.dylib")
for lib in "${main_libraries[@]}"; do
    if [[ -f "$frameworks_dir/$lib" ]]; then
        echo "✓ $lib found in Frameworks"
    else
        echo "⚠️  $lib missing from Frameworks"
    fi
done

# Count external dependencies
external_lib_count=0
if [[ -d "$frameworks_dir" ]]; then
    external_lib_count=$(find "$frameworks_dir" -name "*.dylib" -not -name "libkolosal_server.dylib" -not -name "libllama-*.dylib" | wc -l | xargs)
fi

echo "✓ $external_lib_count external libraries bundled"
echo ""

# Run dependency check if the script exists
if [[ -f "scripts/check_macos_dependencies.sh" ]]; then
    echo "=== Running dependency check ==="
    ./scripts/check_macos_dependencies.sh -f "$frameworks_dir"
    echo ""
fi

# Create DMG if requested
if $CREATE_DMG; then
    echo "=== Creating DMG ==="
    
    dmg_name="kolosal-cli-$(date +%Y%m%d)-macos-$(uname -m).dmg"
    temp_dmg="temp-$dmg_name"
    
    # Remove existing DMG files
    [[ -f "$dmg_name" ]] && rm "$dmg_name"
    [[ -f "$temp_dmg" ]] && rm "$temp_dmg"
    
    # Calculate the size needed (app bundle size + 50MB buffer)
    if command -v "du" &> /dev/null; then
        app_size=$(du -sm "$app_bundle" | cut -f1)
        dmg_size=$((app_size + 50))
    else
        dmg_size=500
    fi
    
    echo "Creating DMG with size ${dmg_size}MB..."
    
    # Create temporary DMG
    hdiutil create -size "${dmg_size}m" -fs HFS+ -volname "Kolosal CLI" "$temp_dmg"
    
    # Mount the DMG
    mount_point=$(hdiutil attach "$temp_dmg" | grep "Apple_HFS" | awk '{print $3}')
    
    # Copy the app bundle
    cp -R "$app_bundle" "$mount_point/"
    
    # Create a symbolic link to Applications
    ln -s /Applications "$mount_point/Applications"
    
    # Unmount the DMG
    hdiutil detach "$mount_point"
    
    # Convert to compressed read-only DMG
    hdiutil convert "$temp_dmg" -format UDZO -o "$dmg_name"
    
    # Remove temporary DMG
    rm "$temp_dmg"
    
    echo "✓ DMG created: $dmg_name"
    
    # Get DMG size
    if [[ -f "$dmg_name" ]]; then
        dmg_size_actual=$(du -sh "$dmg_name" | cut -f1)
        echo "  DMG size: $dmg_size_actual"
    fi
    
    echo ""
fi

# Final summary
echo "=== PACKAGING COMPLETE ==="
echo "App bundle: $app_bundle"
if $CREATE_DMG && [[ -f "$dmg_name" ]]; then
    echo "DMG file: $dmg_name"
fi

echo ""
echo "The app bundle contains:"
echo "  • Main executables: kolosal, kolosal-server, kolosal-launcher"
echo "  • All required external libraries bundled in Frameworks/"
echo "  • Properly configured RPATHs for self-contained deployment"
echo ""

echo "You can now:"
echo "  1. Test the app bundle: open $app_bundle"
if $CREATE_DMG && [[ -f "$dmg_name" ]]; then
    echo "  2. Distribute the DMG: $dmg_name"
fi
echo "  3. Move the app to /Applications for system-wide installation"

echo ""
echo "🎉 macOS packaging completed successfully!"
