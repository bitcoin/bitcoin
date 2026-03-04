# macOS Code Signing and Notarization Guide

This guide explains how to sign and notarize Syscoin QT binaries for macOS distribution.

## Prerequisites

1. **Apple Developer Account** with active membership ($99/year)
2. **Developer ID Application certificate** installed in Keychain
3. **App-specific password** for notarization (create at https://appleid.apple.com)
4. **Xcode Command Line Tools** installed
5. **Unsigned binaries**: 
   - `syscoin-X.X.X-arm64-apple-darwin-unsigned.zip`
   - `syscoin-X.X.X-x86_64-apple-darwin-unsigned.zip`

## Quick Start

For Syscoin 5.0.5 release:

### Option 1: Run all steps automatically
```bash
# Set up credentials first (one-time)
export APPLE_ID="your-email@example.com"
xcrun notarytool store-credentials "syscoin-notary" \
    --apple-id "$APPLE_ID" \
    --team-id "ZF84YCPK58"

# Run all steps
./notarize-all.sh 5.0.5
```

### Option 2: Run steps individually
```bash
# 1. Sign the binaries
./sign-mac-binaries.sh 5.0.5

# 2. Set up Apple ID credentials (one-time)
export APPLE_ID="your-email@example.com"
xcrun notarytool store-credentials "syscoin-notary" \
    --apple-id "$APPLE_ID" \
    --team-id "ZF84YCPK58"

# 3. Notarize the signed binaries
./notarize-mac-binaries.sh 5.0.5

# 4. Staple the notarization tickets
./staple-notarization.sh 5.0.5
```

## Detailed Steps

### 1. Check Your Certificate

```bash
# List all code signing certificates
security find-identity -v -p codesigning

# Expected output should include:
# "Developer ID Application: Syscoin Stichting (ZF84YCPK58)"
```

### 2. Sign the Binaries

The `sign-mac-binaries.sh` script will:
- Extract the unsigned apps
- Apply code signature with hardened runtime
- Add required entitlements for QT apps
- Create signed zip packages

```bash
./sign-mac-binaries.sh <version>
# Example: ./sign-mac-binaries.sh 5.0.5
```

This creates:
- `syscoin-X.X.X-arm64-apple-darwin-signed.zip`
- `syscoin-X.X.X-x86_64-apple-darwin-signed.zip`

### 3. Set Up Notarization Credentials

You only need to do this once:

```bash
# Set your Apple ID
export APPLE_ID="your-apple-id@example.com"

# Store credentials in Keychain
xcrun notarytool store-credentials "syscoin-notary" \
    --apple-id "$APPLE_ID" \
    --team-id "ZF84YCPK58"
```

When prompted, enter your **app-specific password** (not your Apple ID password).

To create an app-specific password:
1. Go to https://appleid.apple.com
2. Sign in → Sign-In and Security → App-Specific Passwords
3. Click "+" to generate a new password
4. Name it "Syscoin Notarization"

### 4. Notarize the Binaries

```bash
./notarize-mac-binaries.sh <version>
# Example: ./notarize-mac-binaries.sh 5.0.5
```

This will:
- Submit both binaries to Apple's notary service
- Wait for processing (typically 5-15 minutes)
- Report success/failure status

### 5. Staple Notarization Tickets

```bash
./staple-notarization.sh <version>
# Example: ./staple-notarization.sh 5.0.5
```

This creates the final distribution packages:
- `syscoin-X.X.X-arm64-apple-darwin-notarized.zip`
- `syscoin-X.X.X-x86_64-apple-darwin-notarized.zip`

## Troubleshooting

### Certificate Issues

If you get "Developer ID Application: Syscoin Stichting" not found:
1. Download the certificate from Apple Developer portal
2. If you have a .p12 file: `security import certificate.p12 -k ~/Library/Keychains/login.keychain-db`
3. If you only have a .cer file, you need the private key from the Mac that created the CSR

### Notarization Errors

**Error 403 - Agreement Required**:
- Log into https://developer.apple.com/account
- Accept any pending agreements
- Only Account Holder/Admin can accept agreements

**Error 401 - Invalid Credentials**:
- Ensure you're using an app-specific password, not your Apple ID password
- Regenerate the app-specific password if needed

### Verification

To verify a notarized app:
```bash
# Check notarization status
xcrun stapler validate /path/to/Syscoin-Qt.app

# Check code signature
codesign --verify --deep --strict --verbose=2 /path/to/Syscoin-Qt.app
```

## Certificate Renewal

Current certificate expires: February 1, 2027

To renew:
1. Create a new certificate request (CSR) before expiration
2. Submit to Apple Developer portal
3. Download and install new certificate
4. Update scripts if Team ID changes

## Manual Process (Without Scripts)

If scripts fail, you can sign manually:

```bash
# Sign an app
codesign --force --deep --sign "Developer ID Application: Syscoin Stichting (ZF84YCPK58)" \
    --options runtime --timestamp \
    --entitlements entitlements.plist \
    Syscoin-Qt.app

# Submit for notarization
xcrun notarytool submit signed-app.zip \
    --keychain-profile "syscoin-notary" \
    --wait

# Staple ticket
xcrun stapler staple Syscoin-Qt.app
```

## Notes

- Keep the signed (non-notarized) versions as backup
- Notarization requires internet connection
- Stapled apps work offline
- Re-sign if you modify the app in any way
- Universal binaries can be created with `lipo` tool if needed

## Related Documentation

- [Detached Signatures Process](DETACHED-SIGNATURES.md) - For official releases using Guix builds
- [Release Process](../../doc/release-process.md) - Complete release workflow
- [Guix Build System](../guix/README.md) - Deterministic build documentation