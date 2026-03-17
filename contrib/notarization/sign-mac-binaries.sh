#!/bin/bash
# Script to sign Syscoin macOS binaries
set -e

# Check for version parameter
if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 5.1.0"
    exit 1
fi

VERSION=$1

# Certificate identity
CERT_ID="Developer ID Application: Syscoin Stichting (ZF84YCPK58)"

# Check if unsigned zip files exist
if [ ! -f "syscoin-${VERSION}-arm64-apple-darwin-unsigned.zip" ] || [ ! -f "syscoin-${VERSION}-x86_64-apple-darwin-unsigned.zip" ]; then
    echo "Error: Unsigned zip files not found!"
    echo "Expected files:"
    echo "  - syscoin-${VERSION}-arm64-apple-darwin-unsigned.zip"
    echo "  - syscoin-${VERSION}-x86_64-apple-darwin-unsigned.zip"
    exit 1
fi

# Check if certificate exists
if ! security find-identity -v -p codesigning | grep -q "$CERT_ID"; then
    echo "Error: Certificate not found: $CERT_ID"
    echo "Please install the Developer ID certificate in your Keychain."
    exit 1
fi

echo "=== Signing Syscoin macOS binaries v${VERSION} ==="

# Create entitlements file if it doesn't exist
if [ ! -f "entitlements.plist" ]; then
    cat > entitlements.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <true/>
    <key>com.apple.security.cs.disable-library-validation</key>
    <true/>
</dict>
</plist>
EOF
fi

# Create working directories
mkdir -p signed_tmp

# Process ARM64 binary
echo "Processing ARM64 binary..."
rm -rf signed_tmp/arm64
unzip -q "syscoin-${VERSION}-arm64-apple-darwin-unsigned.zip" -d signed_tmp/arm64/
echo "Signing ARM64 binary..."
codesign --force --deep --sign "$CERT_ID" \
    --options runtime \
    --timestamp \
    --entitlements entitlements.plist \
    signed_tmp/arm64/Syscoin-Qt.app

echo "Verifying ARM64 signature..."
codesign --verify --deep --strict --verbose=2 signed_tmp/arm64/Syscoin-Qt.app

# Process x86_64 binary
echo -e "\nProcessing x86_64 binary..."
rm -rf signed_tmp/x86_64
unzip -q "syscoin-${VERSION}-x86_64-apple-darwin-unsigned.zip" -d signed_tmp/x86_64/
echo "Signing x86_64 binary..."
codesign --force --deep --sign "$CERT_ID" \
    --options runtime \
    --timestamp \
    --entitlements entitlements.plist \
    signed_tmp/x86_64/Syscoin-Qt.app

echo "Verifying x86_64 signature..."
codesign --verify --deep --strict --verbose=2 signed_tmp/x86_64/Syscoin-Qt.app

# Create signed zip files
echo -e "\nCreating signed packages..."
cd signed_tmp/arm64
zip -r -q "../../syscoin-${VERSION}-arm64-apple-darwin-signed.zip" Syscoin-Qt.app
cd ../x86_64
zip -r -q "../../syscoin-${VERSION}-x86_64-apple-darwin-signed.zip" Syscoin-Qt.app
cd ../..

# Clean up
rm -rf signed_tmp
rm -f entitlements.plist

echo -e "\n=== Signing complete! ==="
echo "Created:"
echo "  - syscoin-${VERSION}-arm64-apple-darwin-signed.zip"
echo "  - syscoin-${VERSION}-x86_64-apple-darwin-signed.zip"
echo -e "\nNext step: Run ./notarize-mac-binaries.sh ${VERSION}" 