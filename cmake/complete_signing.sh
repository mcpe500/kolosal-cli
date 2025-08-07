#!/bin/bash

# Complete signing workflow for Kolosal macOS distribution
# Usage: ./complete_signing.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
STAGING_DIR="/tmp/kolosal-dmg-staging"
DMG_STAGING="/tmp/kolosal-final-dmg"

# Configuration
CODESIGN_IDENTITY="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
TEAM_ID="SNW8GV8C24"
ENTITLEMENTS="$PROJECT_ROOT/resources/kolosal.entitlements"

echo "🚀 Starting complete signing workflow for Kolosal..."

# Step 1: Build and install the project
echo "📦 Building and installing project..."
cd "$PROJECT_ROOT"
cmake --build build --target install --config Release

# Step 2: Sign all components in staging
echo "✍️  Signing all components..."

# Sign all libraries
find "$STAGING_DIR/Kolosal.app/Contents/Frameworks" -name "*.dylib" -exec codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime {} \;

# Sign executables
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements "$ENTITLEMENTS" "$STAGING_DIR/Kolosal.app/Contents/MacOS/kolosal"
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements "$ENTITLEMENTS" "$STAGING_DIR/Kolosal.app/Contents/MacOS/kolosal-server"

# Sign launcher components
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime "$STAGING_DIR/Kolosal.app/Contents/MacOS/kolosal-launcher.sh"
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements "$ENTITLEMENTS" "$STAGING_DIR/Kolosal.app/Contents/MacOS/kolosal-launcher"

# Sign the app bundle
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements "$ENTITLEMENTS" "$STAGING_DIR/Kolosal.app"

# Step 3: Verify signatures
echo "🔍 Verifying signatures..."
codesign --verify --verbose=2 "$STAGING_DIR/Kolosal.app"
echo "✅ Code signature verified (spctl check skipped - requires notarization)"

# Step 4: Create DMG staging
echo "💿 Creating DMG staging..."
rm -rf "$DMG_STAGING"
mkdir -p "$DMG_STAGING/.background"

# Copy signed app bundle
cp -R "$STAGING_DIR/Kolosal.app" "$DMG_STAGING/"

# Create Applications symlink
ln -sf /Applications "$DMG_STAGING/Applications"

# Copy background image
cp "$PROJECT_ROOT/resources/dmg_background.png" "$DMG_STAGING/.background/"

# Step 5: Create DMG
echo "📀 Creating final DMG..."
DMG_NAME="kolosal-1.0.0-apple-silicon-signed.dmg"
DMG_PATH="$BUILD_DIR/$DMG_NAME"

# Remove existing DMG
rm -f "$DMG_PATH"

# Create initial DMG
hdiutil create -volname "Kolosal 1.0.0" -srcfolder "$DMG_STAGING" -ov -format UDRW "$DMG_PATH.tmp"

# Check if the command actually created a .dmg.tmp.dmg file and rename it
if [ -f "$DMG_PATH.tmp.dmg" ]; then
    mv "$DMG_PATH.tmp.dmg" "$DMG_PATH.tmp"
fi

# Mount for configuration (minimal approach)
mount_point="/tmp/kolosal_dmg_mount"

# Clean up any existing mounts and processes
if mountpoint -q "$mount_point" 2>/dev/null; then
    hdiutil detach "$mount_point" -force 2>/dev/null || true
fi
diskutil unmount force "$mount_point" 2>/dev/null || true
if [ -d "$mount_point" ]; then
    rm -rf "$mount_point" 2>/dev/null || true
fi

# Kill any existing hdiutil processes
pkill -f "hdiutil.*kolosal" 2>/dev/null || true
pkill -f "hdiutil.*Kolosal" 2>/dev/null || true
sleep 3

# Create clean mount point and attach
mkdir -p "$mount_point"

echo "⚠️  Skipping DMG appearance configuration to ensure reliable build"
echo "📝 Manual DMG styling can be done later if needed"

# Convert to final compressed DMG
hdiutil convert "$DMG_PATH.tmp" -format UDZO -imagekey zlib-level=9 -o "$DMG_PATH"

# Clean up
rm -f "$DMG_PATH.tmp"

# Step 6: Sign the DMG
echo "✍️  Signing DMG..."
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp "$DMG_PATH"

# Step 7: Verify final DMG
echo "🔍 Final verification..."
codesign --verify --verbose=2 "$DMG_PATH"

# Test the DMG contents
echo "🧪 Testing DMG contents..."
test_mount="/tmp/kolosal_final_test"
if [ -d "$test_mount" ]; then
    hdiutil detach "$test_mount" -force 2>/dev/null || true
    rm -rf "$test_mount"
fi

mkdir -p "$test_mount"
if hdiutil attach "$DMG_PATH" -readonly -nobrowse -mountpoint "$test_mount" 2>/dev/null; then
    codesign --verify --verbose=2 "$test_mount/Kolosal.app"
    echo "✅ DMG app bundle signature verified (spctl check skipped - requires notarization)"
    hdiutil detach "$test_mount" -force 2>/dev/null || true
else
    echo "⚠️  Could not mount DMG for testing, but DMG was created successfully"
fi

echo "✅ Complete signing workflow finished!"
echo "📦 Final DMG: $DMG_PATH"
echo "🔐 Ready for notarization and distribution"

# Clean up staging
rm -rf "$DMG_STAGING"
