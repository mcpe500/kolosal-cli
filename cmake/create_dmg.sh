#!/bin/bash

# Custom DMG creation script with background image support and code signing
# Usage: create_dmg.sh <source_dir> <dmg_name> <volume_name> <background_image> [codesign_app_identity] [notarize_team_id]

set -e

SOURCE_DIR="$1"
DMG_NAME="$2"
VOLUME_NAME="$3" 
BACKGROUND_IMAGE="$4"
CODESIGN_APP_IDENTITY="$5"  # Should be Developer ID Application (not Installer)
NOTARIZE_TEAM_ID="$6"

# Temporary directories
TEMP_DMG_DIR="/tmp/${VOLUME_NAME}_temp"
MOUNT_DIR="/tmp/${VOLUME_NAME}_mount"

echo "Creating DMG: $DMG_NAME"
echo "Source: $SOURCE_DIR"
echo "Volume: $VOLUME_NAME"
echo "Background: $BACKGROUND_IMAGE"

if [ -n "$CODESIGN_APP_IDENTITY" ]; then
    echo "Code signing identity (Application): $CODESIGN_APP_IDENTITY"
fi

if [ -n "$NOTARIZE_TEAM_ID" ]; then
    echo "Notarization team ID: $NOTARIZE_TEAM_ID"
fi

# Clean up any existing temp directories
rm -rf "$TEMP_DMG_DIR" "$MOUNT_DIR"
mkdir -p "$TEMP_DMG_DIR" "$MOUNT_DIR"

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory does not exist: $SOURCE_DIR"
    exit 1
fi

# Copy source files to temp directory
if [ -d "$SOURCE_DIR/Kolosal.app" ]; then
    # Copy the app bundle
    cp -R "$SOURCE_DIR/Kolosal.app" "$TEMP_DMG_DIR/"
    echo "Copied Kolosal.app bundle"
    
    # Create symbolic link to Applications for drag-and-drop installation
    ln -s /Applications "$TEMP_DMG_DIR/Applications"
    echo "Created Applications symlink"
