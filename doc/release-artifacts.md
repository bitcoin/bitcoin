# Release Artifacts

Bitcoin Core releases are distributed via https://bitcoincore.org/bin/ and torrent. This guide clarifies the naming conventions and purposes of the various build artifacts.

## Build Suffixes

- **Standard Binaries** (`.tar.gz`, `.zip`, `-setup.exe`): Production-ready, signed binaries for general use.
- **Debug Builds** (`-debug`): Contains full symbols for troubleshooting and backtrace generation. See [debugging_bitcoin](https://github.com/fjahr/debugging_bitcoin) for usage.
- **Unsigned Artifacts** (`-unsigned`): Raw outputs from the Guix build process. These allow verifiers to audit the build's reproducibility against the signed releases.
- **Codesigning Archives** (`-codesigning`): Intermediate artifacts used during the macOS/Windows signing process. Not intended for end-user installation.
- **Source Tarball**: `bitcoin-<version>.tar.gz` (no platform in the name) contains the release source code for manual compilation.

## Verification Files

| File | Description |
|------|-------------|
| `SHA256SUMS` | Manifest of file hashes. |
| `SHA256SUMS.asc` | Detached GPG signatures from maintainers. |
| `SHA256SUMS.ots` | OpenTimestamps proof for the release manifest. |

For step-by-step verification instructions, see https://bitcoincore.org/en/download/.

## Platform Identifiers

| Identifier | Platform |
|------------|----------|
| `x86_64-linux-gnu` | 64-bit Linux (x86) |
| `aarch64-linux-gnu` | 64-bit ARM Linux |
| `arm-linux-gnueabihf` | 32-bit ARM Linux (hard float) |
| `riscv64-linux-gnu` | 64-bit RISC-V Linux |
| `powerpc64-linux-gnu` | 64-bit POWER Linux |
| `x86_64-apple-darwin` | macOS (Intel) |
| `arm64-apple-darwin` | macOS (Apple Silicon) |
| `win64` | 64-bit Windows |
