#!/data/data/com.termux/files/usr/bin/bash

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

# Get the directory where this script is located (the repository root)
get_repo_dir() {
    local script_path="${BASH_SOURCE[0]}"
    # Resolve symlinks
    while [ -L "$script_path" ]; do
        local script_dir="$(cd "$(dirname "$script_path")" && pwd)"
        script_path="$(readlink "$script_path")"
        [[ $script_path != /* ]] && script_path="$script_dir/$script_path"
    done
    cd "$(dirname "$script_path")" && pwd
}

REPO_DIR="$(get_repo_dir)"

# Check if running in Termux
check_termux() {
    if [[ "$PREFIX" != *"/com.termux/"* ]] && [[ ! -d "/data/data/com.termux" ]]; then
        print_error "This script is designed for Termux on Android."
        print_info "Please run this inside Termux."
        exit 1
    fi
}

# Detect Architecture
detect_arch() {
    local arch=$(uname -m)
    case "$arch" in
        aarch64|arm64)
            echo "arm64"
            ;;
        armv7l|armv8l)
            echo "arm"
            ;;
        x86_64)
            echo "x64"
            ;;
        i686|i386)
            echo "x86"
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
    
    local required_files=(
        "package.json"
        "esbuild.config.js"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$REPO_DIR/$file" ]; then
            print_error "Required file not found: $file"
            print_info "Please run this script from the kolosal-cli repository root."
            exit 1
        fi
    done
    
    print_success "Repository files verified"
}

# Install required dependencies via pkg
install_dependencies() {
    print_header "Installing Dependencies"
    
    print_info "Updating package lists..."
    pkg update -y || true
    
    print_info "Installing required packages..."
    
    # Core build tools and runtime
    local packages=(
        "nodejs-lts"    # Node.js LTS (includes npm)
        "git"           # For git operations
        "python"        # Required for node-gyp (native module builds)
        "make"          # Build tools
        "clang"         # C/C++ compiler for native modules
    )
    
    for package in "${packages[@]}"; do
        if pkg list-installed 2>/dev/null | grep -q "^$package/"; then
            print_success "$package is already installed"
        else
            print_info "Installing $package..."
            pkg install -y "$package" || {
                print_warning "Failed to install $package, continuing..."
            }
        fi
    done
    
    print_success "Dependencies installed"
}

# Build the project from local source
build_project() {
    print_header "Building KolosalCode"
    
    cd "$REPO_DIR"
    
    # Check Node.js version
    local node_version=$(node --version)
    print_info "Node.js version: $node_version"
    
    local npm_version=$(npm --version)
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
        npm ci || {
            print_warning "npm ci failed, trying npm install..."
            npm install || {
                print_error "Failed to install dependencies"
                exit 1
            }
        }
    else
        npm install || {
            print_error "Failed to install dependencies"
            exit 1
        }
    fi
    
    print_success "Dependencies installed"
    
    # Generate git commit info (if the script exists)
    if [ -f "scripts/generate-git-commit-info.js" ]; then
        print_info "Generating git commit info..."
        node scripts/generate-git-commit-info.js || true
    fi
    
    # Bundle the application using esbuild
    print_info "Bundling application with esbuild..."
    
    if [ -f "esbuild.config.js" ]; then
        node esbuild.config.js || {
            print_error "Failed to bundle application"
            exit 1
        }
    else
        print_error "esbuild.config.js not found"
        exit 1
    fi
    
    # Copy bundle assets
    if [ -f "scripts/copy_bundle_assets.js" ]; then
        print_info "Copying bundle assets..."
        node scripts/copy_bundle_assets.js || true
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
        local externals=(
            "@lydell/node-pty"
            "node-pty"
        )
        
        for dep in "${externals[@]}"; do
            if [ -d "$REPO_DIR/node_modules/$dep" ]; then
                cp -R "$REPO_DIR/node_modules/$dep" "$install_dir/lib/node_modules/" 2>/dev/null || true
            fi
        done
    fi
    
    # Create launcher script that uses Termux's Node.js
    print_info "Creating launcher script..."
    
    cat > "$install_dir/bin/kolosal" << 'LAUNCHER_EOF'
#!/data/data/com.termux/files/usr/bin/bash

# Get the directory where this script is located (resolve symlinks)
SCRIPT_PATH="${BASH_SOURCE[0]}"
while [ -L "$SCRIPT_PATH" ]; do
    SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
    SCRIPT_PATH="$(readlink "$SCRIPT_PATH")"
    [[ $SCRIPT_PATH != /* ]] && SCRIPT_PATH="$SCRIPT_DIR/$SCRIPT_PATH"
done
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"

# Use Termux's system Node.js (avoids glibc vs bionic issues)
NODE_BINARY="$(which node)"

if [ ! -x "$NODE_BINARY" ]; then
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
    ln -sf "$install_dir/bin/kolosal" "$bin_dir/kolosal"
    
    print_success "Installation complete"
}

# Verify installation
verify_installation() {
    print_header "Verifying Installation"
    
    if command -v kolosal &> /dev/null; then
        print_success "KolosalCode installed successfully!"
        echo ""
        print_info "Location: $(which kolosal)"
        
        # Try to get version
        print_info "Testing kolosal..."
        if kolosal --version 2>/dev/null; then
            print_success "Version check passed"
        else
            print_warning "Version check returned non-zero, but command exists"
        fi
    else
        print_error "Installation finished but 'kolosal' not found in PATH."
        print_info "Ensure $PREFIX/bin is in your PATH."
        print_info "Try running: hash -r"
        return 1
    fi
}

# Show usage instructions
show_usage() {
    echo ""
    print_header "Quick Start"
    echo ""
    echo "Run KolosalCode:"
    echo -e "  ${BOLD}kolosal${NC}"
    echo ""
    echo "Check version:"
    echo -e "  ${BOLD}kolosal --version${NC}"
    echo ""
    echo "Get help:"
    echo -e "  ${BOLD}kolosal --help${NC}"
    echo ""
    print_info "For more information, visit:"
    print_info "  https://github.com/${GITHUB_REPO}"
    echo ""
}

# Show uninstall instructions
show_uninstall() {
    echo ""
    print_header "Uninstallation"
    echo ""
    echo "To uninstall KolosalCode from Termux:"
    echo "  rm -rf \$PREFIX/opt/kolosal-code"
    echo "  rm -f \$PREFIX/bin/kolosal"
    echo ""
}

# Main installation flow
main() {
    print_header "KolosalCode Termux Installer v${VERSION}"
    echo ""
    print_info "Building from local source (no root required)"
    print_info "Repository: $REPO_DIR"
    echo ""
    
    # Check if running in Termux
    check_termux
    print_success "Running in Termux environment"
    
    # Detect Architecture
    local arch=$(detect_arch)
    print_success "Detected architecture: $arch"
    
    # Check repository files
    check_repo_files
    
    # Check if already installed
    if command -v kolosal &> /dev/null; then
        print_warning "KolosalCode is already installed"
        print_info "Current location: $(which kolosal)"
        echo ""
        read -p "Do you want to reinstall? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
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
        echo "KolosalCode Termux Installer (Build from Source)"
        echo ""
        echo "Usage:"
        echo "  cd kolosal-cli"
        echo "  bash install-termux.sh [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --uninstall    Show uninstall instructions"
        echo ""
        echo "This script builds KolosalCode from the local repository"
        echo "and installs it to \$PREFIX/opt/kolosal-code"
        echo ""
        echo "Prerequisites:"
        echo "  1. Clone the repository: git clone https://github.com/${GITHUB_REPO}.git"
        echo "  2. Enter the directory: cd kolosal-cli"
        echo "  3. Run installer: bash install-termux.sh"
        ;;
    *)
        main "$@"
        ;;
esac
