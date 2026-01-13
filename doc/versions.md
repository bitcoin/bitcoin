# libmultiprocess Versions

Library versions are tracked with simple
[tags](https://github.com/bitcoin-core/libmultiprocess/tags) and
[branches](https://github.com/bitcoin-core/libmultiprocess/branches).

Versioning policy is described in the [version.h](../include/mp/version.h)
include.

## v7
- Current unstable version in master branch.
- Intended to be compatible with Bitcoin Core 30.1 and future releases.

## v6.0
- `EventLoop::addClient` and `EventLoop::removeClient` methods dropped,
  requiring clients to use new `EventLoopRef` class instead.
- Compatible with Bitcoin Core 30.0 release.

## v5.0
- Broke up `proxy-types.h` into `type-*.h` files requiring clients to explicitly
  include overloads needed for C++ ↔️ Cap'n Proto type conversions.
- Now requires C++ 20 support.
- Compatible with Bitcoin Core 29 releases.

## v4.0
- Added better cmake support, installing cmake package files so clients do not
  need to use pkgconfig.
- Compatible with Bitcoin Core 28 releases.

## v3.0
- Dropped compatibility with Cap'n Proto versions before 0.7.
- Compatible with Bitcoin Core 27 releases.

## v2.0
- Changed `PassField` function signature.
- Now requires C++17 support.
- Compatible with Bitcoin Core 25 and 26 releases.

## v1.0
- Dropped hardcoded includes in generated files, now requiring `include` and
  `includeTypes` annotations.
- Compatible with Bitcoin Core 22, 23, and 24 releases.

## v0.0
- Initial version used in a downstream release.
- Compatible with Bitcoin Core 21 releases.
