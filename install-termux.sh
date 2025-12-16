#!/bin/bash

##
# KolosalCode Termux Installer
# Installs KolosalCode on Android via Termux without root
#
# Usage:
#   bash install-termux.sh
##

set -e

# Version to install
VERSION="${VERSION:-0.1.3}"
PACKAGE_VERSION="0.1.3"

# GitHub release URLs
GITHUB_REPO="KolosalAI/kolosal-cli"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Print functions
print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC}  $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_header() {
    echo ""
    echo -e "${BOLD}$1${NC}"
    echo "================================"
}

# Detect Architecture
detect_arch() {
    local arch=$(uname -m)
    case "$arch" in
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            print_error "Unsupported architecture for Termux: $arch"
            print_info "KolosalCode on Termux currently supports arm64 only."
            exit 1
            ;;
    esac
}

# Download file
download_file() {
    local url="$1"
    local output="$2"
    
    print_info "Downloading from: $url"
    
    if command -v curl &> /dev/null; then
        curl -fsSL -o "$output" "$url" || {
            print_error "Download failed"
            return 1
        }
    elif command -v wget &> /dev/null; then
        wget -q -O "$output" "$url" || {
            print_error "Download failed"
            return 1
        }
    else
        print_error "Neither curl nor wget is available"
        print_info "Please install curl or wget: pkg install curl"
        return 1
    fi
    
    print_success "Download complete"
}

# Main installation flow
main() {
    print_header "KolosalCode Termux Installer v${VERSION}"
    echo ""
    
    # Check if running in Termux
    if [[ "$PREFIX" != *"/com.termux/"* ]]; then
        print_warning "It seems you are not running inside Termux standard environment."
        print_info "This script is optimized for Termux on Android."
        print_info "Proceeding anyway..."
    fi

    # Detect Architecture
    local arch=$(detect_arch)
    print_success "Detected architecture: $arch"
    
    # Define paths
    # Use $PREFIX if available, otherwise fallback to $HOME/kolosal-termux
    local install_base="${PREFIX:-$HOME}"
    local install_dir="$install_base/opt/kolosal-code"
    local bin_dir="$install_base/bin"
    
    # URL for tarball
    # Assuming the release includes a tarball following the build script naming convention
    local tarball_url="https://github.com/${GITHUB_REPO}/releases/download/v${VERSION}/kolosal-code-${PACKAGE_VERSION}-linux-${arch}.tar.gz"
    
    local tmp_dir=$(mktemp -d)
    local tarball_file="${tmp_dir}/kolosal.tar.gz"
    
    # Download
    if ! download_file "$tarball_url" "$tarball_file"; then
        print_error "Failed to download package."
        print_info "Make sure a release tarball exists for version v${VERSION} on linux-${arch}."
        rm -rf "$tmp_dir"
        exit 1
    fi
    
    # Install
    print_info "Installing to $install_dir..."
    
    # Create directories
    mkdir -p "$install_dir"
    mkdir -p "$bin_dir"
    
    # Extract
    if tar -xzf "$tarball_file" -C "$install_dir" --strip-components=1; then
        print_success "Extracted files"
    else
        # Try without stripping if directory structure differs
        print_warning "Extraction with strip-components failed, trying strict extract..."
        rm -rf "$install_dir"/*
        tar -xzf "$tarball_file" -C "$install_dir"
    fi
    
    # Handling the case where tarball contains a root folder name
    # The build script packs: tar -czf ... -C parent_dir app_dir_name
    # So it usually contains a top level folder 'kolosal-app'.
    # If we extracted to install_dir, we might have install_dir/kolosal-app.
    if [ -d "$install_dir/kolosal-app" ]; then
        mv "$install_dir/kolosal-app"/* "$install_dir/"
        rmdir "$install_dir/kolosal-app"
    fi
    
    # Create Symlink
    print_info "Creating symlink..."
    ln -sf "$install_dir/bin/kolosal" "$bin_dir/kolosal"
    
    # Fix permissions (just in case)
    chmod +x "$install_dir/bin/kolosal"
    chmod +x "$bin_dir/kolosal"

    # NOTE: The bundled node might not work on Termux (glibc vs bionic).
    # We warn the user about this potential issue.
    print_warning "Note: This installation uses the bundled Node.js binary."
    print_warning "If you encounter 'No such file or directory' or 'Exec format error', you may need to install 'gcompat' or use the system node."
    print_info "To use system node, edit $install_dir/bin/kolosal and change NODE_BINARY location."
    print_info "Install gcompat: pkg install gcompat"

    # Clean up
    rm -rf "$tmp_dir"
    
    # Verify
    if command -v kolosal &> /dev/null; then
        print_success "KolosalCode installed successfully!"
        echo ""
        print_info "Location: $(which kolosal)"
        print_info "Run 'kolosal --version' to enable the CLI."
    else
        print_error "Installation finished but 'kolosal' not found in PATH."
        print_info "Ensure $bin_dir is in your PATH."
    fi
}

main "$@"
