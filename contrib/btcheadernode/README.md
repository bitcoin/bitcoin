# BTC header-node helper build

This directory provides an **optional** build path for Bitcoin Core binaries used by sentry
nodes that perform BTC header-chain policy checks for BTCC signing.

## What it does

- Pins upstream Bitcoin source in `bitcoin.lock`.
- Optionally applies a local patch (for a headers-only fork workflow).
- Auto-detects Bitcoin build system (Autotools for older releases, CMake for newer releases).
- Builds `bitcoind` and `bitcoin-cli`.
- Writes outputs to:
  - `<build-root>/src/bin/btcheadernode/bin/bitcoind`
  - `<build-root>/src/bin/btcheadernode/bin/bitcoin-cli`

## Configure / build

From the Syscoin root:

```bash
./autogen.sh
./configure --enable-btcheadernode-build
make -j$(nproc)
```

If the option is not enabled, this build path is skipped.

When enabled for Linux x86_64 release builds, the produced `bitcoind` and
`bitcoin-cli` are installed into the final package `bin/` directory so sentry
operators can use the shipped binaries directly.

Guix release builds pre-stage a pinned Bitcoin source archive and pass it to the
helper script, so the in-container build does not perform live VCS fetches.

## Lock file

`bitcoin.lock` controls the pinned upstream source:

- `BITCOIN_REPO`: upstream Git URL
- `BITCOIN_REF`: human-readable tag/branch (optional validation anchor)
- `BITCOIN_COMMIT`: immutable commit to build
- `BITCOIN_PATCH`: optional path (relative to `contrib/btcheadernode/`) to apply with `git apply`
- `BITCOIN_PATCH_FORK_REPO`: optional fork repo for patch generation workflow
- `BITCOIN_PATCH_FORK_REF`: optional fork ref for patch generation workflow

The build script validates that `BITCOIN_REF` resolves to `BITCOIN_COMMIT` when `BITCOIN_REF` is set.

## Patch workflow

`HEADERS_ONLY_PATCH_SPEC.md` describes the expected runtime/RPC contract for header-only mode.

To generate `patches/headers-only.diff` from your Bitcoin fork branch:

```bash
contrib/btcheadernode/generate-headers-only-patch.sh \
  --src-root "$(pwd)" \
  --fork-repo https://github.com/syscoin/bitcoin.git \
  --fork-ref headers-only-v30.2
```

If `BITCOIN_PATCH_FORK_REPO` (and optionally `BITCOIN_PATCH_FORK_REF`) are set in
`bitcoin.lock`, you can omit `--fork-repo/--fork-ref`.

Then set `BITCOIN_PATCH` in `bitcoin.lock`:

- `BITCOIN_PATCH=patches/headers-only.diff`

Re-run `make` (or call the build script directly).

## Direct invocation

```bash
contrib/btcheadernode/build-bitcoin-header-node.sh \
  --src-root "$(pwd)" \
  --build-root "$(pwd)"
```

Use `--force-clean` if you want to discard the cached Bitcoin source/build workdirs before rebuilding.

Optional environment variables for direct invocation:

- `BTCHEADERNODE_SOURCE_ARCHIVE`: path to local Bitcoin source archive to use instead of cloning.
- `BTCHEADERNODE_STATIC_LINK_FLAGS`: linker flags for `bitcoind`/`bitcoin-cli` (defaults to `-static-libstdc++`).
- `LDFLAGS`: optional base linker flags inherited from the outer build (for Guix alignment).
