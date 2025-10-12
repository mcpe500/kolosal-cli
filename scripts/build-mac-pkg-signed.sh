#!/bin/bash

##
# Build and notarize macOS package with code signing
# This script sets up the required environment variables for code signing and notarization
#
# Usage:
#   ./scripts/build-mac-pkg-signed.sh          # Normal build
#   ./scripts/build-mac-pkg-signed.sh --clean  # Clean build
##

set -e

# Check for clean flag
CLEAN_BUILD=0
if [ "$1" = "--clean" ] || [ "$1" = "-c" ]; then
    CLEAN_BUILD=1
    echo "🧹 Clean build mode enabled"
    echo ""
fi

# Clean if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo "🗑️  Cleaning build artifacts..."
    rm -rf dist .pkgroot kolosal-server/build bundle
    echo "   ✓ Cleaned"
    echo ""
    
    # Flush DNS cache
    echo "🔄 Flushing DNS cache..."
    sudo dscacheutil -flushcache 2>/dev/null || true
    sudo killall -HUP mDNSResponder 2>/dev/null || true
    echo "   ✓ DNS cache flushed"
    echo ""
fi

echo "🔐 Setting up code signing identities..."

# Set code signing identities
export CODESIGN_IDENTITY_APP="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
export CODESIGN_IDENTITY="Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)"

# Enable notarization (set to 0 to skip)
export NOTARIZE="${NOTARIZE:-1}"

echo "   Application cert: $CODESIGN_IDENTITY_APP"
echo "   Installer cert: $CODESIGN_IDENTITY"
echo "   Notarization: $([ "$NOTARIZE" = "1" ] && echo "enabled" || echo "disabled")"

# Build the package
echo ""
echo "🚀 Building macOS package..."
node scripts/build-standalone-pkg.js

echo ""
echo "✅ Done!"
