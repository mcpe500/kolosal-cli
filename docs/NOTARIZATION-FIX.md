# Notarization Failed - Quick Fix

## ❌ The Problem

Your notarization failed with this error:
```
Status: Invalid
Error: The binary is not signed with a valid Developer ID certificate.
```

**Files affected:**
- `node_modules/node-pty/build/Release/pty.node`
- `node_modules/node-pty/build/Release/spawn-helper`
- `node_modules/@lydell/node-pty-darwin-arm64/pty.node`
- `node_modules/@lydell/node-pty-darwin-arm64/spawn-helper`

## ✅ The Solution

You have a **Developer ID Installer** certificate, but you also need a **Developer ID Application** certificate to sign the native binaries.

### Two Certificates Required

| Certificate | Purpose | Signs |
|------------|---------|-------|
| **Developer ID Application** | Code signing | `.node` files, executables, binaries |
| **Developer ID Installer** | Package signing | `.pkg` installer itself |

## 🔧 Fix Steps

### Step 1: Get the Application Certificate

**Via Xcode (easiest):**
1. Open Xcode → Settings → Accounts
2. Select your Apple ID → Manage Certificates
3. Click **+** → Select **Developer ID Application**
4. Close (certificate is now installed)

**Via Developer Portal:**
1. Go to https://developer.apple.com/account/resources/certificates
2. Click **+** → Select **Developer ID Application**
3. Create CSR if needed, download and install

### Step 2: Verify Both Certificates

```bash
security find-identity -v -p basic
```

You should see **BOTH**:
```
1) ABC123... "Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
2) XYZ789... "Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)"
```

### Step 3: Set Environment Variables

```bash
# Add to ~/.zshrc
export CODESIGN_IDENTITY_APP="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
export CODESIGN_IDENTITY="Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)"

# Reload shell
source ~/.zshrc
```

### Step 4: Rebuild with Automatic Signing

The build script has been updated to automatically sign all binaries:

```bash
# Clean previous build
rm -rf dist/mac/*

# Build with automatic signing
npm run build:mac:pkg
```

**What happens:**
1. ✅ Builds standalone app
2. ✅ **Signs all .node files and executables** with Application cert
3. ✅ Creates .pkg installer
4. ✅ Signs .pkg with Installer cert

### Step 5: Notarize Again

```bash
export NOTARIZE=1
npm run build:mac:pkg

# OR manually:
xcrun notarytool submit \
  dist/mac/KolosalCode-macos-signed.pkg \
  --keychain-profile "notarytool-profile" \
  --wait
```

This time it should succeed! ✅

## 🔍 Verify Signing

Check that binaries are signed:

```bash
# Check a .node file
codesign -dv --verbose=4 \
  dist/mac/kolosal-app/lib/node_modules/@lydell/node-pty-darwin-arm64/pty.node

# Should show:
# Authority=Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)
# Timestamp=<date>
# Runtime Version=...

# Check the .pkg
pkgutil --check-signature dist/mac/KolosalCode-macos-signed.pkg

# Should show:
# Status: signed by a developer certificate issued by Apple
```

## 📚 What Changed in the Build Script

The `scripts/build-standalone-pkg.js` now includes:

1. **New function:** `signNativeBinaries()`
   - Finds all `.node` files and executables
   - Signs each with `codesign --sign "$CODESIGN_IDENTITY_APP"`
   - Uses `--options runtime` for hardened runtime
   - Uses `--timestamp` for secure timestamp
   
2. **Automatic execution:** Runs before creating .pkg
   - Signs binaries → Creates package → Signs package

## 🎯 Complete Build Flow

```
Build Process:
1. Bundle app with esbuild → bundle/gemini.js
2. Copy dependencies → node_modules/
3. Create launcher script → bin/kolosal
4. 🆕 Sign all native binaries (.node, spawn-helper)
5. Package into .pkg structure
6. Sign the .pkg installer
7. (Optional) Submit for notarization
8. (Optional) Staple notarization ticket
```

## ⚡ Quick Commands

```bash
# One-time setup
security find-identity -v -p basic  # Find your cert names
export CODESIGN_IDENTITY_APP="Developer ID Application: Your Name (TEAM_ID)"
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"

# Build signed package
npm run build:mac:pkg

# Build, sign, and notarize
export NOTARIZE=1
npm run build:mac:pkg

# Check notarization status
xcrun notarytool info <submission-id> --keychain-profile "notarytool-profile"

# Get detailed logs
xcrun notarytool log <submission-id> --keychain-profile "notarytool-profile"
```

## 📖 Full Documentation

See `docs/CODE-SIGNING.md` for complete details on:
- Getting certificates
- CI/CD setup
- Troubleshooting
- Advanced options

## 🆘 Still Having Issues?

1. **Certificate not found:**
   ```bash
   security find-identity -v
   # Make sure you see BOTH Application and Installer
   ```

2. **Wrong certificate type:**
   ```bash
   # Application cert is for binaries (.node files)
   # Installer cert is for .pkg files
   # You need BOTH!
   ```

3. **Signing fails:**
   ```bash
   # Check certificate validity
   security find-certificate -c "Developer ID Application" -p | openssl x509 -text
   ```

4. **Notarization fails:**
   ```bash
   # Get detailed logs
   xcrun notarytool log <id> --keychain-profile "notarytool-profile"
   ```