else
    # Copy all contents if no app bundle found
    cp -R "$SOURCE_DIR"/* "$TEMP_DMG_DIR/" 2>/dev/null || true
    echo "Copied source directory contents"
fi

# Create .background directory and copy background image
mkdir -p "$TEMP_DMG_DIR/.background"
if [ -f "$BACKGROUND_IMAGE" ]; then
    cp "$BACKGROUND_IMAGE" "$TEMP_DMG_DIR/.background/"
    BACKGROUND_FILE=$(basename "$BACKGROUND_IMAGE")
    echo "Background image copied: $BACKGROUND_FILE"
else
    echo "Warning: Background image not found: $BACKGROUND_IMAGE"
    BACKGROUND_FILE=""
fi

# Ensure we have some content
if [ -z "$(ls -A "$TEMP_DMG_DIR" 2>/dev/null | grep -v '^\.background$')" ]; then
    echo "Error: No content to package"
    exit 1
fi

echo "Contents of temp directory:"
ls -la "$TEMP_DMG_DIR"

# Calculate size of content
CONTENT_SIZE=$(du -sk "$TEMP_DMG_DIR" | cut -f1)
DMG_SIZE=$((CONTENT_SIZE + 50000))  # Add 50MB buffer
echo "Content size: ${CONTENT_SIZE}KB, DMG size: ${DMG_SIZE}KB"

# Create initial DMG with calculated size
echo "Creating initial DMG..."
hdiutil create -srcfolder "$TEMP_DMG_DIR" -volname "$VOLUME_NAME" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size "${DMG_SIZE}k" "/tmp/${VOLUME_NAME}_temp.dmg"

# Mount the DMG
echo "Mounting DMG..."
DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "/tmp/${VOLUME_NAME}_temp.dmg" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

if [ -z "$DEVICE" ]; then
    echo "Error: Failed to mount DMG"
    exit 1
fi

echo "Mounted device: $DEVICE"

# Find the mount point
MOUNT_POINT="/Volumes/$VOLUME_NAME"

if [ ! -d "$MOUNT_POINT" ]; then
    echo "Error: Could not find mount point $MOUNT_POINT"
    hdiutil detach "$DEVICE" || true
    exit 1
fi

echo "DMG mounted at: $MOUNT_POINT"

# Configure DMG appearance with AppleScript
echo "Configuring DMG appearance..."
if [ -n "$BACKGROUND_FILE" ]; then
echo "Configuring DMG appearance... (background: $BACKGROUND_FILE)"

osascript << EOF
tell application "Finder"
    tell disk "$VOLUME_NAME"
        open
        set theWindow to container window
        set current view of theWindow to icon view
        set toolbar visible of theWindow to false
        set statusbar visible of theWindow to false
        set the bounds of theWindow to {100, 100, 740, 580}
        
        set theViewOptions to the icon view options of theWindow
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 128
        
        -- Set background image using correct DMG path syntax
        try
            set bgFile to ".background:$BACKGROUND_FILE"
            set background picture of theViewOptions to file bgFile
        on error err_msg
            log "Background setting error: " & err_msg
        end try
        
        -- Position items
        try
            set position of item "Kolosal.app" to {160, 200}
        end try
        
        try
            set position of item "Applications" to {480, 200}
        end try
        
        -- Update and refresh
        update without registering applications
        delay 2
        
        -- Close and reopen to ensure changes are saved
        close
        delay 1
        open
        delay 1
        close
    end tell
end tell
EOF
else
# Configure without background image
osascript -e "
tell application \"Finder\"
    tell disk \"$VOLUME_NAME\"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {100, 100, 740, 580}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 128
        
        -- Position items if they exist
        try
            set position of item \"Kolosal.app\" to {160, 200}
        end try
        
        try
            set position of item \"Applications\" to {480, 200}  
        end try
        
        -- Update and close
        update without registering applications
        delay 2
        close
    end tell
end tell
"
fi

# Hide the .background folder if possible (optional)
if command -v SetFile >/dev/null 2>&1; then
    SetFile -a V "$MOUNT_POINT/.background" 2>/dev/null || true
fi

# Wait for Finder to persist .DS_Store
echo "Waiting for Finder to write .DS_Store..."
STORE_PATH="$MOUNT_POINT/.DS_Store"
for i in 1 2 3 4 5; do
    if [ -f "$STORE_PATH" ] && [ -s "$STORE_PATH" ]; then
        break
    fi
    sleep 2
done

# Extra flush
sync || true

# Unmount with retries
echo "Unmounting DMG..."
DETACH_OK=0
for i in 1 2 3 4 5; do
    if hdiutil detach "$DEVICE" -quiet 2>/dev/null; then
        DETACH_OK=1
        break
    fi
    echo "Retrying detach ($i/5)..."
    sleep 2
done

if [ $DETACH_OK -ne 1 ]; then
    echo "Warning: Failed to cleanly detach $DEVICE; attempting force detach"
    hdiutil detach "$DEVICE" -force 2>/dev/null || true
fi

# Create final compressed DMG
echo "Creating final compressed DMG..."
hdiutil convert "/tmp/${VOLUME_NAME}_temp.dmg" -format UDBZ -o "$DMG_NAME"

# Sign the DMG if code signing identity is provided
if [ -n "$CODESIGN_APP_IDENTITY" ]; then
    echo "Signing DMG with Application identity: $CODESIGN_APP_IDENTITY"
    codesign --force --sign "$CODESIGN_APP_IDENTITY" --timestamp "$DMG_NAME"
    
    if [ $? -eq 0 ]; then
        echo "DMG signed successfully"
        
        # Verify the signature
        echo "Verifying DMG signature..."
        codesign --verify --verbose "$DMG_NAME"
        
        # Submit for notarization if team ID is provided
        if [ -n "$NOTARIZE_TEAM_ID" ]; then
            echo "Submitting DMG for notarization..."
            echo "Note: This requires Apple ID credentials to be stored first."
            echo "If this fails, run: xcrun notarytool store-credentials --help"
            
            # Try notarization with stored credentials profile
            xcrun notarytool submit "$DMG_NAME" --keychain-profile "kolosal-profile" --wait 2>/dev/null
            
            if [ $? -ne 0 ]; then
                echo "Notarization with stored profile failed, trying with team-id only..."
                echo "You may be prompted for Apple ID and password..."
                xcrun notarytool submit "$DMG_NAME" --team-id "$NOTARIZE_TEAM_ID" --wait
            fi
            
            if [ $? -eq 0 ]; then
                echo "Notarization successful"
                
                # Staple the notarization
                echo "Stapling notarization to DMG..."
                xcrun stapler staple "$DMG_NAME"
                
                if [ $? -eq 0 ]; then
                    echo "Notarization stapled successfully"
                else
                    echo "Warning: Failed to staple notarization"
                fi
            else
                echo "Warning: Notarization failed."
                echo "To set up notarization credentials, run:"
                echo "  xcrun notarytool store-credentials 'kolosal-profile' \\"
                echo "    --apple-id 'your-apple-id@example.com' \\"
                echo "    --team-id '$NOTARIZE_TEAM_ID' \\"
                echo "    --password 'app-specific-password'"
                echo ""
                echo "DMG is signed and ready for distribution, but not notarized."
            fi
        else
            echo "No team ID provided, skipping notarization"
        fi
    else
        echo "Warning: Failed to sign DMG"
    fi
else
    echo "No code signing identity provided, DMG will not be signed"
fi

# Clean up
rm -f "/tmp/${VOLUME_NAME}_temp.dmg"
rm -rf "$TEMP_DMG_DIR" "$MOUNT_DIR"

echo "DMG created successfully: $DMG_NAME"
