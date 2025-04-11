# Dependencies

These are the dependencies used by Bitcoin Core.
You can find installation instructions in the `build-*.md` file for your platform, or self-compile
them using [depends](/depends/README.md). "Version Used" refers to the release binaries.

## Compiler

Bitcoin Core requires one of the following compilers.

| Dependency | Minimum required |
| --- | --- |
| [Clang](https://clang.llvm.org) | [16.0](https://github.com/bitcoin/bitcoin/pull/30263) |
| [GCC](https://gcc.gnu.org) | [11.1](https://github.com/bitcoin/bitcoin/pull/29091) |

## Required

### Build

| Dependency | Releases | Version used | Minimum required |
| --- | --- | --- | --- |
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.81.0](https://github.com/bitcoin/bitcoin/pull/26557) | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) |
| CMake | [link](https://cmake.org/) | [3.24.2](https://github.com/bitcoin/bitcoin/pull/30730) | [3.22](https://github.com/bitcoin/bitcoin/pull/30454) |
| [libevent](../depends/packages/libevent.mk) | [link](https://github.com/libevent/libevent/releases) | [2.1.12-stable](https://github.com/bitcoin/bitcoin/pull/21991) | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) |

### Runtime

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| glibc | [link](https://www.gnu.org/software/libc/) | [2.31](https://github.com/bitcoin/bitcoin/pull/29987)

## Optional

### Build

| Dependency | Releases | Version used | Minimum required |
| --- | --- | --- | --- |
| [Berkeley DB](../depends/packages/bdb.mk) (legacy wallet) | [link](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.30 | 4.8.x |
| Python (scripts, tests) | [link](https://www.python.org) | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) |
| [Qt](../depends/packages/qt.mk) (gui) | [link](https://download.qt.io/archive/qt/) | [6.7.3](https://github.com/bitcoin/bitcoin/pull/30997) | [6.2](https://github.com/bitcoin/bitcoin/pull/30997) |
| [qrencode](../depends/packages/qrencode.mk) (gui) | [link](https://fukuchi.org/works/qrencode/) | [4.1.1](https://github.com/bitcoin/bitcoin/pull/27312) | N/A |
| [SQLite](../depends/packages/sqlite.mk) (wallet) | [link](https://sqlite.org) | [3.38.5](https://github.com/bitcoin/bitcoin/pull/25378) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) |
| [systemtap](../depends/packages/systemtap.mk) ([tracing](tracing.md)) | [link](https://sourceware.org/systemtap/) | [4.8](https://github.com/bitcoin/bitcoin/pull/26945)| N/A |
| [ZeroMQ](../depends/packages/zeromq.mk) (notifications) | [link](https://github.com/zeromq/libzmq/releases) | [4.3.4](https://github.com/bitcoin/bitcoin/pull/23956) | 4.0.0 |

### Runtime

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| [Fontconfig](../depends/packages/fontconfig.mk) (gui) | [link](https://www.freedesktop.org/wiki/Software/fontconfig/) | 2.6 |
| [FreeType](../depends/packages/freetype.mk) (gui) | [link](https://freetype.org) | 2.3.0 |
