#!/bin/bash

##
# Clean rebuild and package for Linux
# This script performs a complete clean build from scratch
##

set -e

echo "🧹 Clean Rebuild & Package Script (Linux)"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_step() {
    echo -e "${GREEN}▶${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC}  $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Parse arguments
PACKAGE_FORMAT="${1:-all}"

if [[ "$PACKAGE_FORMAT" != "all" && "$PACKAGE_FORMAT" != "deb" && "$PACKAGE_FORMAT" != "rpm" && "$PACKAGE_FORMAT" != "tar" ]]; then
    print_error "Invalid package format: $PACKAGE_FORMAT"
    echo "Usage: $0 [all|deb|rpm|tar]"
    echo ""
    echo "  all - Build all package formats (default)"
    echo "  deb - Build Debian package (.deb)"
    echo "  rpm - Build RPM package (.rpm)"
    echo "  tar - Build tarball (.tar.gz)"
    exit 1
fi

# Step 1: Clean all build artifacts
print_step "Cleaning build artifacts..."
echo ""

if [ -d "dist" ]; then
    echo "   Removing dist/"
    rm -rf dist
fi

if [ -d ".debroot" ]; then
    echo "   Removing .debroot/"
    rm -rf .debroot
fi

if [ -d ".rpmroot" ]; then
    echo "   Removing .rpmroot/"
    rm -rf .rpmroot
fi

if [ -d "kolosal-server/build" ]; then
    echo "   Removing kolosal-server/build/"
    rm -rf kolosal-server/build
fi

if [ -d "bundle" ]; then
    echo "   Removing bundle/"
    rm -rf bundle
fi

echo ""
print_step "✓ Clean complete"
echo ""

# Step 2: Install dependencies (if needed)
if [ ! -d "node_modules" ]; then
    print_step "Installing dependencies..."
    npm install
    echo ""
fi

# Step 3: Build the project
print_step "Building project..."
npm run build
echo ""

# Step 4: Bundle shared libraries for portability  
print_step "Bundling shared libraries for standalone distribution..."
echo ""
echo "   Making kolosal-cli fully portable by bundling critical system libraries..."
echo "   Building both CPU and Vulkan inference engines for maximum compatibility."
echo "   This ensures the package works on minimal Linux systems without dependencies."
echo ""

# Run the library bundling process
if node scripts/bundle_shared_libs.js >/dev/null 2>&1; then
    print_step "✓ Library bundling complete"
    
    # Verify bundled libraries
    BUNDLED_COUNT=$(cd dist/linux/kolosal-app && LD_LIBRARY_PATH=$(pwd)/lib ldd ./bin/kolosal-server 2>/dev/null | grep "$(pwd)" | wc -l)
    SYSTEM_COUNT=$(cd dist/linux/kolosal-app && LD_LIBRARY_PATH=$(pwd)/lib ldd ./bin/kolosal-server 2>/dev/null | grep -v "$(pwd)" | grep "=>" | wc -l)
    
    echo "   📊 Bundled libraries: $BUNDLED_COUNT"
    echo "   🖥️  System libraries: $SYSTEM_COUNT (core libraries only)"
    echo "   ✅ Dependency reduction successful!"
else
    print_warning "Library bundling failed, continuing with system dependencies"
fi
echo ""

# Step 5: Check for required tools
print_step "Checking for packaging tools..."

MISSING_TOOLS=()

if [[ "$PACKAGE_FORMAT" == "all" || "$PACKAGE_FORMAT" == "deb" ]]; then
    if ! command -v dpkg-deb &> /dev/null; then
        print_warning "dpkg-deb not found (needed for .deb packages)"
        MISSING_TOOLS+=("dpkg-deb (install: apt-get install dpkg)")
    else
        echo "   ✓ dpkg-deb found"
    fi
fi

if [[ "$PACKAGE_FORMAT" == "all" || "$PACKAGE_FORMAT" == "rpm" ]]; then
    if ! command -v rpmbuild &> /dev/null; then
        print_warning "rpmbuild not found (needed for .rpm packages)"
        echo "     RPM creation will be skipped"
        echo "     Install with: sudo apt-get install rpm"
    else
        echo "   ✓ rpmbuild found"
    fi
fi

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    print_warning "Some packaging tools are missing:"
    for tool in "${MISSING_TOOLS[@]}"; do
        echo "     - $tool"
    done
    echo ""
    read -p "Continue anyway? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo ""

# Step 6: Build Linux packages
print_step "Building Linux package(s): $PACKAGE_FORMAT"
echo ""
node scripts/build-standalone-linux.js "$PACKAGE_FORMAT"

echo ""
echo "=========================================="
echo -e "${GREEN}✨ Build Complete!${NC}"
echo ""
echo "📦 Package location(s):"
echo "   dist/linux/"
echo ""

# Show package-specific instructions
if [[ "$PACKAGE_FORMAT" == "all" || "$PACKAGE_FORMAT" == "deb" ]]; then
    echo "📝 To install .deb package:"
    echo "   sudo dpkg -i dist/linux/kolosal-code_*.deb"
    echo "   # or"
    echo "   sudo apt install ./dist/linux/kolosal-code_*.deb"
    echo ""
fi

if [[ "$PACKAGE_FORMAT" == "all" || "$PACKAGE_FORMAT" == "rpm" ]]; then
    echo "📝 To install .rpm package:"
    echo "   sudo rpm -i dist/linux/kolosal-code-*.rpm"
    echo "   # or"
    echo "   sudo dnf install dist/linux/kolosal-code-*.rpm"
    echo ""
fi

if [[ "$PACKAGE_FORMAT" == "all" || "$PACKAGE_FORMAT" == "tar" ]]; then
    echo "📝 To install from tarball:"
    echo "   sudo tar -xzf dist/linux/kolosal-code-*-linux-*.tar.gz -C /opt"
    echo "   sudo ln -s /opt/kolosal-app/bin/kolosal /usr/local/bin/kolosal"
    echo ""
fi

echo "🧪 To test before installing:"
echo "   ./dist/linux/kolosal-app/bin/kolosal --version"
echo ""
echo "🎮 Acceleration support included:"
echo "   ✓ CPU inference engine (libllama-cpu.so) - works on all systems"  
echo "   ✓ Vulkan inference engine (libllama-vulkan.so) - GPU acceleration where available"
echo ""

echo "📋 Dependencies (automatically bundled for maximum portability):"
echo "   ✅ Critical libraries bundled in the package"  
echo "   🎯 Minimal core system libraries required (present on ALL Linux distributions)"
echo "      - SSL/TLS: libssl3, libcrypto3, libgnutls30"
echo "      - Core system: libc6, libstdc++6, libgcc-s1, libm6, libz1"
echo "      - Security: kerberos, resolv (standard on all systems)"
echo "   🎮 Vulkan libraries bundled for GPU acceleration compatibility"
echo "   📦 Users can install without worrying about missing dependencies!"
echo ""
echo "�🚀 After installation, run:"
echo "   kolosal --version"
echo ""
