# Code Signing Guide for macOS Package

This guide explains how to add code signing and notarization to your macOS `.pkg` installer to avoid Gatekeeper warnings and build user trust.

## � Quick Start

```bash
# 1. Get BOTH certificates from Apple Developer Portal
#    - Developer ID Application (for binaries)
#    - Developer ID Installer (for .pkg)

# 2. Set environment variables
export CODESIGN_IDENTITY_APP="Developer ID Application: Rifky Bujana Bisri (SNW8GV8C24)"
export CODESIGN_IDENTITY="Developer ID Installer: Rifky Bujana Bisri (SNW8GV8C24)"

# 3. Build with automatic signing
npm run build:mac:pkg

# 4. (Optional) Notarize
export NOTARIZE=1
npm run build:mac:pkg
```

## ⚠️ Common Issue: "Invalid" Notarization

**Problem:** Notarization fails with "The binary is not signed with a valid Developer ID certificate"

**Solution:** You need BOTH certificates:
- ❌ Only having "Developer ID Installer" → Notarization fails
- ✅ Having both "Application" + "Installer" → Notarization succeeds

See [Getting Certificates](#getting-certificates) below.

---

## �📋 Table of Contents

1. [Prerequisites](#prerequisites)
2. [Getting Certificates](#getting-certificates)
3. [Finding Your Certificate Identity](#finding-your-certificate-identity)
4. [Signing the Package](#signing-the-package)
5. [Notarization (Optional but Recommended)](#notarization)
6. [Automation](#automation)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required

- **Apple Developer Account**: 
  - Individual: $99/year
  - Organization: $99/year
  - [Sign up here](https://developer.apple.com/programs/)

- **macOS**: Must build and sign on a Mac

- **Xcode Command Line Tools**:
  ```bash
  xcode-select --install
  ```

### Optional (for Notarization)

- **App-specific password** for notarization
- **Xcode** (for easier certificate management)

---

## Getting Certificates

### ⚠️ IMPORTANT: Two Certificates Required for Notarization

To successfully notarize your package, you need **TWO** different certificates:

1. **Developer ID Application** - Signs native binaries (`.node` files, executables)
2. **Developer ID Installer** - Signs the `.pkg` installer itself

Both are required! Without the Application certificate, notarization will fail.

### Option 1: Via Xcode (Easiest)

1. Open **Xcode** → **Settings/Preferences**
2. Go to **Accounts** tab
3. Click **+** to add your Apple ID
4. Select your account → Click **Manage Certificates**
5. Click **+** → Select **Developer ID Application** (for binaries)
6. Click **+** → Select **Developer ID Installer** (for .pkg)
7. Both certificates will be automatically created and installed

### Option 2: Via Developer Portal

1. Go to [Apple Developer Portal](https://developer.apple.com/account)
2. Navigate to **Certificates, Identifiers & Profiles**
3. Click **+** to create new certificate
4. Select **Developer ID Application** → Create and download
5. Click **+** again to create another certificate
6. Select **Developer ID Installer** → Create and download
7. Double-click both .cer files to install them

### Option 3: Via Command Line

```bash
# Create a CSR
security create-keypair -a RSA -s 2048 \
  -f ~/Desktop/CertificateSigningRequest.certSigningRequest \
  -p ~/Desktop/private.key \
  -d "Your Name"

# Then upload the CSR to Apple Developer Portal
```

---

## Finding Your Certificate Identity

Once you have **both** certificates installed, find their identities:

```bash
# List all available signing identities
security find-identity -v -p basic

# You should see BOTH:
#   1) ABC123... "Developer ID Application: Your Name (TEAM_ID)"
#   2) XYZ789... "Developer ID Installer: Your Name (TEAM_ID)"
```

You'll need **both** identity strings:
- **Application**: `"Developer ID Application: Your Name (TEAM_ID)"`
- **Installer**: `"Developer ID Installer: Your Name (TEAM_ID)"`

---

## Signing the Package

### Method 1: Sign After Build (Manual)

After running `npm run build:mac:pkg`, sign everything manually:

```bash
# Step 1: Sign all native binaries (.node files and executables)
find dist/mac/kolosal-app -type f \( -name "*.node" -o -name "spawn-helper" \) \
  -exec codesign --sign "Developer ID Application: Your Name (TEAM_ID)" \
  --force --timestamp --options runtime {} \;

# Step 2: Rebuild the package (since we modified the files)
npm run build:mac:pkg

# Step 3: Sign the .pkg installer
productsign --sign "Developer ID Installer: Your Name (TEAM_ID)" \
  dist/mac/KolosalCode-macos.pkg \
  dist/mac/KolosalCode-macos-signed.pkg

# Step 4: Verify the signature
pkgutil --check-signature dist/mac/KolosalCode-macos-signed.pkg
spctl --assess --type install dist/mac/KolosalCode-macos-signed.pkg
```

### Method 2: Automated Signing (Recommended)

The build script automatically signs binaries and the package if you set the environment variables:

```bash
# Set BOTH signing identities
export CODESIGN_IDENTITY_APP="Developer ID Application: Your Name (TEAM_ID)"
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"

# Build with automatic signing
npm run build:mac:pkg
```

The script will:
1. ✅ Build the standalone app
2. ✅ Sign all native binaries with Application certificate
3. ✅ Create the .pkg installer  
4. ✅ Sign the .pkg with Installer certificate
    
    console.log('✅ Package signed successfully');
    
    // Verify signature
    const { stdout } = await execAsync(
      `pkgutil --check-signature "${signedPkgPath}"`
    );
    console.log('\n📋 Signature verification:');
    console.log(stdout);
    
    // Remove unsigned package
    await fs.unlink(pkgPath);
    
    return signedPkgPath;
  } catch (error) {
    console.error(`❌ Signing failed: ${error.message}`);
    throw error;
  }
}
```

Then update the `main()` function:

```javascript
async function main() {
  try {
    const appDir = await buildStandalone();
    let pkgPath = await createPkg(appDir);
    
    // Sign the package
    pkgPath = await signPkg(pkgPath);
    
    console.log('\n✨ Build complete!');
    console.log(`\n📦 Package: ${pkgPath}`);
    // ... rest of output
  } catch (error) {
    console.error(`\n❌ Build failed: ${error.message}`);
    process.exit(1);
  }
}
```

### Usage

```bash
# Set your signing identity
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"

# Build and sign
npm run build:mac:pkg
```

### Environment Variable Setup

Add to your `.zshrc` or `.bashrc`:

```bash
# Required for notarization - BOTH certificates needed
export CODESIGN_IDENTITY_APP="Developer ID Application: Your Name (TEAM_ID)"
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"
```

For CI/CD, set both as secret environment variables.

---

## Notarization

Notarization is Apple's process of scanning your app for malware. It's **highly recommended** for distribution.

### ⚠️ Prerequisites for Notarization

**ALL of these are required:**

1. ✅ **Both certificates** (Application + Installer)
2. ✅ **All binaries signed** with Developer ID Application + hardened runtime + timestamp
3. ✅ **.pkg signed** with Developer ID Installer
4. ✅ **App-specific password** for notarytool

### Setup Notarization

1. **App-specific password**:
   ```bash
   # Create at: https://appleid.apple.com
   # Go to Security → App-Specific Passwords
   ```

2. **Store credentials**:
   ```bash
   xcrun notarytool store-credentials "notarytool-profile" \
     --apple-id "your-email@example.com" \
     --team-id "YOUR_TEAM_ID" \
     --password "xxxx-xxxx-xxxx-xxxx"
   ```

### Notarization Steps

#### 1. Submit for Notarization

```bash
xcrun notarytool submit \
  dist/mac/KolosalCode-macos-signed.pkg \
  --keychain-profile "notarytool-profile" \
  --wait
```

This will:
- Upload your package to Apple
- Wait for notarization to complete (usually 1-5 minutes)
- Show the result

#### 2. Check Status (if needed)

```bash
# Get submission ID from previous command, then:
xcrun notarytool info <submission-id> \
  --keychain-profile "notarytool-profile"

# Get detailed log
xcrun notarytool log <submission-id> \
  --keychain-profile "notarytool-profile"
```

#### 3. Staple the Notarization

After successful notarization, staple the ticket to the package:

```bash
xcrun stapler staple dist/mac/KolosalCode-macos-signed.pkg

# Verify stapling
xcrun stapler validate dist/mac/KolosalCode-macos-signed.pkg
spctl --assess -vv --type install dist/mac/KolosalCode-macos-signed.pkg
```

### Automated Notarization Function

Add to `build-standalone-pkg.js`:

```javascript
async function notarizePkg(pkgPath) {
  const profile = process.env.NOTARIZE_PROFILE || 'notarytool-profile';
  
  if (!process.env.NOTARIZE) {
    console.log('\n⚠️  Skipping notarization (set NOTARIZE=1 to enable)');
    return pkgPath;
  }
  
  console.log('\n🔔 Submitting for notarization...');
  console.log('   This may take 1-5 minutes...');
  
  try {
    // Submit for notarization
    const { stdout: submitOutput } = await execAsync(
      `xcrun notarytool submit "${pkgPath}" ` +
      `--keychain-profile "${profile}" --wait`
    );
    console.log(submitOutput);
    
    // Staple the notarization ticket
    console.log('\n📎 Stapling notarization ticket...');
    await execAsync(`xcrun stapler staple "${pkgPath}"`);
    
    // Verify
    const { stdout: verifyOutput } = await execAsync(
      `xcrun stapler validate "${pkgPath}"`
    );
    console.log('✅ Notarization complete!');
    console.log(verifyOutput);
    
    return pkgPath;
  } catch (error) {
    console.error(`❌ Notarization failed: ${error.message}`);
    throw error;
  }
}
```

Update `main()`:

```javascript
async function main() {
  try {
    const appDir = await buildStandalone();
    let pkgPath = await createPkg(appDir);
    pkgPath = await signPkg(pkgPath);
    pkgPath = await notarizePkg(pkgPath); // Add this line
    
    // ... rest of code
  }
}
```

---

## Automation

### Full Build Script with Signing & Notarization

```bash
#!/bin/bash
# build-and-sign.sh

set -e

echo "🔨 Building package..."
npm run build:mac:pkg

echo "🔐 Setting up signing..."
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"

echo "🔔 Enabling notarization..."
export NOTARIZE=1
export NOTARIZE_PROFILE="notarytool-profile"

echo "✨ Build, sign, and notarize complete!"
```

### CI/CD (GitHub Actions Example)

```yaml
name: Build and Sign macOS Package

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '20'
      
      - name: Install dependencies
        run: npm ci
      
      - name: Import certificates
        env:
          CERTIFICATE_BASE64: ${{ secrets.CERTIFICATE_BASE64 }}
          CERTIFICATE_PASSWORD: ${{ secrets.CERTIFICATE_PASSWORD }}
        run: |
          # Create keychain
          security create-keychain -p actions build.keychain
          security default-keychain -s build.keychain
          security unlock-keychain -p actions build.keychain
          
          # Import certificate
          echo $CERTIFICATE_BASE64 | base64 --decode > certificate.p12
          security import certificate.p12 -k build.keychain \
            -P $CERTIFICATE_PASSWORD -T /usr/bin/codesign \
            -T /usr/bin/productsign
          
          # Allow codesign to access keychain
          security set-key-partition-list -S apple-tool:,apple: \
            -s -k actions build.keychain
      
      - name: Build and Sign
        env:
          CODESIGN_IDENTITY: ${{ secrets.CODESIGN_IDENTITY }}
          NOTARIZE: "1"
          APPLE_ID: ${{ secrets.APPLE_ID }}
          TEAM_ID: ${{ secrets.TEAM_ID }}
          APP_PASSWORD: ${{ secrets.APP_PASSWORD }}
        run: |
          # Store notarization credentials
          xcrun notarytool store-credentials "ci-profile" \
            --apple-id "$APPLE_ID" \
            --team-id "$TEAM_ID" \
            --password "$APP_PASSWORD"
          
          export NOTARIZE_PROFILE="ci-profile"
          npm run build:mac:pkg
      
      - name: Upload artifact
        uses: actions/upload-artifact@v3
        with:
          name: macos-package
          path: dist/mac/KolosalCode-macos-signed.pkg
```

---

## Troubleshooting

### "Developer ID Installer certificate not found"

```bash
# List all certificates
security find-identity -v

# If certificate exists but not found:
security list-keychains  # Check which keychains are available
security default-keychain  # Check default keychain

# Add login keychain if needed
security list-keychains -s ~/Library/Keychains/login.keychain-db
```

### "productsign failed with error"

```bash
# Check certificate validity
security find-certificate -c "Developer ID Installer" -p | openssl x509 -text

# Check for expired certificates
security find-identity -v -p codesigning
```

### "Notarization failed: Invalid"

Common reasons:
- Package not signed
- Signing identity doesn't match Apple Developer account
- Hardened runtime not enabled (usually not needed for .pkg)

Get detailed logs:
```bash
xcrun notarytool log <submission-id> --keychain-profile "notarytool-profile"
```

### "Signature invalid or not verifiable"

```bash
# Re-sign the package
productsign --sign "Developer ID Installer: Your Name (TEAM_ID)" \
  dist/mac/KolosalCode-macos.pkg \
  dist/mac/KolosalCode-macos-signed.pkg

# Verify again
spctl --assess --verbose --type install dist/mac/KolosalCode-macos-signed.pkg
```

---

## Quick Reference

### Essential Commands

```bash
# Find signing identity
security find-identity -v -p basic

# Sign package
productsign --sign "IDENTITY" input.pkg output.pkg

# Verify signature
pkgutil --check-signature package.pkg
spctl --assess --type install package.pkg

# Submit for notarization
xcrun notarytool submit package.pkg \
  --keychain-profile "profile-name" --wait

# Staple notarization
xcrun stapler staple package.pkg

# Verify stapling
xcrun stapler validate package.pkg
```

### Environment Variables

```bash
# Required for signing
export CODESIGN_IDENTITY="Developer ID Installer: Your Name (TEAM_ID)"

# Optional for notarization
export NOTARIZE=1
export NOTARIZE_PROFILE="notarytool-profile"
```

---

## Next Steps

1. **Get a Developer ID certificate** (see [Getting Certificates](#getting-certificates))
2. **Test signing locally** (see [Method 1](#method-1-sign-after-build-manual))
3. **Update build script** (see [Method 2](#method-2-automated-signing-recommended))
4. **Set up notarization** (see [Notarization](#notarization))
5. **Automate in CI/CD** (see [CI/CD Example](#cicd-github-actions-example))

---

## Additional Resources

- [Apple Code Signing Guide](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/)
- [Notarization Documentation](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
- [productsign man page](https://ss64.com/osx/productsign.html)
- [notarytool documentation](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow)
