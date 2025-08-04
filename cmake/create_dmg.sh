#!/bin/bash

# Custom DMG creation script with background image support
# Usage: create_dmg.sh <source_dir> <dmg_name> <volume_name> <background_image>

set -e

SOURCE_DIR="$1"
DMG_NAME="$2"
VOLUME_NAME="$3" 
BACKGROUND_IMAGE="$4"

# Temporary directories
TEMP_DMG_DIR="/tmp/${VOLUME_NAME}_temp"
MOUNT_DIR="/tmp/${VOLUME_NAME}_mount"

echo "Creating DMG: $DMG_NAME"
echo "Source: $SOURCE_DIR"
echo "Volume: $VOLUME_NAME"
echo "Background: $BACKGROUND_IMAGE"

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
if [ -n "$BACKGROUND_FILE" ]; then
echo "Configuring DMG appearance..."

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
            set background picture of theViewOptions to file ".background:dmg_background.png"
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

# Wait for changes to be applied
sleep 5

# Unmount
echo "Unmounting DMG..."
hdiutil detach "$DEVICE"

# Create final compressed DMG
echo "Creating final compressed DMG..."
hdiutil convert "/tmp/${VOLUME_NAME}_temp.dmg" -format UDBZ -o "$DMG_NAME"

# Clean up
rm -f "/tmp/${VOLUME_NAME}_temp.dmg"
rm -rf "$TEMP_DMG_DIR" "$MOUNT_DIR"

echo "DMG created successfully: $DMG_NAME"
