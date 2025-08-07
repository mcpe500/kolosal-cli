#!/bin/bash

# Script to set up code signing for Kolosal CLI
# This script will import the Developer ID certificates and configure signing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CERT_DIR="$SCRIPT_DIR/../cert"

echo "Setting up code signing for Kolosal CLI..."

# Check if certificate files exist
if [ ! -f "$CERT_DIR/developerID_application.cer" ]; then
    echo "Error: Application certificate not found at $CERT_DIR/developerID_application.cer"
    exit 1
fi

if [ ! -f "$CERT_DIR/developerID_installer.cer" ]; then
    echo "Error: Installer certificate not found at $CERT_DIR/developerID_installer.cer"
    exit 1
fi

echo "Found certificate files:"
echo "  Application: $CERT_DIR/developerID_application.cer"
echo "  Installer: $CERT_DIR/developerID_installer.cer"

# Check for p12 files (private keys)
P12_FILES=("$CERT_DIR"/*.p12)
if [ ${#P12_FILES[@]} -gt 0 ] && [ -f "${P12_FILES[0]}" ]; then
    echo "Found p12 files (private keys):"
    for p12_file in "$CERT_DIR"/*.p12; do
        if [ -f "$p12_file" ]; then
            echo "  $(basename "$p12_file")"
        fi
    done
    echo ""
    echo "To import p12 files with private keys, run:"
    echo "  security import \"path/to/file.p12\" -k ~/Library/Keychains/login.keychain"
    echo "  (You'll be prompted for the p12 password)"
    echo ""
else
    echo "No p12 files found - you'll need the private keys for signing"
fi

# Import certificates into keychain
echo ""
echo "Importing certificates into keychain..."

# Import application certificate
security import "$CERT_DIR/developerID_application.cer" -k ~/Library/Keychains/login.keychain

# Import installer certificate  
security import "$CERT_DIR/developerID_installer.cer" -k ~/Library/Keychains/login.keychain

echo "Certificates imported successfully!"

# List available signing identities
echo ""
echo "Available code signing identities:"
security find-identity -v -p codesigning

echo ""
echo "Available installer signing identities:"
security find-identity -v -p basic

echo ""
echo "Setup complete!"
echo ""
echo "To use code signing when building:"
echo "1. Find your Developer ID Application identity from the list above"
echo "2. Find your Developer ID Installer identity from the list above"
echo "3. Run CMake with the following options:"
echo ""
echo "cmake -DENABLE_CODESIGN=ON \\"
echo "      -DCODESIGN_IDENTITY=\"Developer ID Application: Your Name (XXXXXXXXXX)\" \\"
echo "      -DCODESIGN_INSTALLER_IDENTITY=\"Developer ID Installer: Your Name (XXXXXXXXXX)\" \\"
echo "      -DDEVELOPER_TEAM_ID=\"XXXXXXXXXX\" \\"
echo "      ."
echo ""
echo "Then build with:"
echo "make package    # For standard signed DMG"
echo "make dmg        # For custom DMG with background and signing"
echo ""
echo "Note: Replace 'Your Name (XXXXXXXXXX)' with the actual identity names from the list above"
echo "      Replace 'XXXXXXXXXX' with your actual Apple Developer Team ID"
echo ""
echo "For notarization, you'll also need to:"
echo "1. Store your Apple ID credentials: xcrun notarytool store-credentials"
echo "2. Make sure your Team ID is correct in the DEVELOPER_TEAM_ID variable"
