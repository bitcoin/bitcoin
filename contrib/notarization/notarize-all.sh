#!/bin/bash
# Helper script to run all notarization steps
set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 5.1.0"
    echo ""
    echo "This script will run all notarization steps:"
    echo "1. Sign the binaries"
    echo "2. Notarize with Apple"
    echo "3. Staple the tickets"
    echo ""
    echo "Prerequisites:"
    echo "- Unsigned binaries in current directory"
    echo "- Valid Developer ID certificate in Keychain"
    echo "- Apple ID credentials stored (or set APPLE_ID env var)"
    exit 1
fi

VERSION=$1

echo "=== Full notarization process for Syscoin v${VERSION} ==="
echo ""

# Step 1: Sign
echo "Step 1: Signing binaries..."
./sign-mac-binaries.sh "$VERSION"

# Step 2: Check credentials
if [ -z "$APPLE_ID" ]; then
    echo ""
    echo "APPLE_ID environment variable not set."
    echo "Please set it before continuing:"
    echo "export APPLE_ID='your-email@example.com'"
    echo ""
    echo "Then run: ./notarize-mac-binaries.sh ${VERSION}"
    echo "Followed by: ./staple-notarization.sh ${VERSION}"
    exit 0
fi

# Step 3: Notarize
echo ""
echo "Step 2: Notarizing with Apple..."
./notarize-mac-binaries.sh "$VERSION"

# Step 4: Staple
echo ""
echo "Step 3: Stapling notarization tickets..."
./staple-notarization.sh "$VERSION"

echo ""
echo "=== All done! ==="
echo "Your notarized binaries are ready for distribution." 