# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.1] - 2023-04-10
We strongly recommend updating to 0.3.1 if you use or plan to use Clang >=14 to compile libsecp256k1, e.g., Xcode >=14 on macOS has Clang >=14. When in doubt, check the Clang version using `clang -v`.

#### Security
 - Fix "constant-timeness" issue with Clang >=14 that could leave applications using libsecp256k1 vulnerable to a timing side-channel attack. The fix avoids secret-dependent control flow and secret-dependent memory accesses in conditional moves of memory objects when libsecp256k1 is compiled with Clang >=14.

#### Added
  - Added tests against [Project Wycheproof's](https://github.com/google/wycheproof/) set of ECDSA test vectors (Bitcoin "low-S" variant), a fixed set of test cases designed to trigger various edge cases.

#### Changed
 - Increased minimum required CMake version to 3.13. CMake builds remain experimental.

#### ABI Compatibility
The ABI is compatible with version 0.3.0.

## [0.3.0] - 2023-03-08

#### Added
 - Added experimental support for CMake builds. Traditional GNU Autotools builds (`./configure` and `make`) remain fully supported.
 - Usage examples: Added a recommended method for securely clearing sensitive data, e.g., secret keys, from memory.
 - Tests: Added a new test binary `noverify_tests`. This binary runs the tests without some additional checks present in the ordinary `tests` binary and is thereby closer to production binaries. The `noverify_tests` binary is automatically run as part of the `make check` target.

#### Fixed
 - Fixed declarations of API variables for MSVC (`__declspec(dllimport)`). This fixes MSVC builds of programs which link against a libsecp256k1 DLL dynamically and use API variables (and not only API functions). Unfortunately, the MSVC linker now will emit warning `LNK4217` when trying to link against libsecp256k1 statically. Pass `/ignore:4217` to the linker to suppress this warning.

#### Changed
 - Forbade cloning or destroying `secp256k1_context_static`. Create a new context instead of cloning the static context. (If this change breaks your code, your code is probably wrong.)
 - Forbade randomizing (copies of) `secp256k1_context_static`. Randomizing a copy of `secp256k1_context_static` did not have any effect and did not provide defense-in-depth protection against side-channel attacks. Create a new context if you want to benefit from randomization.

#### Removed
 - Removed the configuration header `src/libsecp256k1-config.h`. We recommend passing flags to `./configure` or `cmake` to set configuration options (see `./configure --help` or `cmake -LH`). If you cannot or do not want to use one of the supported build systems, pass configuration flags such as `-DSECP256K1_ENABLE_MODULE_SCHNORRSIG` manually to the compiler (see the file `configure.ac` for supported flags).

#### ABI Compatibility
Due to changes in the API regarding `secp256k1_context_static` described above, the ABI is *not* compatible with previous versions.

## [0.2.0] - 2022-12-12

#### Added
 - Added usage examples for common use cases in a new `examples/` directory.
 - Added `secp256k1_selftest`, to be used in conjunction with `secp256k1_context_static`.
 - Added support for 128-bit wide multiplication on MSVC for x86_64 and arm64, giving roughly a 20% speedup on those platforms.

#### Changed
 - Enabled modules `schnorrsig`, `extrakeys` and `ecdh` by default in `./configure`.
 - The `secp256k1_nonce_function_rfc6979` nonce function, used by default by `secp256k1_ecdsa_sign`, now reduces the message hash modulo the group order to match the specification. This only affects improper use of ECDSA signing API.

#### Deprecated
 - Deprecated context flags `SECP256K1_CONTEXT_VERIFY` and `SECP256K1_CONTEXT_SIGN`. Use `SECP256K1_CONTEXT_NONE` instead.
 - Renamed `secp256k1_context_no_precomp` to `secp256k1_context_static`.
 - Module `schnorrsig`: renamed `secp256k1_schnorrsig_sign` to `secp256k1_schnorrsig_sign32`.

#### ABI Compatibility
Since this is the first release, we do not compare application binary interfaces.
However, there are earlier unreleased versions of libsecp256k1 that are *not* ABI compatible with this version.

## [0.1.0] - 2013-03-05 to 2021-12-25

This version was in fact never released.
The number was given by the build system since the introduction of autotools in Jan 2014 (ea0fe5a5bf0c04f9cc955b2966b614f5f378c6f6).
Therefore, this version number does not uniquely identify a set of source files.

[unreleased]: https://github.com/bitcoin-core/secp256k1/compare/v0.3.1...HEAD
[0.3.1]: https://github.com/bitcoin-core/secp256k1/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/bitcoin-core/secp256k1/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/bitcoin-core/secp256k1/compare/423b6d19d373f1224fd671a982584d7e7900bc93..v0.2.0
[0.1.0]: https://github.com/bitcoin-core/secp256k1/commit/423b6d19d373f1224fd671a982584d7e7900bc93
