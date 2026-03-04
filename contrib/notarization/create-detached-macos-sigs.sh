#!/bin/bash
# Helper script to create detached macOS signatures for Guix builds
set -e

# Check for version parameter
if [ -z "$1" ]; then
    echo "Usage: $0 <version> [cert-path]"
    echo "Example: $0 5.0.5 /path/to/codesign.p12"
    echo ""
    echo "This script creates detached signatures for macOS binaries from Guix builds."
    echo "If cert-path is not provided, it will use the certificate from Keychain."
    exit 1
fi

VERSION=$1
CERT_PATH=$2

# Expected unsigned tarball from Guix build
UNSIGNED_TARBALL="syscoin-${VERSION}-osx-unsigned.tar.gz"

if [ ! -f "$UNSIGNED_TARBALL" ]; then
    echo "Error: Unsigned tarball not found: $UNSIGNED_TARBALL"
    echo ""
    echo "This file should be produced by the Guix build process:"
    echo "  ./contrib/guix/guix-build"
    exit 1
fi

echo "=== Creating detached signatures for Syscoin ${VERSION} macOS binaries ==="

# Extract unsigned binaries
echo "Extracting unsigned binaries..."
rm -rf dist/
tar xf "$UNSIGNED_TARBALL"

# Check if we have a dist directory with the app
if [ ! -d "dist/Syscoin-Qt.app" ]; then
    echo "Error: Expected dist/Syscoin-Qt.app not found after extraction"
    exit 1
fi

# Determine architecture
ARCH=$(file dist/Syscoin-Qt.app/Contents/MacOS/Syscoin-Qt | grep -o 'arm64\|x86_64' | head -1)
echo "Detected architecture: $ARCH"

# Create detached signatures
echo "Creating detached signatures..."
if [ -n "$CERT_PATH" ]; then
    # Use provided certificate file
    if [ ! -f "$CERT_PATH" ]; then
        echo "Error: Certificate file not found: $CERT_PATH"
        exit 1
    fi
    ./contrib/macdeploy/detached-sig-create.sh "$CERT_PATH"
else
    # Use certificate from Keychain
    echo "Using Developer ID certificate from Keychain..."
    # Create a temporary script that uses the Keychain certificate
    cat > temp-sign.sh << 'EOF'
#!/bin/bash
set -e
SIGNAPPLE=signapple
TEMPDIR=sign.temp
BUNDLE="dist/Syscoin-Qt.app"
ARCH=$(${SIGNAPPLE} info ${BUNDLE}/Contents/MacOS/Syscoin-Qt | head -n 1 | cut -d " " -f 1)
OUT="signature-osx-${ARCH}.tar.gz"
OUTROOT=osx/dist

rm -rf ${TEMPDIR}
mkdir -p ${TEMPDIR}

# Sign using Keychain identity
${SIGNAPPLE} sign --hardened-runtime -f --detach "${TEMPDIR}/${OUTROOT}" \
    --identity "Developer ID Application: Syscoin Stichting" \
    "${BUNDLE}"

tar -C "${TEMPDIR}" -czf "${OUT}" .
rm -rf "${TEMPDIR}"
echo "Created ${OUT}"
EOF
    chmod +x temp-sign.sh
    ./temp-sign.sh
    rm -f temp-sign.sh
fi

# Verify output
OUTPUT_FILE="signature-osx-${ARCH}.tar.gz"
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Expected output file not created: $OUTPUT_FILE"
    exit 1
fi

echo ""
echo "=== Detached signatures created successfully! ==="
echo "Output file: $OUTPUT_FILE"
echo ""
echo "Next steps:"
echo "1. Test the signatures by running guix-codesign (optional but recommended):"
echo "   export DETACHED_SIGS_REPO=/path/to/test-repo"
echo "   tar xf $OUTPUT_FILE -C \$DETACHED_SIGS_REPO"
echo "   ./contrib/guix/guix-codesign"
echo ""
echo "2. Submit signatures to syscoin-detached-sigs repository:"
echo "   cd /path/to/syscoin-detached-sigs"
echo "   git checkout -b v${VERSION}"
echo "   tar xf /path/to/$OUTPUT_FILE"
echo "   git add -A && git commit -m 'macOS signatures for v${VERSION}'"
echo "   git tag -s v${VERSION}"
echo ""
echo "3. After signatures are merged and tagged, builders can create signed binaries"
echo "4. Notarize the signed binaries using ./contrib/notarization/notarize-all.sh"

# Clean up
rm -rf dist/ 