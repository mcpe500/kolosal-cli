#!/bin/bash

# Kolosal macOS App Launcher Script
# This script is executed when the user double-clicks the Kolosal.app
# It sets up PATH access and provides user-friendly setup

# Get the directory where this script is located (Contents/MacOS)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_CONTENTS="$(dirname "$SCRIPT_DIR")"
APP_BUNDLE="$(dirname "$APP_CONTENTS")"

# Paths to the executables
KOLOSAL_EXECUTABLE="$SCRIPT_DIR/kolosal"
KOLOSAL_SERVER_EXECUTABLE="$SCRIPT_DIR/kolosal-server"

# Function to create symlinks in /usr/local/bin
create_symlinks() {
    local target_dir="/usr/local/bin"
    
    # Check if /usr/local/bin exists and is writable, or if we can create it
    if [[ ! -d "$target_dir" ]]; then
        # Use osascript to run mkdir with admin privileges
        if ! osascript -e "do shell script \"mkdir -p '$target_dir'\" with administrator privileges" 2>/dev/null; then
            echo "Failed to create $target_dir directory"
            return 1
        fi
    fi
    
    # Create both symlinks in a single osascript call to avoid double authentication
    if [[ -f "$KOLOSAL_EXECUTABLE" && -f "$KOLOSAL_SERVER_EXECUTABLE" ]]; then
        local combined_command="ln -sf '$KOLOSAL_EXECUTABLE' '$target_dir/kolosal' && ln -sf '$KOLOSAL_SERVER_EXECUTABLE' '$target_dir/kolosal-server'"
        if ! osascript -e "do shell script \"$combined_command\" with administrator privileges" 2>/dev/null; then
            echo "Failed to create symlinks"
            return 1
        fi
        echo "✓ Created symlink: $target_dir/kolosal"
        echo "✓ Created symlink: $target_dir/kolosal-server"
    else
        echo "Error: One or both executables not found"
        return 1
    fi
    
    return 0
}

# Function to show notification
show_notification() {
    local title="$1"
    local message="$2"
    
    osascript -e "display notification \"$message\" with title \"$title\"" 2>/dev/null || true
}

# Function to check if symlinks already exist and are correct
check_symlinks() {
    local target_dir="/usr/local/bin"
    local kolosal_ok=false
    local server_ok=false
    
    # Check kolosal symlink
    if [[ -L "$target_dir/kolosal" ]]; then
        local current_target="$(readlink "$target_dir/kolosal")"
        if [[ "$current_target" == "$KOLOSAL_EXECUTABLE" ]]; then
            kolosal_ok=true
        fi
    fi
    
    # Check kolosal-server symlink
    if [[ -L "$target_dir/kolosal-server" ]]; then
        local current_target="$(readlink "$target_dir/kolosal-server")"
        if [[ "$current_target" == "$KOLOSAL_SERVER_EXECUTABLE" ]]; then
            server_ok=true
        fi
    fi
    
    # Return true only if both are correctly set up
    if [[ "$kolosal_ok" == true && "$server_ok" == true ]]; then
        return 0
    fi
    
    return 1
}

# Function to show setup dialog
show_setup_dialog() {
    local message="Welcome to Kolosal CLI!\n\nTo use Kolosal commands (kolosal, kolosal-server) from any Terminal window, we can set up command line access.\n\nThis will:\n• Create symlinks in /usr/local/bin\n• Require administrator privileges\n• Make commands available globally\n\nAlternatively, you can skip this and use the executables directly from the app bundle."
    
    local response
    response=$(osascript -e "display dialog \"$message\" buttons {\"Skip\", \"Set Up Command Line Access\"} default button \"Set Up Command Line Access\" with title \"Kolosal Setup\" with icon note" 2>/dev/null)
    
    if [[ "$response" == *"Set Up Command Line Access"* ]]; then
        return 0
    else
        return 1
    fi
}

