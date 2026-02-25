# libmultiprocess Versions

Library versions are tracked with simple
[tags](https://github.com/bitcoin-core/libmultiprocess/tags) and
[branches](https://github.com/bitcoin-core/libmultiprocess/branches).

Versioning policy is described in the [version.h](../include/mp/version.h)
include.

## v8
- Current unstable version in master branch.

## v7.0
- Adds SpawnProcess race fix, cmake `target_capnp_sources` option, ci and documentation improvements.
- Used in Bitcoin Core master branch, pulled in by [#34363](https://github.com/bitcoin/bitcoin/pull/34363).

## v7.0-pre1

- Adds support for log levels to reduce logging and "thread busy" error to avoid a crash on misuse. Fixes intermittent mptest hang and makes other minor improvements.
- Used in Bitcoin Core 30.1 and 30.2 releases and 30.x branch. Pulled in by [#33412](https://github.com/bitcoin/bitcoin/pull/33412), [#33518](https://github.com/bitcoin/bitcoin/pull/33518), and [#33519](https://github.com/bitcoin/bitcoin/pull/33519).

## v6.0

- Adds fixes for unclean shutdowns and thread sanitizer issues and adds CI scripts.
- Drops `EventLoop::addClient` and `EventLoop::removeClient` methods,
  requiring clients to use new `EventLoopRef` class instead.
- Used in Bitcoin Core 30.0 release. Pulled in by [#31741](https://github.com/bitcoin/bitcoin/pull/31741), [#32641](https://github.com/bitcoin/bitcoin/pull/32641), [#32345](https://github.com/bitcoin/bitcoin/pull/32345), [#33241](https://github.com/bitcoin/bitcoin/pull/33241), and [#33322](https://github.com/bitcoin/bitcoin/pull/33322).

## v5.0
- Adds build improvements and tidy/warning fixes.
- Used in Bitcoin Core 29 releases, pulled in by [#31945](https://github.com/bitcoin/bitcoin/pull/31945).

## v5.0-pre1
- Adds many improvements to Bitcoin Core mining interface: splitting up type headers, fixing shutdown bugs, adding subtree build support.
- Broke up `proxy-types.h` into `type-*.h` files requiring clients to explicitly
  include overloads needed for C++ ↔️ Cap'n Proto type conversions.
- Now requires C++ 20 support.
- Minimum required version for Bitcoin Core 29 releases, pulled in by [#30509](https://github.com/bitcoin/bitcoin/pull/30509), [#30510](https://github.com/bitcoin/bitcoin/pull/30510), [#31105](https://github.com/bitcoin/bitcoin/pull/31105), [#31740](https://github.com/bitcoin/bitcoin/pull/31740).

## v4.0
- Added better cmake support, installing cmake package files so clients do not
  need to use pkgconfig.
- Used in Bitcoin Core 28 releases, pulled in by [#30490](https://github.com/bitcoin/bitcoin/pull/30490) and [#30513](https://github.com/bitcoin/bitcoin/pull/30513).

## v3.0
- Dropped compatibility with Cap'n Proto versions before 0.7.
- Used in Bitcoin Core 27 releases, pulled in by [#28735](https://github.com/bitcoin/bitcoin/pull/28735), [#28907](https://github.com/bitcoin/bitcoin/pull/28907), and [#29276](https://github.com/bitcoin/bitcoin/pull/29276).

## v2.0
- Changed `PassField` function signature.
- Now requires C++17 support.
- Used in Bitcoin Core 25 and 26 releases, pulled in by [#26672](https://github.com/bitcoin/bitcoin/pull/26672).

## v1.0
- Dropped hardcoded includes in generated files, now requiring `include` and
  `includeTypes` annotations.
- Used in Bitcoin Core 22, 23, and 24 releases, pulled in by [#19160](https://github.com/bitcoin/bitcoin/pull/19160).

## v0.0
- Initial version used in a downstream release.
- Used in Bitcoin Core 21 releases, pulled in by [#16367](https://github.com/bitcoin/bitcoin/pull/16367), [#18588](https://github.com/bitcoin/bitcoin/pull/18588), and [#18677](https://github.com/bitcoin/bitcoin/pull/18677).
