#!/bin/bash
# Kolosal CLI Terminal Launcher for macOS App Bundle
# This script launches Kolosal CLI in a new Terminal window

# Get the directory containing this script (MacOS directory in app bundle)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_BUNDLE_DIR="$(dirname "$SCRIPT_DIR")"
FRAMEWORKS_DIR="$APP_BUNDLE_DIR/Frameworks"
RESOURCES_DIR="$APP_BUNDLE_DIR/Resources"

# Set up environment for the app bundle
export DYLD_LIBRARY_PATH="$FRAMEWORKS_DIR:$DYLD_LIBRARY_PATH"
export KOLOSAL_RESOURCES_PATH="$RESOURCES_DIR"

# Function to install kolosal CLI system-wide
install_kolosal() {
    echo "Installing Kolosal CLI to system..."
    
    # Installation directory
    if [ "$EUID" -eq 0 ]; then
        INSTALL_DIR="/usr/local/kolosal"
        BIN_DIR="/usr/local/bin"
    else
        INSTALL_DIR="$HOME/.local/kolosal"
        BIN_DIR="$HOME/.local/bin"
    fi
    
    # Create installation directories
    mkdir -p "$INSTALL_DIR/bin"
    mkdir -p "$INSTALL_DIR/lib"
    mkdir -p "$INSTALL_DIR/etc"
    mkdir -p "$BIN_DIR"
    
    # Copy files from app bundle
    echo "Copying files..."
    cp "$SCRIPT_DIR/kolosal-cli" "$INSTALL_DIR/bin/kolosal"
    cp "$SCRIPT_DIR/kolosal-server" "$INSTALL_DIR/bin/" 2>/dev/null || true
    cp -R "$FRAMEWORKS_DIR/"* "$INSTALL_DIR/lib/" 2>/dev/null || true
    cp "$RESOURCES_DIR/config.yaml" "$INSTALL_DIR/etc/" 2>/dev/null || true
    
    # Set permissions
    chmod +x "$INSTALL_DIR/bin/kolosal"
    chmod +x "$INSTALL_DIR/bin/kolosal-server" 2>/dev/null || true
    
    # Create symlinks for command-line access
    echo "Creating command-line shortcuts..."
    ln -sf "$INSTALL_DIR/bin/kolosal" "$BIN_DIR/kolosal"
    ln -sf "$INSTALL_DIR/bin/kolosal-server" "$BIN_DIR/kolosal-server"
    
    # Update PATH for user installation
    if [ "$EUID" -ne 0 ]; then
        # Add to shell profiles
        for PROFILE in "$HOME/.zshrc" "$HOME/.bash_profile" "$HOME/.bashrc"; do
            if [ -f "$PROFILE" ]; then
                if ! grep -q "$BIN_DIR" "$PROFILE"; then
                    echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$PROFILE"
                    echo "Updated $PROFILE"
                fi
            fi
        done
    fi
    
    # Create user configuration directory
    CONFIG_DIR="$HOME/Library/Application Support/Kolosal"
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$HOME/Library/Logs/Kolosal"
    mkdir -p "$HOME/Library/Caches/Kolosal/models"
    
    # Copy default config if it doesn't exist
    if [ ! -f "$CONFIG_DIR/config.yaml" ] && [ -f "$INSTALL_DIR/etc/config.yaml" ]; then
        cp "$INSTALL_DIR/etc/config.yaml" "$CONFIG_DIR/config.yaml"
        echo "Created default configuration at $CONFIG_DIR/config.yaml"
    fi
    
    echo ""
    echo "Installation completed successfully!"
    echo ""
    echo "Kolosal CLI has been installed to: $INSTALL_DIR"
    echo "Command-line tools available at: $BIN_DIR"
    echo "Configuration stored at: $CONFIG_DIR/config.yaml"
    echo ""
    if [ "$EUID" -ne 0 ]; then
        echo "Please restart your terminal or run: source ~/.zshrc"
    fi
    echo ""
    echo "To get started, run: kolosal --help"
}

# Check if this is being run from inside an app bundle
if [[ "$SCRIPT_DIR" == *.app/Contents/MacOS ]]; then
    # We're in an app bundle, provide installation options
    echo "Kolosal CLI Installer"
    echo "===================="
    echo ""
    echo "Choose an option:"
    echo "1) Install Kolosal CLI system-wide"
    echo "2) Run Kolosal CLI once from app bundle"
    echo "3) Open Terminal and add to PATH temporarily"
    echo ""
    read -p "Enter your choice (1-3): " choice
    
    case $choice in
        1)
            install_kolosal
            ;;
        2)
            echo "Running Kolosal CLI from app bundle..."
            cd "$HOME"
            "$SCRIPT_DIR/kolosal-cli" "$@"
            ;;
        3)
            echo "Opening Terminal with Kolosal CLI in PATH..."
            # Create a temporary script that sets up the environment
            TEMP_SCRIPT=$(mktemp)
            cat > "$TEMP_SCRIPT" << EOF
#!/bin/bash
export PATH="$SCRIPT_DIR:\$PATH"
export DYLD_LIBRARY_PATH="$FRAMEWORKS_DIR:\$DYLD_LIBRARY_PATH"
export KOLOSAL_RESOURCES_PATH="$RESOURCES_DIR"
# Create a symlink for easier access
ln -sf "$SCRIPT_DIR/kolosal-cli" "$SCRIPT_DIR/kolosal" 2>/dev/null || true
echo "Kolosal CLI is now available. Try: kolosal --help"
echo "This is a temporary session. To install permanently, choose option 1."
cd "\$HOME"
exec "\$SHELL"
EOF
            chmod +x "$TEMP_SCRIPT"
            osascript -e "tell app \"Terminal\" to do script \"$TEMP_SCRIPT\""
            ;;
        *)
            echo "Invalid choice. Exiting."
            exit 1
            ;;
    esac
else
    # Direct execution (probably installed system-wide)
    exec "$SCRIPT_DIR/kolosal-cli" "$@"
fi
