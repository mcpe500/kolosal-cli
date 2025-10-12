# Kolosal Code - macOS Distribution Guide

## ✅ YES - Distribute ONLY the .pkg file!

You only need to share one file: **`KolosalCode-macos.pkg`**

## 📦 Package Details

- **File**: `KolosalCode-macos.pkg`
- **Size**: 2.7 MB (compressed)
- **Installed Size**: ~15 MB
- **Architecture**: Universal (Apple Silicon + Intel)

## 📁 What's Included in the Package

The `.pkg` is a complete, self-contained installer that includes:

✅ Application bundle (13 MB)
  - Bundled JavaScript code
  - All inline dependencies
  - Sandbox configuration files

✅ External dependencies (2 MB)
  - node-pty native binaries
  - Required for terminal functionality

✅ Installation scripts
  - Launcher with symlink resolution
  - Automatic PATH setup

## 🎯 User Requirements

Users need to have installed:
- **macOS** (any recent version)
- **Node.js 20+** ([download here](https://nodejs.org))
- That's it!

## 🚀 Distribution Options

### Option 1: Direct Download
Host the `.pkg` file on:
- GitHub Releases
- Your website
- Cloud storage (Dropbox, Google Drive, etc.)

### Option 2: GitHub Releases (Recommended)
```bash
# Create a release with the .pkg
gh release create v0.0.14 \
  dist/mac/KolosalCode-macos.pkg \
  --title "Kolosal Code v0.0.14" \
  --notes "macOS installer package"
```

### Option 3: Website Download
Simply provide a download link to the `.pkg` file.

## 👥 User Installation Instructions

Share these instructions with your users:

```markdown
## Installing Kolosal Code on macOS

### Step 1: Download
Download `KolosalCode-macos.pkg`

### Step 2: Install
Double-click the `.pkg` file and follow the installer, or:
```bash
sudo installer -pkg KolosalCode-macos.pkg -target /
```

### Step 3: Verify
```bash
kolosal --version
```

### Requirements
- macOS (Apple Silicon or Intel)
- Node.js 20 or higher ([Download](https://nodejs.org))
```

## 📊 Size Comparison

| Distribution Method | Size | What's Included |
|---------------------|------|-----------------|
| `.pkg` installer | 2.7 MB | Everything (app + deps) |
| npm package | ~500 KB | Source only (deps separate) |
| Source code | ~200 MB | Full repo with dev deps |

## ✅ Verification Checklist

Before distributing, verify:

- [ ] Package builds successfully: `npm run build:mac:pkg`
- [ ] Package file exists: `dist/mac/KolosalCode-macos.pkg`
- [ ] Package size is reasonable: ~2-3 MB
- [ ] Clean install works: Test on a different Mac
- [ ] Version is correct in installer
- [ ] Command works after install: `kolosal --version`

## 🔐 Optional: Code Signing (For Production)

For production distribution, consider:

1. **Code Sign** the package:
   ```bash
   productsign --sign "Developer ID Installer: Your Name" \
     KolosalCode-macos.pkg \
     KolosalCode-macos-signed.pkg
   ```

2. **Notarize** for Gatekeeper:
   ```bash
   xcrun notarytool submit KolosalCode-macos-signed.pkg \
     --keychain-profile "notary-profile" \
     --wait
   ```

3. **Staple** the notarization:
   ```bash
   xcrun stapler staple KolosalCode-macos-signed.pkg
   ```

## 📝 Sample Release Notes Template

```markdown
## Kolosal Code v0.0.14 - macOS Installer

### Installation

Download `KolosalCode-macos.pkg` and install:
- **GUI**: Double-click the .pkg file
- **CLI**: `sudo installer -pkg KolosalCode-macos.pkg -target /`

### Requirements
- macOS (Apple Silicon or Intel)  
- Node.js 20+

### After Installation
Run `kolosal --version` to verify installation.

### File Details
- Size: 2.7 MB
- SHA256: [add checksum]
```

## 🎉 Summary

**YES** - Just distribute the single `.pkg` file! It contains everything users need except Node.js (which they need to install separately).

**File to distribute**: `dist/mac/KolosalCode-macos.pkg` (2.7 MB)
