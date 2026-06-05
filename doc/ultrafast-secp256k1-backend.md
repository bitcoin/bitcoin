# UltrafastSecp256k1 — Optional Secondary secp256k1 Backend

This document describes the `SECP256K1_BACKEND=ultrafast` compile-time option
added by the `feature/ultrafast-secp256k1-backend` branch.

## What this PR does

Adds a single CMake option that lets operators evaluate an alternative secp256k1
engine without modifying any Bitcoin Core C++ source file:

```bash
cmake -B build -DSECP256K1_BACKEND=ultrafast
cmake --build build
```

The default (`SECP256K1_BACKEND=bundled`) is unchanged. All existing users are
unaffected.

## How it works

The alternative engine (UltrafastSecp256k1) ships a shim that exposes the
identical `secp256k1.h` / `secp256k1_schnorrsig.h` / `secp256k1_extrakeys.h` /
`secp256k1_ecdh.h` / `secp256k1_recovery.h` / `secp256k1_ellswift.h` /
`secp256k1_musig.h` API surface. Bitcoin Core links against the shim; no
Bitcoin Core source file is aware of the backend change.

The shim is compiled *into* the engine library (`SECP256K1_BUILD_SHIM=ON`) so
the compiler can inline across the API boundary without LTO.

## Performance (bench_bitcoin, GCC 14.2.0, Release+LTO, Intel i5-14400F)

| Benchmark | bundled | ultrafast | delta |
|---|---|---|---|
| `SignSchnorrWithMerkleRoot` | 113 µs | 84 µs | **+35%** |
| `SignTransactionECDSA` | 165 µs | 150 µs | **+10%** |
| `SignTransactionSchnorr` | 138 µs | 125 µs | **+10%** |
| `VerifyScriptP2TR_ScriptPath` | 84 µs | 77 µs | **+10%** |
| `ConnectBlockAllEcdsa` (LTO) | 257 ms | 254 ms | **+1.2%** |
| `ConnectBlockAllSchnorr` (LTO) | 255 ms | 253 ms | **+0.9%** |

Raw data: `src/ultrafast_secp256k1/docs/BITCOIN_CORE_BENCH_RESULTS.json`

*Without LTO: ConnectBlock ~0.5–1.0% slower due to i-cache pressure. LTO is
recommended for production Bitcoin Core builds and is already the default.*

**Bitcoin Core test suite: 749/749 passing** with `SECP256K1_BACKEND=ultrafast`
(GCC 14.2.0, May 2026).

## Security

- Constant-time signing: ECDSA, Schnorr, MuSig2, FROST, BIP-324 XDH
- Per-context DPA blinding via `secp256k1_context_randomize` (fully implemented)
- Strict scalar parsing: private keys >= n and == 0 are rejected
- BIP-66 strict DER parsing in all shim paths
- 262 exploit PoC security probes, all passing
- CAAS autonomy: 100/100

Audit evidence: `src/ultrafast_secp256k1/docs/BITCOIN_CORE_BACKEND_EVIDENCE.md`

## Known differences from bundled secp256k1

Documented in `src/ultrafast_secp256k1/docs/SHIM_KNOWN_DIVERGENCES.md`.
The most notable: custom nonce function pointers in Schnorr signing are rejected
(fail-closed); only the BIP-340 standard nonce function is accepted.

## Files changed in Bitcoin Core

```
cmake/secp256k1.cmake     # backend selection logic (new CMake option)
src/CMakeLists.txt        # link against secp256k1 or ultrafast_shim
CMakePresets.json         # adds ultrafast preset
.gitmodules               # src/ultrafast_secp256k1 submodule
src/ultrafast_secp256k1/  # submodule (UltrafastSecp256k1 v4.0.0)
```

No changes to any `*.cpp` or `*.h` file in Bitcoin Core itself.
