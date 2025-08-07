#!/bin/bash

# Test script to verify code signing setup
echo "Testing code signing setup for Kolosal CLI..."
echo ""

# Your specific certificate names
APP_CERT="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
INSTALLER_CERT="Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)"
TEAM_ID="SNW8GV8C24"

echo "Looking for certificates..."
echo "Application cert: $APP_CERT"
echo "Installer cert: $INSTALLER_CERT"
echo "Team ID: $TEAM_ID"
echo ""

# Check if certificates exist
echo "Checking keychain for certificates..."
security find-certificate -c "Developer ID Application: Rifky Bujana Bisri" >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Application certificate found in keychain"
else
    echo "✗ Application certificate NOT found in keychain"
fi

security find-certificate -c "Developer ID Installer: Rifky Bujana Bisri" >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Installer certificate found in keychain"
else
    echo "✗ Installer certificate NOT found in keychain"
fi

echo ""

# Check for signing identities (certificates + private keys)
echo "Checking for valid signing identities..."

APP_IDENTITY=$(security find-identity -v -p codesigning | grep "Developer ID Application: Rifky Bujana Bisri")
if [ -n "$APP_IDENTITY" ]; then
    echo "✓ Application signing identity available:"
    echo "  $APP_IDENTITY"
else
    echo "✗ Application signing identity NOT available (missing private key?)"
fi

INSTALLER_IDENTITY=$(security find-identity -v -p basic | grep "Developer ID Installer: Rifky Bujana Bisri")
if [ -n "$INSTALLER_IDENTITY" ]; then
    echo "✓ Installer signing identity available:"
    echo "  $INSTALLER_IDENTITY"
else
    echo "✗ Installer signing identity NOT available (missing private key?)"
fi

echo ""

# Test signing a dummy file
echo "Testing code signing with a dummy file..."
echo "test" > /tmp/test_sign_file

if [ -n "$APP_IDENTITY" ]; then
    codesign -s "$APP_CERT" /tmp/test_sign_file >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "✓ Code signing test successful!"
        codesign -v /tmp/test_sign_file >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "✓ Signature verification successful!"
        else
            echo "✗ Signature verification failed"
        fi
    else
        echo "✗ Code signing test failed"
    fi
else
    echo "⚠ Skipping signing test (no valid identity)"
fi

rm -f /tmp/test_sign_file

echo ""
echo "Summary:"
if [ -n "$APP_IDENTITY" ] && [ -n "$INSTALLER_IDENTITY" ]; then
    echo "🎉 Your certificates are properly set up for code signing!"
    echo ""
    echo "You can now build with:"
    echo "cmake -DENABLE_CODESIGN=ON \\"
    echo "      -DCODESIGN_IDENTITY=\"$APP_CERT\" \\"
    echo "      -DCODESIGN_INSTALLER_IDENTITY=\"$INSTALLER_CERT\" \\"
    echo "      -DDEVELOPER_TEAM_ID=\"$TEAM_ID\" \\"
    echo "      ."
elif [ -z "$APP_IDENTITY" ] && [ -z "$INSTALLER_IDENTITY" ]; then
    echo "❌ No valid signing identities found."
    echo "You need to download the complete certificates (with private keys) from Apple Developer Portal."
    echo ""
    echo "Steps:"
    echo "1. Go to https://developer.apple.com/account/resources/certificates"
    echo "2. Download your Developer ID Application and Developer ID Installer certificates"
    echo "3. Double-click each .cer file to install them in Keychain"
    echo "4. Run this test again"
else
    echo "⚠ Partial setup detected. You have some certificates but missing others."
    echo "Please download all required certificates from Apple Developer Portal."
fi
