#!/bin/bash
# Script to notarize signed Syscoin macOS binaries
set -e

# Check for version parameter
if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 5.1.0"
    exit 1
fi

VERSION=$1

# Check if signed zip files exist
if [ ! -f "syscoin-${VERSION}-arm64-apple-darwin-signed.zip" ] || [ ! -f "syscoin-${VERSION}-x86_64-apple-darwin-signed.zip" ]; then
    echo "Error: Signed zip files not found!"
    echo "Expected files:"
    echo "  - syscoin-${VERSION}-arm64-apple-darwin-signed.zip"
    echo "  - syscoin-${VERSION}-x86_64-apple-darwin-signed.zip"
    echo ""
    echo "Run ./sign-mac-binaries.sh ${VERSION} first."
    exit 1
fi

# Check for Apple ID credentials
if [ -z "$APPLE_ID" ]; then
    echo "Please set APPLE_ID environment variable (your Apple Developer account email)"
    echo "Example: export APPLE_ID='your-email@example.com'"
    exit 1
fi

if [ -z "$APPLE_TEAM_ID" ]; then
    APPLE_TEAM_ID="ZF84YCPK58"
    echo "Using team ID: $APPLE_TEAM_ID"
fi

# Check if credentials are stored
if ! xcrun notarytool history --keychain-profile "syscoin-notary" 2>&1 | grep -q "id:"; then
    echo "Notarization credentials not found."
    echo ""
    echo "Please store your credentials first:"
    echo "xcrun notarytool store-credentials \"syscoin-notary\" --apple-id \"$APPLE_ID\" --team-id \"$APPLE_TEAM_ID\""
    echo ""
    echo "You'll need an app-specific password from https://appleid.apple.com"
    exit 1
fi

echo "=== Notarizing Syscoin macOS binaries v${VERSION} ==="
echo "Apple ID: $APPLE_ID"
echo "Team ID: $APPLE_TEAM_ID"

# Function to notarize a file
notarize_file() {
    local FILE=$1
    local ARCH=$2
    
    echo -e "\nNotarizing $ARCH binary..."
    echo "Submitting $FILE for notarization..."
    
    # Submit for notarization
    if ! xcrun notarytool submit "$FILE" \
        --keychain-profile "syscoin-notary" \
        --wait; then
        echo "Error: Notarization failed for $ARCH binary"
        echo ""
        echo "Common issues:"
        echo "1. Check Apple Developer portal for any pending agreements"
        echo "2. Verify your app-specific password is correct"
        echo "3. Ensure your Developer account is active"
        exit 1
    fi
    
    echo "$ARCH binary notarization complete!"
}

# Notarize both files
notarize_file "syscoin-${VERSION}-arm64-apple-darwin-signed.zip" "ARM64"
notarize_file "syscoin-${VERSION}-x86_64-apple-darwin-signed.zip" "x86_64"

echo -e "\n=== Notarization complete! ==="
echo "The signed binaries have been notarized by Apple."
echo ""
echo "Next step: Run ./staple-notarization.sh ${VERSION}"
echo ""
echo "To check notarization status later:"
echo "xcrun notarytool history --keychain-profile \"syscoin-notary\"" 