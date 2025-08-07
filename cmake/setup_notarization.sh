#!/bin/bash

# Script to set up notarization credentials for Kolosal CLI
echo "Setting up Apple notarization credentials for Kolosal CLI"
echo "========================================================"
echo ""

# Your team ID
TEAM_ID="SNW8GV8C24"

echo "Your Apple Developer Team ID: $TEAM_ID"
echo ""
echo "You'll need:"
echo "1. Your Apple ID email address"
echo "2. An App-Specific Password (NOT your regular Apple ID password)"
echo ""
echo "To create an App-Specific Password:"
echo "1. Go to: https://appleid.apple.com/account/manage"
echo "2. Sign in with your Apple ID"
echo "3. In Security section → Generate Password under App-Specific Passwords"
echo "4. Enter label: 'Kolosal Notarization'"
echo "5. Copy the password (format: xxxx-xxxx-xxxx-xxxx)"
echo ""

read -p "Enter your Apple ID email: " APPLE_ID

if [ -z "$APPLE_ID" ]; then
    echo "Error: Apple ID email is required"
    exit 1
fi

echo ""
echo "Now enter your App-Specific Password when prompted..."
echo ""

# Store the credentials
xcrun notarytool store-credentials "kolosal-profile" \
    --apple-id "$APPLE_ID" \
    --team-id "$TEAM_ID"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Credentials stored successfully!"
    echo ""
    echo "You can now build with notarization:"
    echo "  make dmg"
    echo ""
    echo "Or build with CMake configuration:"
    echo "  cmake -DENABLE_CODESIGN=ON \\"
    echo "        -DCODESIGN_IDENTITY=\"Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)\" \\"
    echo "        -DCODESIGN_INSTALLER_IDENTITY=\"Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)\" \\"
    echo "        -DDEVELOPER_TEAM_ID=\"$TEAM_ID\" \\"
    echo "        ."
else
    echo ""
    echo "❌ Failed to store credentials."
    echo "Please check your Apple ID and App-Specific Password."
    echo ""
    echo "Common issues:"
    echo "- Using regular Apple ID password instead of App-Specific Password"
    echo "- Incorrect Apple ID email"
    echo "- Network connectivity issues"
fi
