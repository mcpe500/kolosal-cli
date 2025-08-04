on run (volumeName)
    tell application "Finder"
        tell disk (volumeName as string)
            open
            
            set theXOrigin to 100
            set theYOrigin to 100
            set theWidth to 640
            set theHeight to 480
            
            set theBottomRightX to (theXOrigin + theWidth)
            set theBottomRightY to (theYOrigin + theHeight)
            
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set the bounds of container window to {theXOrigin, theYOrigin, theBottomRightX, theBottomRightY}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to 128
            set background picture of viewOptions to file ".background:dmg_background.png"
            
            -- Position items in the DMG window
            try
                set position of item "Kolosal.app" to {160, 200}
            end try
            
            try
                set position of item "Applications" to {480, 200}
            end try
            
            -- Update and close
            update without registering applications
            delay 2
            close
        end tell
    end tell
end run
