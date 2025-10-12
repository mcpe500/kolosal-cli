# macOS .pkg Installer - Quick Guide

## ✅ Solution Summary

The macOS `.pkg` installer now works perfectly! Users can simply install the package and immediately use the `kolosal` command.

## For Developers: Building the Package

```bash
# Single command to build everything
npm run build:mac:pkg
```

Output: `dist/mac/KolosalCode-macos.pkg`

## For End Users: Installing

### Method 1: Double-click the .pkg file
Just double-click `KolosalCode-macos.pkg` and follow the installer.

### Method 2: Command line
```bash
sudo installer -pkg KolosalCode-macos.pkg -target /
```

### Verify Installation
```bash
kolosal --version
```

## How It Works

Instead of creating a single executable binary (which has ES module compatibility issues), we bundle the application with its dependencies:

```
/usr/local/kolosal-app/
├── bin/kolosal              # Launcher script
└── lib/
    ├── bundle/gemini.js     # Application (ES module)
    └── node_modules/        # Bundled dependencies

/usr/local/bin/kolosal       # Symlink (in PATH)
```

The launcher script:
1. Resolves symlinks properly
2. Sets up NODE_PATH for dependencies
3. Executes the application with system Node.js

## Requirements

- macOS (Apple Silicon or Intel)
- Node.js 20+ installed on the system

## Uninstalling

```bash
sudo rm -rf /usr/local/kolosal-app
sudo rm /usr/local/bin/kolosal
sudo pkgutil --forget ai.kolosal.kolosal-code
```

## Technical Details

See `docs/macos-packaging-fix.md` for complete technical documentation.

## Key Files

- `scripts/build-standalone-pkg.js` - Build script
- `package.json` - `build:mac:pkg` script entry
- `README.md` - User documentation
