# Dependencies

These are the dependencies used by Bitcoin Core.
You can find installation instructions in the `build-*.md` file for your platform.
"Runtime" and "Version Used" are both in reference to the release binaries.

## Compiler

Bitcoin Core requires one of the following compilers.

| Dependency | Minimum required |
| --- | --- |
| [Clang](https://clang.llvm.org) | [16.0](https://github.com/bitcoin/bitcoin/pull/30263) |
| [GCC](https://gcc.gnu.org) | [11.1](https://github.com/bitcoin/bitcoin/pull/29091) |

## Required

| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| CMake | [link](https://cmake.org/) | N/A | [3.22](https://github.com/bitcoin/bitcoin/pull/30454) | No |
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.81.0](https://github.com/bitcoin/bitcoin/pull/26557) | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) | No |
| [libevent](../depends/packages/libevent.mk) | [link](https://github.com/libevent/libevent/releases) | [2.1.12-stable](https://github.com/bitcoin/bitcoin/pull/21991) | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) | No |
| glibc | [link](https://www.gnu.org/software/libc/) | N/A | [2.31](https://github.com/bitcoin/bitcoin/pull/29987) | Yes |
| Linux Kernel (if building that platform) | [link](https://www.kernel.org/) | N/A | [3.17.0](https://github.com/bitcoin/bitcoin/pull/27699) | Yes |

## Optional

| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Fontconfig](../depends/packages/fontconfig.mk) (gui) | [link](https://www.freedesktop.org/wiki/Software/fontconfig/) | [2.12.6](https://github.com/bitcoin/bitcoin/pull/23495) | 2.6 | Yes |
| [FreeType](../depends/packages/freetype.mk) (gui) | [link](https://freetype.org) | [2.11.0](https://github.com/bitcoin/bitcoin/commit/01544dd78ccc0b0474571da854e27adef97137fb) | 2.3.0 | Yes |
| [qrencode](../depends/packages/qrencode.mk) (gui) | [link](https://fukuchi.org/works/qrencode/) | [4.1.1](https://github.com/bitcoin/bitcoin/pull/27312) | N/A | No |
| [Qt](../depends/packages/qt.mk) (gui) | [link](https://download.qt.io/archive/qt/) | [6.7.3](https://github.com/bitcoin/bitcoin/pull/30997) | [6.2](https://github.com/bitcoin/bitcoin/pull/30997) | No |
| [ZeroMQ](../depends/packages/zeromq.mk) (notifications) | [link](https://github.com/zeromq/libzmq/releases) | [4.3.4](https://github.com/bitcoin/bitcoin/pull/23956) | 4.0.0 | No |
| [Berkeley DB](../depends/packages/bdb.mk) (legacy wallet) | [link](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.30 | 4.8.x | No |
| [SQLite](../depends/packages/sqlite.mk) (wallet) | [link](https://sqlite.org) | [3.38.5](https://github.com/bitcoin/bitcoin/pull/25378) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) | No |
| Python (scripts, tests) | [link](https://www.python.org) | N/A | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) | No |
| [systemtap](../depends/packages/systemtap.mk) ([tracing](tracing.md)) | [link](https://sourceware.org/systemtap/) | [4.8](https://github.com/bitcoin/bitcoin/pull/26945)| N/A | No |
