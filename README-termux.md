# KolosalCode on Termux

Run KolosalCode on Android using Termux. No root required.

## Requirements

- [Termux](https://termux.dev/en/) app
- Internet connection
- Storage permission (optional, for accessing files outside Termux)

## Installation

1. **Update Termux**
   Run this to ensure your package lists and installed tools are up to date:
   ```bash
   pkg update -y && pkg upgrade -y
   ```

2. **Install Git**
   ```bash
   pkg install git -y
   ```

3. **Clone the Repository**
   ```bash
   git clone https://github.com/KolosalAI/kolosal-cli.git
   cd kolosal-cli
   ```

4. **Run the Installer**
   Use `bash` to run the installer (do not use `sh`):
   ```bash
   bash install-termux.sh
   ```
   
   This script will:
   - Install required dependencies (Node.js LTS, Python, build tools).
   - Build the project from source using `esbuild`.
   - Install the application to `$PREFIX/opt/kolosal-code`.
   - Create a `kolosal` command in `$PREFIX/bin`.

## Usage

Run the CLI:
```bash
kolosal
```

Check version:
```bash
kolosal --version
```

Get help:
```bash
kolosal --help
```

## Troubleshooting

### "Cannot find module 'tiktoken'"
If you see an error about missing modules, ensure you are using the latest `install-termux.sh` which includes a fix to copy external dependencies correctly. Run the installer again:
```bash
bash install-termux.sh
```

### "Permission denied" or "Command not found"
Ensure you are running the script with `bash`:
```bash
bash install-termux.sh
```
### "Permission denied" or "Command not found"
Ensure you are running the script with `bash` as instructed, not `sh` or directly (`./install-termux.sh`).
```bash
bash install-termux.sh
```

### Node.js Warnings
You might see some warnings during `npm install` or execution. These are usually harmless as long as the installation completes and `kolosal --version` works.