# Function to show alternative setup instructions
show_manual_setup() {
    local manual_path="export PATH=\"$SCRIPT_DIR:\$PATH\""
    local alias_kolosal="alias kolosal='$KOLOSAL_EXECUTABLE'"
    local alias_server="alias kolosal-server='$KOLOSAL_SERVER_EXECUTABLE'"
    
    local message="Command line setup was not completed.\n\nTo use Kolosal from Terminal manually, you can:\n\n1. Add to your shell profile (~/.zshrc or ~/.bash_profile):\n   $manual_path\n\n2. Or create aliases:\n   $alias_kolosal\n   $alias_server\n\n3. Or run directly:\n   $KOLOSAL_EXECUTABLE\n   $KOLOSAL_SERVER_EXECUTABLE"
    
    osascript -e "display dialog \"$message\" buttons {\"Copy Path to Clipboard\", \"OK\"} default button \"OK\" with title \"Kolosal Manual Setup\"" 2>/dev/null
    
    if [[ $? -eq 0 ]]; then
        # Check if user clicked "Copy Path to Clipboard"
        local button_response
        button_response=$(osascript -e "display dialog \"$message\" buttons {\"Copy Path to Clipboard\", \"OK\"} default button \"OK\" with title \"Kolosal Manual Setup\"" 2>/dev/null)
        if [[ "$button_response" == *"Copy Path to Clipboard"* ]]; then
            echo "$SCRIPT_DIR" | pbcopy
            show_notification "Kolosal" "Path copied to clipboard!"
        fi
    fi
}

# Function to show success message and open terminal
show_success() {
    local message="✅ Kolosal command line access is now set up!\n\nYou can now use these commands in any Terminal window:\n• kolosal --help\n• kolosal-server --help\n\nWould you like to open Terminal to try it out?"
    
    local response
    response=$(osascript -e "display dialog \"$message\" buttons {\"Later\", \"Open Terminal\"} default button \"Open Terminal\" with title \"Kolosal Setup Complete\"" 2>/dev/null)
    
    if [[ "$response" == *"Open Terminal"* ]]; then
        # Open terminal with helpful commands
        osascript -e 'tell application "Terminal"
            activate
            do script "echo \"🎉 Kolosal is now available! Try these commands:\"; echo \"  kolosal --help\"; echo \"  kolosal-server --help\"; echo \"\""
        end tell' 2>/dev/null || true
    fi
}

# Main execution
main() {
    # Check if this is being run from within the app bundle
    if [[ ! -f "$KOLOSAL_EXECUTABLE" ]]; then
        echo "Error: kolosal executable not found at $KOLOSAL_EXECUTABLE"
        show_notification "Kolosal Error" "Installation appears to be corrupted"
        exit 1
    fi
    
    # Check if symlinks are already set up correctly
    if check_symlinks; then
        echo "✅ Kolosal commands are already accessible from Terminal"
        show_notification "Kolosal" "Commands are ready to use in Terminal"
        
        # Show reminder message
        local message="Kolosal commands are already set up and ready to use!\n\n• kolosal --help\n• kolosal-server --help\n\nWould you like to open Terminal?"
        local response
        response=$(osascript -e "display dialog \"$message\" buttons {\"No\", \"Open Terminal\"} default button \"Open Terminal\" with title \"Kolosal Ready\"" 2>/dev/null)
        
        if [[ "$response" == *"Open Terminal"* ]]; then
            osascript -e 'tell application "Terminal"
                activate
                do script "echo \"Kolosal commands are ready:\"; echo \"  kolosal --help\"; echo \"  kolosal-server --help\"; echo \"\""
            end tell' 2>/dev/null || true
        fi
    else
        # Show setup dialog
        if show_setup_dialog; then
            echo "Setting up command line access..."
            
            if create_symlinks; then
                echo "✅ Successfully created command line access!"
                show_notification "Kolosal" "Setup complete! Commands available in Terminal"
                show_success
            else
                echo "❌ Failed to set up command line access"
                show_notification "Kolosal" "Setup failed - you may need administrator privileges"
                show_manual_setup
            fi
        else
            echo "User chose to skip automatic setup"
            show_notification "Kolosal" "App bundle is ready to use"
            show_manual_setup
        fi
    fi
}

# Run the main function
main "$@"
