#!/usr/bin/env bash

##
# KolosalCode Termux Installer (Build from Source)
# Installs KolosalCode on Android via Termux without root
# Builds from the local repository source
#
# Usage:
#   cd kolosal-cli
#   bash install-termux.sh
#
# Requirements:
#   - Termux with pkg
#   - Internet connection (for npm dependencies)
##

# Exit on error
set -e

# Version
VERSION="${VERSION:-0.1.3}"

# GitHub repository (for reference)
GITHUB_REPO="KolosalAI/kolosal-cli"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# Print functions using printf for compatibility
print_info() {
    printf "${BLUE}ℹ${NC} %s\n" "$1"
}

print_success() {
    printf "${GREEN}✓${NC} %s\n" "$1"
}

print_warning() {
    printf "${YELLOW}⚠${NC}  %s\n" "$1"
}

print_error() {
    printf "${RED}✗${NC} %s\n" "$1"
}

print_header() {
    printf "\n${BOLD}%s${NC}\n" "$1"
    printf "================================\n"
}

# Get the directory where this script is located (the repository root)
get_repo_dir() {
    # Use $0 for script path, works in most shells
    local script_path="$0"
    
    # Handle relative paths
    if [ -z "${script_path%%/*}" ]; then
        # Absolute path
        script_path="$script_path"
    else
        # Relative path
        script_path="$(pwd)/$script_path"
    fi
    
    # Get directory and resolve to absolute path
    cd "$(dirname "$script_path")" && pwd
}

REPO_DIR="$(get_repo_dir)"

# Check if running in Termux
check_termux() {
    if [ -z "$PREFIX" ]; then
        print_error "This script is designed for Termux on Android."
        print_info "The PREFIX environment variable is not set."
        print_info "Please run this inside Termux."
        exit 1
    fi
    
    if [ ! -d "$PREFIX" ]; then
        print_error "Termux PREFIX directory not found: $PREFIX"
        exit 1
    fi
}

# Detect Architecture
detect_arch() {
    local arch
    arch=$(uname -m)
    case "$arch" in
        aarch64|arm64)
            printf "arm64"
            ;;
        armv7l|armv8l)
            printf "arm"
            ;;
        x86_64)
            printf "x64"
            ;;
        i686|i386)
            printf "x86"
            ;;
        *)
            print_error "Unsupported architecture: $arch"
            print_info "KolosalCode on Termux supports: arm64, arm, x64"
            exit 1
            ;;
    esac
}

# Check if required files exist in the repository
check_repo_files() {
    print_info "Checking repository files..."
    print_info "Repository: $REPO_DIR"
    
    if [ ! -f "$REPO_DIR/package.json" ]; then
        print_error "Required file not found: package.json"
        print_info "Please run this script from the kolosal-cli repository root."
        exit 1
    fi
    
    if [ ! -f "$REPO_DIR/esbuild.config.js" ]; then
        print_error "Required file not found: esbuild.config.js"
        print_info "Please run this script from the kolosal-cli repository root."
        exit 1
    fi
    
    print_success "Repository files verified"
}

# Install required dependencies via pkg
install_dependencies() {
    print_header "Installing Dependencies"
    
    print_info "Updating package lists..."
    pkg update -y 2>/dev/null || true
    
    print_info "Upgrading existing packages (this may take a while)..."
    if pkg upgrade -y 2>&1; then
        print_success "Packages upgraded"
    else
        print_warning "Some packages failed to upgrade, continuing..."
    fi
    
    print_info "Installing required packages..."
    
    # Install packages one by one for better error handling
    for package in nodejs-lts git python make clang; do
        if pkg list-installed 2>/dev/null | grep -q "^$package/"; then
            print_success "$package is already installed"
        else
            print_info "Installing $package..."
            if pkg install -y "$package" 2>&1; then
                print_success "$package installed"
            else
                print_warning "Failed to install $package, continuing..."
            fi
        fi
    done
    
    # Verify Node.js works
    if command -v node >/dev/null 2>&1; then
        if node --version >/dev/null 2>&1; then
            print_success "Node.js is working: $(node --version)"
        else
            print_error "Node.js is installed but not working!"
            print_info "Try running: pkg upgrade"
            print_info "Then run this installer again."
            exit 1
        fi
    else
        print_error "Node.js is not installed!"
        print_info "Try running manually: pkg install nodejs-lts"
        exit 1
    fi
    
    print_success "Dependencies installed"
}

