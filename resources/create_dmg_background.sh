#!/bin/bash
# Create a professional DMG background with the Kolosal icon

# Check if we have the right tools
if ! command -v sips >/dev/null 2>&1; then
    echo "Error: sips command not found"
    exit 1
fi

cd "$(dirname "$0")"

# Create a base background (600x400 for DMG)
WIDTH=600
HEIGHT=400

# Create a temporary background using a solid color
sips --setPadColor FFFFFF --padToHeightWidth $HEIGHT $WIDTH --setProperty format png "icons/Kolosal Icon-macOS-Default-256x256@1x.png" --out temp_bg.png 2>/dev/null

# If that doesn't work, create a simple background another way
if [ ! -f temp_bg.png ]; then
    # Create a simple gradient background using ImageMagick if available
    if command -v convert >/dev/null 2>&1; then
        convert -size ${WIDTH}x${HEIGHT} gradient:'#f5f5f7-#e8e8ed' dmg_background.png
        
        # Add the Kolosal icon as a watermark
        if [ -f "icons/Kolosal Icon-macOS-Default-256x256@1x.png" ]; then
            convert dmg_background.png \
                "icons/Kolosal Icon-macOS-Default-256x256@1x.png" \
                -gravity center -geometry +150+0 -compose over -composite \
                dmg_background.png
        fi
        
        # Add text using ImageMagick
        convert dmg_background.png \
            -font "Helvetica" -pointsize 14 -fill "#666666" \
            -gravity south -annotate +0+20 "Drag Kolosal CLI.app to Applications to install" \
            dmg_background.png
    else
        echo "Neither sips nor ImageMagick available. Creating simple background..."
        # Just copy an icon as background for now
        cp "icons/Kolosal Icon-macOS-Default-512x512@1x.png" dmg_background.png 2>/dev/null || touch dmg_background.png
    fi
else
    mv temp_bg.png dmg_background.png
fi

echo "DMG background created/updated: dmg_background.png"
