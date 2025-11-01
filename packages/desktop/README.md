# Kolosal Desktop

A desktop application for the Kolosal CLI built with Tauri.

## Features

- **Native Desktop App**: Cross-platform desktop application using Tauri
- **Server Management**: Start and stop the Kolosal CLI server from the UI
- **Chat Interface**: Clean, responsive chat interface for interacting with Kolosal
- **Real-time Status**: Monitor server status and connection health
- **Full Tool Access**: Complete access to all Kolosal tools including write_file, edit, and run_shell_command

## Prerequisites

- Node.js 18+ 
- Rust 1.70+
- Kolosal CLI (this app expects to be in the same directory as the kolosal-code project)

## Development

### Install Dependencies
```bash
npm install
```

### Run in Development Mode
```bash
npm run tauri dev
```

### Build for Production
```bash
npm run tauri build
```

## Usage

1. **Start the Application**: Launch the desktop app
2. **Start Server**: Click "Start Server" to initialize the Kolosal CLI server
3. **Chat**: Type your messages and press Enter or click Send
4. **Monitor Status**: The server status indicator shows connection health

## Architecture

- **Frontend**: Vanilla JavaScript with Vite
- **Backend**: Rust with Tauri
- **Communication**: HTTP API calls to Kolosal CLI server (port 38080)
- **Process Management**: Rust manages the npm process for the CLI server

## Server Integration

The desktop app automatically:
- Starts the Kolosal CLI in `--server-only --no_ui_output` mode
- Manages the server process lifecycle
- Handles API communication on port 38080
- Provides real-time status updates

## Build Outputs

- **macOS**: `.app` bundle and `.dmg` installer
- **Windows**: `.exe` installer
- **Linux**: `.deb` and `.AppImage` packages

## Configuration

The app is configured to:
- Use API port 38080
- Connect to 127.0.0.1
- Auto-detect the Kolosal CLI directory (expects sibling directory structure)

## Troubleshooting

1. **Server won't start**: Ensure the Kolosal CLI is properly built and accessible
2. **Connection issues**: Check that port 38080 is not in use by another application
3. **Build failures**: Verify Rust and Node.js versions meet requirements