# Build the project from local source
build_project() {
    print_header "Building KolosalCode"
    
    cd "$REPO_DIR"
    
    # Check Node.js version
    if ! command -v node >/dev/null 2>&1; then
        print_error "Node.js not found. Please install with: pkg install nodejs-lts"
        exit 1
    fi
    
    local node_version
    node_version=$(node --version)
    print_info "Node.js version: $node_version"
    
    local npm_version
    npm_version=$(npm --version)
    print_info "npm version: $npm_version"
    
    # Clean previous build artifacts if they exist
    if [ -d "$REPO_DIR/bundle" ]; then
        print_info "Cleaning previous build..."
        rm -rf "$REPO_DIR/bundle"
    fi
    
    # Install npm dependencies
    print_info "Installing npm dependencies..."
    print_info "This may take a while..."
    
    # Use npm ci for clean install, or npm install as fallback
    if [ -f "package-lock.json" ]; then
        if npm ci 2>&1; then
            print_success "Dependencies installed with npm ci"
        else
            print_warning "npm ci failed, trying npm install..."
            if npm install 2>&1; then
                print_success "Dependencies installed with npm install"
            else
                print_error "Failed to install dependencies"
                exit 1
            fi
        fi
    else
        if npm install 2>&1; then
            print_success "Dependencies installed"
        else
            print_error "Failed to install dependencies"
            exit 1
        fi
    fi
    
    # Generate git commit info (if the script exists)
    if [ -f "scripts/generate-git-commit-info.js" ]; then
        print_info "Generating git commit info..."
        node scripts/generate-git-commit-info.js 2>/dev/null || true
    fi
    
    # Bundle the application using esbuild
    print_info "Bundling application with esbuild..."
    
    if [ -f "esbuild.config.js" ]; then
        if node esbuild.config.js; then
            print_success "Bundle created"
        else
            print_error "Failed to bundle application"
            exit 1
        fi
    else
        print_error "esbuild.config.js not found"
        exit 1
    fi
    
    # Copy bundle assets
    if [ -f "scripts/copy_bundle_assets.js" ]; then
        print_info "Copying bundle assets..."
        node scripts/copy_bundle_assets.js 2>/dev/null || true
    fi
    
    # Verify bundle was created
    if [ ! -d "$REPO_DIR/bundle" ]; then
        print_error "Bundle directory was not created!"
        exit 1
    fi
    
    if [ ! -f "$REPO_DIR/bundle/gemini.js" ]; then
        print_error "Main bundle file (gemini.js) not found!"
        exit 1
    fi
    
    print_success "Build complete"
}

# Install the built application
install_app() {
    print_header "Installing KolosalCode"
    
    local install_dir="$PREFIX/opt/kolosal-code"
    local bin_dir="$PREFIX/bin"
    
    print_info "Installing to $install_dir..."
    
    # Remove existing installation if present
    if [ -d "$install_dir" ]; then
        print_info "Removing previous installation..."
        rm -rf "$install_dir"
    fi
    
    # Create installation directory
    mkdir -p "$install_dir"
    mkdir -p "$install_dir/lib"
    mkdir -p "$install_dir/bin"
    mkdir -p "$bin_dir"
    
    # Copy bundle
    print_info "Copying bundle..."
    cp -R "$REPO_DIR/bundle" "$install_dir/lib/"
    
    # Copy required node_modules (external dependencies that aren't bundled)
    if [ -d "$REPO_DIR/node_modules" ]; then
        print_info "Copying required node_modules..."
        mkdir -p "$install_dir/lib/node_modules"
        
        # Copy only necessary external modules that can't be bundled
        for dep in "@lydell/node-pty" "node-pty"; do
            if [ -d "$REPO_DIR/node_modules/$dep" ]; then
                cp -R "$REPO_DIR/node_modules/$dep" "$install_dir/lib/node_modules/" 2>/dev/null || true
            fi
        done
    fi
    
    # Create launcher script that uses Termux's Node.js
    print_info "Creating launcher script..."
    
    cat > "$install_dir/bin/kolosal" << 'LAUNCHER_EOF'
#!/usr/bin/env bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"

# Use Termux's system Node.js (avoids glibc vs bionic issues)
NODE_BINARY="$(command -v node)"

if [ -z "$NODE_BINARY" ] || [ ! -x "$NODE_BINARY" ]; then
    echo "Error: Node.js not found. Please install with: pkg install nodejs-lts"
    exit 1
fi

# Set NODE_PATH to include our bundled node_modules
export NODE_PATH="$APP_DIR/lib/node_modules:$NODE_PATH"

# Suppress deprecation warnings
export NODE_OPTIONS="--no-deprecation"

# Execute the bundle with Termux's Node.js
exec "$NODE_BINARY" "$APP_DIR/lib/bundle/gemini.js" "$@"
LAUNCHER_EOF
    
    chmod +x "$install_dir/bin/kolosal"
    
    # Create symlink in Termux bin directory
    print_info "Creating symlink in $bin_dir..."
    rm -f "$bin_dir/kolosal"
    ln -sf "$install_dir/bin/kolosal" "$bin_dir/kolosal"
    
    print_success "Installation complete"
}

