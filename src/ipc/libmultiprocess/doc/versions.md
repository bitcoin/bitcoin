# libmultiprocess Versions

Library versions are tracked with simple
[tags](https://github.com/bitcoin-core/libmultiprocess/tags) and
[branches](https://github.com/bitcoin-core/libmultiprocess/branches).

Versioning policy is described in the [version.h](../include/mp/version.h)
include.

## v13
- Current unstable version.

## [v12.0](https://github.com/bitcoin-core/libmultiprocess/commits/v12.0)
- Adds an optional `max_connections` parameter to `ListenConnections` ([#269](https://github.com/bitcoin-core/libmultiprocess/pull/269)).
- Used in Bitcoin Core 32.x and newer releases with [#35684](https://github.com/bitcoin/bitcoin/pull/35684).

## [v11.0](https://github.com/bitcoin-core/libmultiprocess/commits/v11.0)
- Adds `makePool` method on `ThreadMap` to support thread pool routing, allowing requests without a specific client thread to be dispatched to a pool using a shortest-queue strategy ([#283](https://github.com/bitcoin-core/libmultiprocess/pull/283)).
- Adds `std::unordered_set` support, a `BuildList` helper, and a `ReadList` helper to reduce duplication in list build and read handlers ([#277](https://github.com/bitcoin-core/libmultiprocess/pull/277), [#285](https://github.com/bitcoin-core/libmultiprocess/pull/285)).
- Adds support for translating C++ `std::optional<T>` struct fields to pairs of `T` + `hasT :Bool` Cap'n Proto struct fields, allowing unset optional primitive fields to be represented ([#243](https://github.com/bitcoin-core/libmultiprocess/pull/243)).
- Produces more readable log output for Proxy object lifecycle events and IPC server-side failures ([#218](https://github.com/bitcoin-core/libmultiprocess/pull/218)).
- Handles exceptions thrown by `destroy` methods by logging instead of aborting ([#273](https://github.com/bitcoin-core/libmultiprocess/pull/273)). This can prevent server crashes when non-libmultiprocess clients disconnect without destroying objects, in the case where a server object owns client objects and the server destructor tries to call the disconnected client to free them ([#219](https://github.com/bitcoin-core/libmultiprocess/issues/219)).
- Handles unexpected exceptions thrown by callbacks (that should never happen) by logging errors instead of deadlocking ([#260](https://github.com/bitcoin-core/libmultiprocess/pull/260)).
- Fixes a rare mptest hang on musl builds caused by a lost wakeup bug in `Waiter` ([#295](https://github.com/bitcoin-core/libmultiprocess/pull/295)).
- Fixes a race condition in a log print detected by TSan ([#286](https://github.com/bitcoin-core/libmultiprocess/pull/286)).
- Build improvements: makes `target_capnp_sources` work correctly when libmultiprocess is used as a CMake subproject ([#289](https://github.com/bitcoin-core/libmultiprocess/pull/289)), adds `mp_headers` target for better lint tool support ([#291](https://github.com/bitcoin-core/libmultiprocess/pull/291)), and fixes compatibility with recent Nix and CMake 4.0 ([#238](https://github.com/bitcoin-core/libmultiprocess/pull/238)).
- Test, CI, documentation, and minor code improvements: design document corrections ([#278](https://github.com/bitcoin-core/libmultiprocess/pull/278)), field constant comments ([#279](https://github.com/bitcoin-core/libmultiprocess/pull/279)), clang-tidy fix ([#292](https://github.com/bitcoin-core/libmultiprocess/pull/292)), new smoke test for double-precision float values ([#294](https://github.com/bitcoin-core/libmultiprocess/pull/294)), new test for recursive async IPC calls ([#301](https://github.com/bitcoin-core/libmultiprocess/pull/301)), removal of libevent from Core CI builds ([#299](https://github.com/bitcoin-core/libmultiprocess/pull/299)), and rename of `EventLoop::m_num_clients` to `m_num_refs` ([#302](https://github.com/bitcoin-core/libmultiprocess/pull/302)).
- Used in Bitcoin Core 32.x and newer releases with [#35661](https://github.com/bitcoin/bitcoin/pull/35661).

## [v10.0](https://github.com/bitcoin-core/libmultiprocess/commits/v10.0)
- Increases spawn test timeout to avoid spurious failures.
- Uses `throwRecoverableException` instead of raw `throw` to improve runtime error messages in macOS builds.
- Used in Bitcoin Core 31.0rc3 and newer releases with [#34977](https://github.com/bitcoin/bitcoin/pull/34977) and backport [#35028](https://github.com/bitcoin/bitcoin/pull/35028).

## [v9.0](https://github.com/bitcoin-core/libmultiprocess/commits/v9.0)
- Fixes race conditions where worker thread could be used after destruction, where getParams() could be called after request cancel, and where m_on_cancel could be called after request finishes.
- Adds `CustomHasField` hook to map Cap'n Proto null values to C++ null values.
- Improves `CustomBuildField` for `std::optional` to use move semantics.
- Adds LLVM 22 compatibility fix in type-map.
- Used in Bitcoin Core 31.0rc3 and newer releases with [#34804](https://github.com/bitcoin/bitcoin/pull/34804) and backport [#34952](https://github.com/bitcoin/bitcoin/pull/34952).

## [v8.0](https://github.com/bitcoin-core/libmultiprocess/commits/v8.0)
- Better support for non-libmultiprocess IPC clients: avoiding errors on unclean disconnects, and allowing simultaneous requests to worker threads which previously triggered "thread busy" errors.
- Used in Bitcoin Core 31.x and newer releases with [#34422](https://github.com/bitcoin/bitcoin/pull/34422).

## [v7.0](https://github.com/bitcoin-core/libmultiprocess/commits/v7.0)
- Adds SpawnProcess race fix, cmake `target_capnp_sources` option, ci and documentation improvements. Adds `version.h` header declaring major and minor version numbers.
- Used in Bitcoin Core 31.x and newer releases with [#34363](https://github.com/bitcoin/bitcoin/pull/34363).

## [v7.0-pre2](https://github.com/bitcoin-core/libmultiprocess/commits/v7.0-pre2)
- Fixes intermittent mptest hang and makes other minor improvements.
- Used in Bitcoin Core 30.1 and newer releases with [#33518](https://github.com/bitcoin/bitcoin/pull/33518) and backport [#33519](https://github.com/bitcoin/bitcoin/pull/33519).

## [v7.0-pre1](https://github.com/bitcoin-core/libmultiprocess/commits/v7.0-pre1)
- Adds support for log levels to reduce logging and "thread busy" error to avoid a crash on misuse.
- Used in Bitcoin Core 30.1 and newer releases with [#33412](https://github.com/bitcoin/bitcoin/pull/33412) and [#33518](https://github.com/bitcoin/bitcoin/pull/33518) and backport [#33519](https://github.com/bitcoin/bitcoin/pull/33519).
- Minimum required version since Bitcoin Core 30.1 due to [#33517](https://github.com/bitcoin/bitcoin/pull/33517) and backport [#33609](https://github.com/bitcoin/bitcoin/pull/33609).

## [v6.0](https://github.com/bitcoin-core/libmultiprocess/commits/v6.0)
- Adds CI scripts and build and test fixes.
- Used in Bitcoin Core 30.x and newer releases with [#32345](https://github.com/bitcoin/bitcoin/pull/32345), [#33241](https://github.com/bitcoin/bitcoin/pull/33241), and [#33322](https://github.com/bitcoin/bitcoin/pull/33322).

## [v6.0-pre1](https://github.com/bitcoin-core/libmultiprocess/commits/v6.0-pre1)
- Adds fixes for unclean shutdowns and thread sanitizer issues.
- Drops `EventLoop::addClient` and `EventLoop::removeClient` methods,
  requiring clients to use new `EventLoopRef` class instead.
- Used in Bitcoin Core 30.x and newer releases with [#31741](https://github.com/bitcoin/bitcoin/pull/31741), [#32641](https://github.com/bitcoin/bitcoin/pull/32641), and [#32345](https://github.com/bitcoin/bitcoin/pull/32345).
- Minimum required version since Bitcoin Core 30.0 due to [#32345](https://github.com/bitcoin/bitcoin/pull/32345).

## [v5.0](https://github.com/bitcoin-core/libmultiprocess/commits/v5.0)
- Adds build improvements and tidy/warning fixes.
- Used in Bitcoin Core 29.x and newer releases with [#31945](https://github.com/bitcoin/bitcoin/pull/31945).

## [v5.0-pre1](https://github.com/bitcoin-core/libmultiprocess/commits/v5.0-pre1)
- Adds many improvements to Bitcoin Core mining interface: splitting up type headers, fixing shutdown bugs, adding subtree build support.
- Broke up `proxy-types.h` into `type-*.h` files requiring clients to explicitly
  include overloads needed for C++ ↔️ Cap'n Proto type conversions.
- Now requires C++ 20 support.
- Used in Bitcoin Core 29.x and newer releases with [#30509](https://github.com/bitcoin/bitcoin/pull/30509), [#30510](https://github.com/bitcoin/bitcoin/pull/30510), [#31105](https://github.com/bitcoin/bitcoin/pull/31105), [#31740](https://github.com/bitcoin/bitcoin/pull/31740).
- Minimum required version since Bitcoin Core 29.0 due to [#31740](https://github.com/bitcoin/bitcoin/pull/31740).

## [v4.0](https://github.com/bitcoin-core/libmultiprocess/commits/v4.0)
- Added better cmake support, installing cmake package files so clients do not
  need to use pkgconfig.
- Used in Bitcoin Core 28.x and newer releases with [#30490](https://github.com/bitcoin/bitcoin/pull/30490) and [#30513](https://github.com/bitcoin/bitcoin/pull/30513).

## [v3.0](https://github.com/bitcoin-core/libmultiprocess/commits/v3.0)
- Dropped compatibility with Cap'n Proto versions before 0.7.
- Used in Bitcoin Core 27.x and newer releases with [#28735](https://github.com/bitcoin/bitcoin/pull/28735), [#28907](https://github.com/bitcoin/bitcoin/pull/28907), and [#29276](https://github.com/bitcoin/bitcoin/pull/29276).

## [v2.0](https://github.com/bitcoin-core/libmultiprocess/commits/v2.0)
- Changed `PassField` function signature.
- Now requires C++17 support.
- Used in Bitcoin Core 25.x and newer releases with [#26672](https://github.com/bitcoin/bitcoin/pull/26672).

## [v1.0](https://github.com/bitcoin-core/libmultiprocess/commits/v1.0)
- Dropped hardcoded includes in generated files, now requiring `include` and
  `includeTypes` annotations.
- Used in Bitcoin Core 22.x and newer releases with [#19160](https://github.com/bitcoin/bitcoin/pull/19160).

## [v0.0](https://github.com/bitcoin-core/libmultiprocess/commits/v0.0)
- Initial version used in a downstream release.
- Used in Bitcoin Core 21.x and newer releases with [#16367](https://github.com/bitcoin/bitcoin/pull/16367), [#18588](https://github.com/bitcoin/bitcoin/pull/18588), and [#18677](https://github.com/bitcoin/bitcoin/pull/18677).
