# Code Signing Setup for Kolosal CLI on macOS

This guide explains how to set up code signing for the Kolosal CLI macOS package using the Developer ID certificates in the `cert` folder.

## Prerequisites

1. **Apple Developer Account**: You need an Apple Developer account with Developer ID certificates
2. **Certificates**: The `cert` folder should contain:
   - `developerID_application.cer` - For signing applications and libraries
   - `developerID_installer.cer` - For signing installers and DMG files
3. **Xcode Command Line Tools**: Install with `xcode-select --install`

## Quick Setup

### 1. Import Certificates

Run the setup script to import your certificates:

```bash
./cmake/setup_codesign.sh
```

This script will:
- Import both Developer ID certificates into your keychain
- Display available signing identities
- Show you the exact CMake configuration needed

### 2. Configure CMake with Code Signing

```bash
cmake -DENABLE_CODESIGN=ON \
      -DCODESIGN_IDENTITY="Developer ID Application: Your Name (XXXXXXXXXX)" \
      -DCODESIGN_INSTALLER_IDENTITY="Developer ID Installer: Your Name (XXXXXXXXXX)" \
      -DDEVELOPER_TEAM_ID="XXXXXXXXXX" \
      .
```

Replace the placeholder values with your actual:
- **Developer ID Application**: The exact name from `security find-identity -v -p codesigning`
- **Developer ID Installer**: The exact name from `security find-identity -v -p basic`  
- **Team ID**: Your 10-character Apple Developer Team ID

**For your specific setup:**
```bash
cmake -DENABLE_CODESIGN=ON \
      -DCODESIGN_IDENTITY="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)" \
      -DCODESIGN_INSTALLER_IDENTITY="Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)" \
      -DDEVELOPER_TEAM_ID="SNW8GV8C24" \
      .
```

**Important Certificate Usage:**
- **Developer ID Application**: Used for signing applications, libraries, and DMG files
- **Developer ID Installer**: Used only for signing `.pkg` installer packages (not DMG files)

The build system will automatically use the correct certificate for each type of file.

**Important**: The `.cer` files in your `cert` folder contain only the public certificates. To actually sign code, you need the private keys. You have the `.p12` files which contain the private keys.

#### Import the Private Keys

You have p12 files in your cert folder that contain the private keys. Import them:

```bash
# Import the application signing key (you'll be prompted for password)
security import "cert/Kolosal, Inc. App Key.p12" -k ~/Library/Keychains/login.keychain

# Import the installer signing key (you'll be prompted for password)  
security import "cert/Kolosal, Inc. Key.p12" -k ~/Library/Keychains/login.keychain
```

After importing, verify the setup works:
```bash
./cmake/test_codesign.sh
```

#### Alternative Options if Above Doesn't Work:

**Option A**: Download from Apple Developer Portal
1. Go to [Apple Developer Portal](https://developer.apple.com/account/resources/certificates)
2. Find your "Developer ID Application" and "Developer ID Installer" certificates
3. Download them (they will be `.cer` files, but if you created them on this Mac, the private keys should already be in your keychain)

**Option B**: Double-click the p12 files
1. Double-click `cert/Kolosal, Inc. App Key.p12` in Finder
2. Double-click `cert/Kolosal, Inc. Key.p12` in Finder  
3. Enter the password when prompted
4. This will import both certificate and private key

### 3. Build and Package

```bash
# Build the project
make

# Create a standard signed DMG
make package

# Create a custom DMG with background image and signing
make dmg
```

## What Gets Signed

When code signing is enabled, the build process will sign:

1. **External Libraries** (in Frameworks/):
   - libssl.3.dylib
   - libcrypto.3.dylib
   - libkolosal_server.dylib
   - libllama-metal.dylib
   - All other bundled dependencies

2. **Executables** (in MacOS/):
   - kolosal (main CLI)
   - kolosal-server (server executable)
   - kolosal-launcher (launcher script)

3. **App Bundle**:
   - The entire Kolosal.app bundle

4. **DMG File**:
   - The final distribution DMG

## Notarization

If you provide a `DEVELOPER_TEAM_ID`, the build process will automatically:

1. Submit the signed DMG to Apple for notarization
2. Wait for notarization to complete
3. Staple the notarization ticket to the DMG

### Setting up Notarization Credentials

Before building with notarization, you need to store your Apple ID credentials. You have two options:

#### Option 1: Store credentials in keychain (Recommended)
```bash
xcrun notarytool store-credentials "kolosal-profile" \
  --apple-id "your-apple-id@example.com" \
  --team-id "SNW8GV8C24" \
  --password "your-app-specific-password"
```

#### Option 2: Skip credential storage (Manual authentication)
If you don't store credentials, you'll be prompted for your Apple ID and app-specific password during each build.

**Important**: You need an **App-Specific Password**, not your regular Apple ID password:
1. Go to [Apple ID Account Page](https://appleid.apple.com/account/manage)
2. Sign in with your Apple ID
3. In the "Security" section, click "Generate Password" under "App-Specific Passwords"
4. Enter a label like "Kolosal Notarization"
5. Copy the generated password (format: xxxx-xxxx-xxxx-xxxx)
6. Use this password in the notarytool command above

## Troubleshooting

### Certificate Issues

If you get certificate errors:

1. **Check certificate installation**:
   ```bash
   security find-identity -v -p codesigning
   security find-identity -v -p basic
   ```

2. **If you see "0 valid identities found"** (common issue):
   This means you have the certificates but not the private keys. You need to:
   
   **Option A**: Download complete certificates from Apple Developer Portal
   - Go to https://developer.apple.com/account/resources/certificates
   - Download your Developer ID certificates
   - Double-click to install them
   
   **Option B**: If certificates were created on another Mac
   - You need to export them as `.p12` files from the original Mac
   - Include private keys when exporting
   - Import the `.p12` files on this Mac

3. **Import certificates manually**:
   ```bash
   security import cert/developerID_application.cer -k ~/Library/Keychains/login.keychain
   security import cert/developerID_installer.cer -k ~/Library/Keychains/login.keychain
   ```

4. **Check if certificates exist but lack private keys**:
   ```bash
   security find-certificate -a -c "Developer ID" | grep "labl"
   ```

5. **Unlock keychain if needed**:
   ```bash
   security unlock-keychain ~/Library/Keychains/login.keychain
   ```

### Signing Failures

If signing fails during build:
- The build will fall back to ad-hoc signing (for development)
- Check that your certificates are valid and not expired
- Ensure the certificate names match exactly (including parentheses and spaces)

### Notarization Issues

If notarization fails:
- Check your Apple ID credentials: `xcrun notarytool history --keychain-profile "kolosal-profile"`
- Verify your Team ID is correct
- Ensure you have the necessary entitlements in `resources/kolosal.entitlements`

## Manual Verification

After building, you can verify the signatures:

```bash
# Check app bundle signature
codesign --verify --verbose Kolosal.app

# Check DMG signature
codesign --verify --verbose kolosal-1.0.0-apple-silicon.dmg

# Check notarization
spctl --assess --verbose Kolosal.app
```

## Building Without Code Signing

For development builds, you can disable code signing:

```bash
cmake -DENABLE_CODESIGN=OFF .
make
```

This will use ad-hoc signing, which is sufficient for local development but not for distribution.

## File Locations

- **Certificates**: `cert/`
- **Entitlements**: `resources/kolosal.entitlements`
- **Setup Script**: `cmake/setup_codesign.sh`
- **DMG Creation Script**: `cmake/create_dmg.sh`
- **Built DMG**: `build/kolosal-1.0.0-apple-silicon.dmg`
