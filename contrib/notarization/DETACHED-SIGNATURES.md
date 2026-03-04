# Detached Signatures Process

This document explains how detached signatures work in the Syscoin build process, following the Bitcoin Core model.

## Overview

The detached signatures process allows for:
1. **Deterministic builds**: Multiple builders create identical unsigned binaries
2. **Separate signing**: Code signers create detached signatures without the full build environment
3. **Verification**: Anyone can verify that signed binaries match the deterministic builds
4. **Trust distribution**: No single party controls the entire build and signing process

## Process Flow

```
1. Developers → Push code → GitHub
                    ↓
2. Multiple builders → Guix build → Unsigned binaries
                    ↓
3. Builders → Verify identical outputs → Attestations
                    ↓
4. Code signers → Create detached signatures → syscoin-detached-sigs repo
                    ↓
5. Builders → Apply signatures → Signed binaries
                    ↓
6. Verification → Match attestations → Release
```

## Detailed Steps

### 1. Deterministic Build (Guix)

Multiple builders independently build unsigned binaries:

```bash
# Each builder runs:
./contrib/guix/guix-build
```

This produces unsigned binaries:
- `syscoin-${VERSION}-osx-unsigned.tar.gz`
- `syscoin-${VERSION}-win-unsigned.tar.gz`
- Other platform binaries...

### 2. Create Attestations

Builders sign their build outputs:

```bash
./contrib/guix/guix-attest
```

This creates `noncodesigned.SHA256SUMS` with builder's signature.

### 3. Create Detached Signatures

#### macOS Signatures

The macOS code signer (with access to the Developer ID certificate):

```bash
# Extract unsigned macOS binaries
tar xf syscoin-${VERSION}-osx-unsigned.tar.gz

# Create detached signatures using signapple
./contrib/macdeploy/detached-sig-create.sh /path/to/codesign.p12

# This creates: signature-osx-${ARCH}.tar.gz
```

#### Windows Signatures

The Windows code signer:

```bash
# Extract unsigned Windows binaries
tar xf syscoin-${VERSION}-win-unsigned.tar.gz

# Create detached signatures
./contrib/windeploy/detached-sig-create.sh -key /path/to/codesign.key

# This creates: signature-win.tar.gz
```

### 4. Commit Detached Signatures

Code signers commit signatures to the detached sigs repository:

```bash
# In syscoin-detached-sigs repository
git checkout -b ${VERSION}
rm -rf ./*
tar xf /path/to/signature-osx-${ARCH}.tar.gz
tar xf /path/to/signature-win.tar.gz
git add -A
git commit -m "Add ${VERSION} detached signatures"
git tag -s "v${VERSION}" HEAD
git push origin ${VERSION} --tags
```

### 5. Apply Detached Signatures

All builders can now create signed binaries:

```bash
# Point to detached sigs repo
export DETACHED_SIGS_REPO=/path/to/syscoin-detached-sigs

# Create signed binaries
./contrib/guix/guix-codesign
```

This produces signed binaries:
- `syscoin-${VERSION}-osx-signed.tar.gz`
- `syscoin-${VERSION}-win-signed.tar.gz`

### 6. Create Final Attestations

Builders attest to the signed binaries:

```bash
./contrib/guix/guix-attest
```

This creates `all.SHA256SUMS` containing hashes of both signed and unsigned binaries.

## Repository Structure

### syscoin-detached-sigs Repository

```
syscoin-detached-sigs/
├── osx/
│   └── dist/
│       └── Syscoin-Qt.app/
│           └── ... (detached signature files)
├── win/
│   ├── syscoin-cli.exe.pem
│   ├── syscoin-qt.exe.pem
│   └── ... (other .pem signature files)
└── README.md
```

## Integration with Direct Signing

The detached signature process is **separate** from the direct signing process documented in the [README.md](README.md). Here's how they differ:

| Aspect | Direct Signing | Detached Signatures |
|--------|----------------|---------------------|
| When to use | Local/development builds | Official releases |
| Build system | Any | Guix (deterministic) |
| Signature storage | In the binary | Separate repository |
| Verification | Single signer | Multiple attestations |
| Notarization | Can be done immediately | Done after codesigning |

## Using Both Processes

For official releases:
1. Use Guix to create unsigned deterministic builds
2. Create detached signatures (this document)
3. Apply signatures with guix-codesign
4. Notarize the signed binaries (see [README.md](README.md))

For development/testing:
1. Use direct signing (see [README.md](README.md))
2. Notarize immediately

## Security Benefits

1. **No single point of failure**: No one person has both build environment and signing keys
2. **Verifiable builds**: Anyone can verify binaries match source code
3. **Audit trail**: All signatures are committed to version control
4. **Reproducibility**: Identical binaries can be rebuilt anytime

## Technical Details

### macOS Detached Signatures

Uses `signapple` to create signatures without embedding them:

```bash
signapple sign --hardened-runtime -f --detach <output-dir> <signing-args> <app-bundle>
```

The `--detach` flag creates signature files instead of embedding them.

### Windows Detached Signatures

Uses `osslsigncode` to extract signatures as PEM files:

```bash
osslsigncode sign -certs <cert> -key <key> -h sha256 -in <unsigned.exe> -out <signed.exe>
osslsigncode extract-signature -pem -in <signed.exe> -out <signature.pem>
```

### Applying Signatures

The `guix-codesign` script:
1. Reads detached signatures from the repository
2. Applies them to unsigned binaries
3. Creates bit-for-bit identical signed binaries

## Troubleshooting

### Signature Mismatch

If signed binaries don't match attestations:
- Ensure using the exact same unsigned binaries
- Verify detached-sigs repo is at correct tag
- Check that no local modifications exist

### Missing Signatures

If signatures are missing:
- Verify the detached-sigs repo is up to date
- Ensure the version tag exists
- Check that all required signers have contributed

## Further Reading

- [Guix Build Documentation](../guix/README.md)
- [Release Process](../../doc/release-process.md)
- [Bitcoin Core Deterministic Builds](https://github.com/bitcoin/bitcoin/blob/master/doc/release-process.md) 