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
    # libcurl and yaml-cpp are needed for kolosal-server
    # esbuild is needed for bundling (system version avoids postinstall script issues)
    # cmake is needed for building kolosal-server
    # libopenblas and blas-openblas are needed for LAPACK support in faiss (kolosal-server)
    for package in nodejs-lts git python make clang cmake libcurl yaml-cpp esbuild libopenblas blas-openblas pkg-config; do
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
    
    # Clean node_modules to ensure fresh install without cached problematic scripts
    if [ -d "$REPO_DIR/node_modules" ]; then
        print_info "Cleaning node_modules for fresh install..."
        rm -rf "$REPO_DIR/node_modules"
    fi
    
    # Install npm dependencies
    # Use --ignore-scripts because some packages have postinstall scripts
    # that don't work on Android (e.g., node-pty needs Android NDK, @lvce-editor/ripgrep
    # doesn't recognize Android platform). We'll use Termux's system ripgrep instead.
    print_info "Installing npm dependencies..."
    print_info "This may take a while..."
    print_info "Skipping native modules that don't support Android..."
    
    # Force npm to ignore scripts via environment variable as well
    export npm_config_ignore_scripts=true
    
    # Use npm install with --ignore-scripts to skip postinstall scripts that fail on Android
    # (e.g., node-pty, @lvce-editor/ripgrep). We do NOT use --omit=optional because
    # esbuild needs its platform-specific binaries (@esbuild/android-arm64) which are optional.
    if [ -f "package-lock.json" ]; then
        if npm ci --ignore-scripts 2>&1; then
            print_success "Dependencies installed with npm ci"
        else
            print_warning "npm ci failed, trying npm install..."
            if npm install --ignore-scripts 2>&1; then
                print_success "Dependencies installed with npm install"
            else
                print_error "Failed to install dependencies"
                exit 1
            fi
        fi
    else
        if npm install --ignore-scripts 2>&1; then
            print_success "Dependencies installed"
        else
            print_error "Failed to install dependencies"
            exit 1
        fi
    fi
    
    # Link system esbuild to node_modules if needed (since we ignored scripts)
    if command -v esbuild >/dev/null 2>&1; then
        print_info "Linking system esbuild..."
        mkdir -p node_modules/.bin
        rm -f node_modules/.bin/esbuild
        ln -sf "$(command -v esbuild)" node_modules/.bin/esbuild
    fi
    
    # Generate git commit info (if the script exists)
    if [ -f "scripts/generate-git-commit-info.js" ]; then
        print_info "Generating git commit info..."
        node scripts/generate-git-commit-info.js 2>/dev/null || true
    fi

    # Patch package.json files to point to source instead of dist
    # This allows esbuild to bundle TS directly, bypassing tsc errors
    print_info "Patching packages for direct source bundling..."
    
    # Function to patch a package.json
    patch_package_json() {
        local pkg_dir="$1"
        local pkg_json="$pkg_dir/package.json"
        
        if [ -f "$pkg_json" ]; then
            print_info "  - Patching $pkg_dir"
            # Backup original
            cp "$pkg_json" "$pkg_json.bak"
            
            # Replace dist/index.js with index.ts (or src/index.ts if index.ts doesn't exist)
            # We assume most packages use index.ts at root or src/index.ts
            if [ -f "$pkg_dir/index.ts" ]; then
                 entry_point="index.ts"
            elif [ -f "$pkg_dir/src/index.ts" ]; then
                 entry_point="src/index.ts"
            else
                 # Fallback, keep as is
                 return 0
            fi
            
            # Use sed to replace main and exports
            # 1. Replace "main": "dist/..." with "main": "$entry_point"
            # 2. Replace "import": "./dist/..." with "import": "./$entry_point"
            # 3. Remove "types" fields to avoid confusion (optional, but safe)
            
            # Note: simplistic sed, assumes standard formatting
            sed -i "s|\"main\": \"dist/.*\"|\"main\": \"$entry_point\"|g" "$pkg_json"
            sed -i "s|\"import\": \"\./dist/.*\"|\"import\": \"\./$entry_point\"|g" "$pkg_json"
            sed -i "s|\"require\": \"\./dist/.*\"|\"require\": \"\./$entry_point\"|g" "$pkg_json"
            
            # Also handle standalone.js for api-server if needed, but bundle uses gemini.js
        fi
    }
    
    # Patch all packages
    for pkg in packages/*; do
        if [ -d "$pkg" ]; then
            patch_package_json "$pkg"
        fi
    done
    
    # Create a Termux-specific esbuild config that forces bundling of workspaces
    print_info "Creating Termux-specific esbuild config..."
    cat > esbuild.config.termux.js << 'EOF'
import esbuild from 'esbuild';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const require = createRequire(import.meta.url);
const pkg = require(path.resolve(__dirname, 'package.json'));

console.log('Building for Termux with workspace bundling...');

esbuild
  .build({
    entryPoints: ['packages/cli/index.ts'],
    bundle: true,
    outfile: 'bundle/gemini.js',
    platform: 'node',
    target: 'node18',
    format: 'esm',
    // Only exclude genuine system/native dependencies
    external: [
      '@lydell/node-pty',
      'node-pty',
      'tiktoken',
      'esbuild',
      '@esbuild/*',
    ],
    alias: {
      'is-in-ci': path.resolve(
        __dirname,
        'packages/cli/src/patches/is-in-ci.ts',
      ),
      // Explicitly alias workspace packages to source to force bundling
      '@kolosal-ai/api-server': path.resolve(__dirname, 'packages/api-server/index.ts'),
      '@kolosal-ai/kolosal-ai-core': path.resolve(__dirname, 'packages/core/index.ts'),
      '@kolosal-ai/kolosal-ai-test-utils': path.resolve(__dirname, 'packages/test-utils/index.ts'),
      // Add other workspaces if needed
    },
    define: {
      'process.env.CLI_VERSION': JSON.stringify(pkg.version),
    },
    banner: {
      js: `import { createRequire as _kolosalCreateRequire } from 'module'; const require = _kolosalCreateRequire(import.meta.url); globalThis.__filename = _kolosalCreateRequire(import.meta.url)('url').fileURLToPath(import.meta.url); globalThis.__dirname = _kolosalCreateRequire(import.meta.url)('path').dirname(globalThis.__filename);`,
    },
    loader: { '.node': 'file', '.wasm': 'copy' },
    logLevel: 'info',
  })
  .catch(() => process.exit(1));
EOF

    # Bundle the application using esbuild with the new config
    print_info "Bundling application with esbuild (Termux config)..."
    
    if [ -f "esbuild.config.termux.js" ]; then
        if node esbuild.config.termux.js; then
            print_success "Bundle created"
        else
            print_error "Failed to bundle application"
            # Try to restore package.json files
            for pkg in packages/*; do
                if [ -f "$pkg/package.json.bak" ]; then
                    mv "$pkg/package.json.bak" "$pkg/package.json"
                fi
            done
            exit 1
        fi
        
        # Restore package.json files
        for pkg in packages/*; do
            if [ -f "$pkg/package.json.bak" ]; then
                mv "$pkg/package.json.bak" "$pkg/package.json"
            fi
        done
        
        # Cleanup temp config
        rm esbuild.config.termux.js
    else
        print_error "esbuild.config.termux.js creation failed"
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

# Prepare git submodules
prepare_submodules() {
    print_info "Checking submodules..."
    if [ -d ".git" ]; then
        if [ -f ".gitmodules" ]; then
            print_info "Initializing git submodules..."
            
            # Check if kolosal-server exists but is not a valid submodule checkout (e.g. only contains build dir from failed build)
            if [ -d "kolosal-server" ] && [ ! -f "kolosal-server/CMakeLists.txt" ]; then
                print_warning "kolosal-server directory exists but appears incomplete. Cleaning..."
                rm -rf kolosal-server
            fi

            # Try to update submodules, but don't fail strictly if network issues or not needed
            git submodule update --init --recursive || print_warning "Failed to update submodules. Kolosal Server might not build."
        fi
    else
        print_warning "Not a git repository. Skipping submodule initialization."
        # Check if kolosal-server is empty
        if [ ! -f "kolosal-server/CMakeLists.txt" ]; then
             print_warning "kolosal-server/CMakeLists.txt not found. Server build will likely fail."
        fi
    fi
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
    
    # Copy Kolosal Server artifacts if they exist
    # Check for linux directory (patched script treats android as linux)
    if [ -d "$REPO_DIR/dist/linux/kolosal-app" ]; then
        print_info "Copying Kolosal Server artifacts..."
        
        # Copy binaries
        if [ -d "$REPO_DIR/dist/linux/kolosal-app/bin" ]; then
            cp -R "$REPO_DIR/dist/linux/kolosal-app/bin/"* "$install_dir/bin/" 2>/dev/null || true
        fi
        
        # Copy libraries
        if [ -d "$REPO_DIR/dist/linux/kolosal-app/lib" ]; then
            mkdir -p "$install_dir/lib"
            cp -R "$REPO_DIR/dist/linux/kolosal-app/lib/"* "$install_dir/lib/" 2>/dev/null || true
        fi
        
        # Copy resources
        if [ -d "$REPO_DIR/dist/linux/kolosal-app/Resources" ]; then
            mkdir -p "$install_dir/Resources"
            cp -R "$REPO_DIR/dist/linux/kolosal-app/Resources/"* "$install_dir/Resources/" 2>/dev/null || true
        fi
    fi

    # Copy required node_modules (external dependencies that aren't bundled)
    if [ -d "$REPO_DIR/node_modules" ]; then
        print_info "Copying required node_modules..."
        mkdir -p "$install_dir/lib/node_modules"
        
        # Copy only necessary external modules that can't be bundled
        for dep in "tiktoken" "node-pty" "@lydell"; do
            if [ -d "$REPO_DIR/node_modules/$dep" ]; then
                dest="$install_dir/lib/node_modules/$dep"
                mkdir -p "$(dirname "$dest")"
                cp -R "$REPO_DIR/node_modules/$dep" "$dest" 2>/dev/null || true
            fi
        done
    fi
    
    # Create launcher script that uses Termux's Node.js
    print_info "Creating launcher script..."
    
    cat > "$install_dir/bin/kolosal" << 'LAUNCHER_EOF'
#!/usr/bin/env bash

# Resolve the actual script location (following symlinks)
# This is needed because the script is invoked via a symlink in $PREFIX/bin
SCRIPT_PATH="$0"

# Follow symlinks to get the real path
if command -v readlink > /dev/null 2>&1; then
    # Try readlink -f first (GNU readlink)
    REAL_PATH="$(readlink -f "$SCRIPT_PATH" 2>/dev/null)"
    if [ -z "$REAL_PATH" ]; then
        # Fallback for BSD readlink (no -f option)
        REAL_PATH="$SCRIPT_PATH"
        while [ -L "$REAL_PATH" ]; do
            LINK_TARGET="$(readlink "$REAL_PATH")"
            case "$LINK_TARGET" in
                /*) REAL_PATH="$LINK_TARGET" ;;
                *) REAL_PATH="$(dirname "$REAL_PATH")/$LINK_TARGET" ;;
            esac
        done
    fi
else
    REAL_PATH="$SCRIPT_PATH"
fi

# Get the directory where the actual script is located
SCRIPT_DIR="$(cd "$(dirname "$REAL_PATH")" && pwd)"
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
    read -r response
    
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
    
    # Prepare submodules
    prepare_submodules
    
    # Build the project from local source
    build_project
    
    # Build Kolosal Server (C++ backend)
    # Check if cmake is installed
    if command -v cmake >/dev/null 2>&1; then
        print_header "Building Kolosal Server"
        print_info "Building C++ inference server..."
        
        # Ensure we are in the repo dir
        cd "$REPO_DIR"
        
        # Patch build script for Android support (treat android as linux) and disable PoDoFo
        if [ -f "scripts/build_kolosal_server.js" ]; then
             print_info "Patching build script for Android support..."
             sed -i "s|const isLinux = process.platform === 'linux';|const isLinux = process.platform === 'linux' \|\| process.platform === 'android';|g" scripts/build_kolosal_server.js
             
             # Disable PoDoFo (PDF support) as it fails to build on Termux due to C++ standard issues
             print_info "Disabling PoDoFo for Termux build..."
             sed -i "s|cmake .. -DCMAKE_BUILD_TYPE=Release|cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PODOFO=OFF -DPODOFO_BUILD_STATIC=OFF|g" scripts/build_kolosal_server.js
        fi
        
        # Run the build script
        if node scripts/build_kolosal_server.js; then
            print_success "Kolosal Server built successfully"
        else
            print_warning "Kolosal Server build failed. CLI will work but local inference might not."
            print_info "You can try building it manually later."
        fi
    else
        print_warning "CMake not found. Skipping Kolosal Server build."
        print_info "Install cmake with: pkg install cmake"
    fi
    
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
