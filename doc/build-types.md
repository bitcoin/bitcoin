# Bitcoin Core Build Types

This document explains the different types of Bitcoin Core binaries available for download from bitcoincore.org and via torrents.

## Overview

Bitcoin Core releases include several different build types to meet various user needs and security requirements. Each build type serves a specific purpose in the release and verification process.

## Build Types

### Unsigned Builds

**Files**: `*-unsigned.zip` (Windows), `*-unsigned.tar.gz`, `*-unsigned.dmg` (macOS)

Unsigned builds are the raw binary outputs from the reproducible build process before any code signatures have been applied. These builds:

- Are produced directly from the Guix deterministic build system
- Do not contain platform-specific code signatures
- Cannot be run on some systems without additional steps (particularly macOS with Gatekeeper)
- Are primarily used by developers and the codesigning process
- Allow verification of the reproducible build process before signatures are applied

**When to use**: If you want to verify the reproducible build process yourself, or if you're a developer participating in the release process.

### Signed/Codesigned Builds

**Files**: Regular `.tar.gz`, `.zip`, `.exe`, `.dmg` files without "unsigned" in the name

Signed builds are the standard release binaries that most users should download. These builds:

- Include platform-specific code signatures (Authenticode for Windows, Apple codesigning for macOS)
- Can be run without security warnings on Windows and macOS
- Are created by applying detached signatures to the unsigned builds
- Maintain the same hash for the actual binary content (only the signature differs)

**When to use**: This is the recommended version for most users who want to run Bitcoin Core on Windows or macOS.

### Debug Builds

**Files**: `*-debug.zip`, `*-debug.tar.gz`

Debug builds contain additional debugging information that helps developers diagnose issues. These builds:

- Include debug symbols (`.pdb` files on Windows, `.dbg` files on Linux)
- Are significantly larger than regular builds
- Have the same functionality as regular builds but with debugging information
- Are useful for crash analysis and debugging
- May have slightly different performance characteristics

**When to use**: Only if you're a developer debugging Bitcoin Core, or if you're asked to provide debug information for a crash report.

### Codesigning Archives

**Files**: `*-codesigning.tar.gz`

Codesigning archives are special packages used in the release process. These archives:

- Contain the structure needed for applying platform signatures
- Are used by designated codesigners who have access to signing certificates
- Are not intended for end users
- Are part of the deterministic build and signing workflow

**When to use**: Only if you're an official Bitcoin Core codesigner participating in the release process.

## File Formats by Platform

### Windows
- `.zip` - Compressed archive containing the executable files
- `*-setup.exe` - Installer for Windows (signed version includes code signature)
- `*-debug.zip` - Debug symbols for Windows builds

### macOS
- `.dmg` - Disk image for macOS (signed version is notarized by Apple)
- `.tar.gz` - Compressed archive for macOS

### Linux
- `.tar.gz` - Compressed archive for Linux distributions
- `*-debug.tar.gz` - Debug symbols for Linux builds

## Verification

All builds come with corresponding hash files:
- `SHA256SUMS` - Contains SHA256 hashes of all release files
- `SHA256SUMS.asc` - GPG-signed version of the hash file

To verify your download:
1. Check the SHA256 hash of your downloaded file against the value in `SHA256SUMS`
2. Verify the GPG signature of `SHA256SUMS.asc` using the release signing keys

## Source Code

**Files**: `bitcoin-*.tar.gz` (where * is the version number)

The source code archive contains:
- Complete source code for building Bitcoin Core
- Build documentation and scripts
- No compiled binaries

**When to use**: If you want to compile Bitcoin Core yourself or review the source code.

## Additional Information

For more details on:
- Building from source: See [build documentation](README.md)
- The release process: See [release-process.md](release-process.md)
- Verifying signatures: See [verify-binaries documentation](../contrib/verify-binaries/README.md)
