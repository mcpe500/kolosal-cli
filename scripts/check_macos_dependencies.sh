#!/bin/bash

# Script to check if all necessary external dependencies are bundled in the macOS package
# This helps validate that the packaging process includes all required libraries

set -e

# Default values
BUILD_DIR="build"
INSTALL_DIR="install"
FRAMEWORKS_DIR=""

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Check macOS app bundle dependencies"
    echo ""
    echo "Options:"
    echo "  -b, --build-dir DIR      Build directory (default: build)"
    echo "  -i, --install-dir DIR    Install directory (default: install)"
    echo "  -f, --frameworks DIR     Frameworks directory to check (auto-detected if not specified)"
    echo "  -h, --help              Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -i|--install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        -f|--frameworks)
            FRAMEWORKS_DIR="$2"
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

# Auto-detect frameworks directory if not specified
if [[ -z "$FRAMEWORKS_DIR" ]]; then
    if [[ -d "$INSTALL_DIR/Kolosal.app/Contents/Frameworks" ]]; then
        FRAMEWORKS_DIR="$INSTALL_DIR/Kolosal.app/Contents/Frameworks"
    elif [[ -d "$BUILD_DIR/Kolosal.app/Contents/Frameworks" ]]; then
        FRAMEWORKS_DIR="$BUILD_DIR/Kolosal.app/Contents/Frameworks"
    else
        echo "Error: Could not find Frameworks directory. Please specify with -f option."
        exit 1
    fi
fi

# Auto-detect MacOS directory
MACOS_DIR=""
if [[ -n "$FRAMEWORKS_DIR" ]]; then
    # If frameworks directory is provided, derive MacOS directory from it
    # FRAMEWORKS_DIR should be like "/path/to/Kolosal.app/Contents/Frameworks"
    APP_CONTENTS_DIR=$(dirname "$FRAMEWORKS_DIR")
    MACOS_DIR="$APP_CONTENTS_DIR/MacOS"
    if [[ ! -d "$MACOS_DIR" ]]; then
        echo "Error: Could not find MacOS directory at $MACOS_DIR"
        exit 1
    fi
elif [[ -d "$INSTALL_DIR/Kolosal.app/Contents/MacOS" ]]; then
    MACOS_DIR="$INSTALL_DIR/Kolosal.app/Contents/MacOS"
elif [[ -d "$BUILD_DIR/Kolosal.app/Contents/MacOS" ]]; then
    MACOS_DIR="$BUILD_DIR/Kolosal.app/Contents/MacOS"
else
    echo "Error: Could not find MacOS directory in app bundle."
    exit 1
fi

echo "Checking macOS app bundle dependencies..."
echo "Frameworks directory: $FRAMEWORKS_DIR"
echo "MacOS directory: $MACOS_DIR"
echo ""

# Check if required external libraries are present
echo "=== Checking for required external libraries ==="
EXTERNAL_LIBS=(
    "libssl.3.dylib"
    "libcrypto.3.dylib"
    "libfontconfig.1.dylib"
    "libfreetype.6.dylib"
    "libpng16.16.dylib"
    "libtiff.6.dylib"
    "libjpeg.8.dylib"
    "libzstd.1.dylib"
    "liblzma.5.dylib"
)

missing_libs=()
present_libs=()

for lib in "${EXTERNAL_LIBS[@]}"; do
    if [[ -f "$FRAMEWORKS_DIR/$lib" ]]; then
        present_libs+=("$lib")
        echo "✓ $lib - FOUND"
    else
        missing_libs+=("$lib")
        echo "✗ $lib - MISSING"
    fi
done

echo ""

# Check main executables and their dependencies
echo "=== Checking main executables ==="
MAIN_BINARIES=(
    "$MACOS_DIR/kolosal"
    "$MACOS_DIR/kolosal-server"
)

for binary in "${MAIN_BINARIES[@]}"; do
    if [[ -f "$binary" ]]; then
        echo "Checking $(basename "$binary"):"
        
        # Get dependencies
        deps=$(otool -L "$binary" | grep -E "(homebrew|usr/local)" || true)
        if [[ -n "$deps" ]]; then
            echo "  ⚠️  Found external dependencies:"
            echo "$deps" | sed 's/^/    /'
        else
            echo "  ✓ No external dependencies found"
        fi
        echo ""
    else
        echo "✗ $(basename "$binary") - NOT FOUND"
    fi
done

# Check main libraries and their dependencies
echo "=== Checking main libraries ==="
MAIN_LIBS=(
    "$FRAMEWORKS_DIR/libkolosal_server.dylib"
    "$FRAMEWORKS_DIR/libllama-metal.dylib"
)

for lib in "${MAIN_LIBS[@]}"; do
    if [[ -f "$lib" ]]; then
        echo "Checking $(basename "$lib"):"
        
        # Get dependencies
        deps=$(otool -L "$lib" | grep -E "(homebrew|usr/local)" || true)
        if [[ -n "$deps" ]]; then
            echo "  ⚠️  Found external dependencies:"
            echo "$deps" | sed 's/^/    /'
        else
            echo "  ✓ No external dependencies found"
        fi
        echo ""
    else
        echo "✗ $(basename "$lib") - NOT FOUND"
    fi
done

# Check bundled external libraries and their dependencies
echo "=== Checking bundled external libraries ==="
for lib in "${present_libs[@]}"; do
    lib_path="$FRAMEWORKS_DIR/$lib"
    echo "Checking $lib:"
    
    # Check the install name
    install_name=$(otool -D "$lib_path" | tail -n 1)
    if [[ "$install_name" == "@rpath/$lib" ]]; then
        echo "  ✓ Install name correctly set to @rpath/$lib"
    elif [[ "$install_name" == *"homebrew"* ]] || [[ "$install_name" == *"usr/local"* ]]; then
        echo "  ⚠️  Install name still points to external path: $install_name"
    else
        echo "  ℹ️  Install name: $install_name"
    fi
    
    # Get dependencies
    deps=$(otool -L "$lib_path" | grep -E "(homebrew|usr/local)" || true)
    if [[ -n "$deps" ]]; then
        echo "  ⚠️  Found external dependencies:"
        echo "$deps" | sed 's/^/    /'
    else
        echo "  ✓ No external dependencies found"
    fi
    echo ""
done

# Summary
echo "=== SUMMARY ==="
echo "Present libraries: ${#present_libs[@]}/${#EXTERNAL_LIBS[@]}"
echo "Missing libraries: ${#missing_libs[@]}/${#EXTERNAL_LIBS[@]}"

if [[ ${#missing_libs[@]} -gt 0 ]]; then
    echo ""
    echo "Missing libraries:"
    for lib in "${missing_libs[@]}"; do
        echo "  - $lib"
    done
    echo ""
    echo "To install missing libraries, make sure you have them installed via Homebrew:"
    echo "  brew install openssl@3 fontconfig freetype libpng libtiff jpeg-turbo zstd xz"
fi

echo ""
if [[ ${#missing_libs[@]} -eq 0 ]]; then
    echo "🎉 All required external libraries are present!"
else
    echo "⚠️  Some external libraries are missing from the bundle."
fi

echo ""
echo "Note: This script checks for the presence of external libraries and their"
echo "dependencies. For a fully self-contained app bundle, all external"
echo "dependencies should be eliminated or bundled."