# Verify installation
verify_installation() {
    print_header "Verifying Installation"
    
    # Refresh command hash
    hash -r 2>/dev/null || true
    
    local kolosal_path="$PREFIX/bin/kolosal"
    
    if [ -x "$kolosal_path" ]; then
        print_success "KolosalCode installed successfully!"
        printf "\n"
        print_info "Location: $kolosal_path"
        
        # Try to get version
        print_info "Testing kolosal..."
        if "$kolosal_path" --version 2>/dev/null; then
            print_success "Version check passed"
        else
            print_warning "Version check returned non-zero, but command exists"
        fi
    else
        print_error "Installation finished but kolosal not found at $kolosal_path"
        return 1
    fi
}

# Show usage instructions
show_usage() {
    printf "\n"
    print_header "Quick Start"
    printf "\n"
    printf "Run KolosalCode:\n"
    printf "  ${BOLD}kolosal${NC}\n"
    printf "\n"
    printf "Check version:\n"
    printf "  ${BOLD}kolosal --version${NC}\n"
    printf "\n"
    printf "Get help:\n"
    printf "  ${BOLD}kolosal --help${NC}\n"
    printf "\n"
    print_info "For more information, visit:"
    print_info "  https://github.com/${GITHUB_REPO}"
    printf "\n"
}

# Show uninstall instructions
show_uninstall() {
    printf "\n"
    print_header "Uninstallation"
    printf "\n"
    printf "To uninstall KolosalCode from Termux:\n"
    printf "  rm -rf \$PREFIX/opt/kolosal-code\n"
    printf "  rm -f \$PREFIX/bin/kolosal\n"
    printf "\n"
}

# Ask yes/no question (compatible with POSIX sh)
ask_yes_no() {
    local prompt="$1"
    local response
    
    printf "%s (y/n) " "$prompt"
    read response
    
    case "$response" in
        [Yy]|[Yy][Ee][Ss])
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Main installation flow
main() {
    print_header "KolosalCode Termux Installer v${VERSION}"
    printf "\n"
    print_info "Building from local source (no root required)"
    
    # Check if running in Termux
    check_termux
    print_success "Running in Termux environment"
    print_info "PREFIX: $PREFIX"
    
    # Detect Architecture
    local arch
    arch=$(detect_arch)
    print_success "Detected architecture: $arch"
    
    # Check repository files
    check_repo_files
    
    # Check if already installed
    if [ -x "$PREFIX/bin/kolosal" ]; then
        print_warning "KolosalCode is already installed"
        print_info "Current location: $PREFIX/bin/kolosal"
        printf "\n"
        if ! ask_yes_no "Do you want to reinstall?"; then
            print_info "Installation cancelled"
            exit 0
        fi
    fi
    
    # Install dependencies
    install_dependencies
    
    # Build the project from local source
    build_project
    
    # Install the application
    install_app
    
    # Verify installation
    verify_installation
    
    # Show usage
    show_usage
    
    print_success "Installation completed successfully!"
}

# Handle arguments
case "${1:-}" in
    --uninstall)
        show_uninstall
        ;;
    --help|-h)
        printf "KolosalCode Termux Installer (Build from Source)\n"
        printf "\n"
        printf "Usage:\n"
        printf "  cd kolosal-cli\n"
        printf "  bash install-termux.sh [OPTIONS]\n"
        printf "\n"
        printf "Options:\n"
        printf "  --help, -h     Show this help message\n"
        printf "  --uninstall    Show uninstall instructions\n"
        printf "\n"
        printf "This script builds KolosalCode from the local repository\n"
        printf "and installs it to \$PREFIX/opt/kolosal-code\n"
        printf "\n"
        printf "Prerequisites:\n"
        printf "  1. Clone the repository: git clone https://github.com/${GITHUB_REPO}.git\n"
        printf "  2. Enter the directory: cd kolosal-cli\n"
        printf "  3. Run installer: bash install-termux.sh\n"
        ;;
    *)
        main "$@"
        ;;
esac
