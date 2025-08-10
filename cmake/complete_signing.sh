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
# Remove existing staging with permission fix
if [ -d "$DMG_STAGING" ]; then
    chmod -R +w "$DMG_STAGING" 2>/dev/null || true
    rm -rf "$DMG_STAGING"
fi
mkdir -p "$DMG_STAGING/.background"

# Copy signed app bundle
cp -R "$STAGING_DIR/Kolosal.app" "$DMG_STAGING/"

# Create Applications symlink
ln -sf /Applications "$DMG_STAGING/Applications"

# Copy background image
cp "$PROJECT_ROOT/resources/dmg_background.png" "$DMG_STAGING/.background/"

# Step 5: Create DMG using unified script (consistent styling + signing)
echo "📀 Creating final DMG with styling via create_dmg.sh..."
DMG_NAME="kolosal-1.0.0-apple-silicon-signed.dmg"
DMG_PATH="$BUILD_DIR/$DMG_NAME"
VOLNAME="Kolosal"

rm -f "$DMG_PATH"
"$PROJECT_ROOT/cmake/create_dmg.sh" \
    "$DMG_STAGING" \
    "$DMG_PATH" \
    "$VOLNAME" \
    "$PROJECT_ROOT/resources/dmg_background.png" \
    "$CODESIGN_IDENTITY" \
    "$TEAM_ID"

# Step 6: (Optional) Re-sign DMG to ensure timestamp
echo "✍️  Ensuring DMG is signed..."
codesign --force --sign "$CODESIGN_IDENTITY" --timestamp "$DMG_PATH" || true

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
if [ -d "$DMG_STAGING" ]; then
    chmod -R +w "$DMG_STAGING" 2>/dev/null || true
    rm -rf "$DMG_STAGING"
fi
