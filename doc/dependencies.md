# Dependencies

These are the dependencies used by Bitcoin Core.
You can find installation instructions in the `build-*.md` file for your platform.
"Runtime" and "Version Used" are both in reference to the release binaries.

| Dependency | Minimum required |
| --- | --- |
| [Clang](https://clang.llvm.org) | [16.0](https://github.com/bitcoin/bitcoin/pull/30263) |
| [CMake](https://cmake.org/) | [3.22](https://github.com/bitcoin/bitcoin/pull/30454) |
| [GCC](https://gcc.gnu.org) | [11.1](https://github.com/bitcoin/bitcoin/pull/29091) |
| [Python](https://www.python.org) (scripts, tests) | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) |
| [systemtap](https://sourceware.org/systemtap/) ([tracing](tracing.md))| N/A |

## Required

| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.81.0](https://github.com/bitcoin/bitcoin/pull/26557) | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) | No |
| [libevent](../depends/packages/libevent.mk) | [link](https://github.com/libevent/libevent/releases) | [2.1.12-stable](https://github.com/bitcoin/bitcoin/pull/21991) | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) | No |
| glibc | [link](https://www.gnu.org/software/libc/) | N/A | [2.31](https://github.com/bitcoin/bitcoin/pull/29987) | Yes |
| Linux Kernel | [link](https://www.kernel.org/) | N/A | [3.17.0](https://github.com/bitcoin/bitcoin/pull/27699) | Yes |

## Optional

### GUI
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Fontconfig](../depends/packages/fontconfig.mk) | [link](https://www.freedesktop.org/wiki/Software/fontconfig/) | [2.12.6](https://github.com/bitcoin/bitcoin/pull/23495) | 2.6 | Yes |
| [FreeType](../depends/packages/freetype.mk) | [link](https://freetype.org) | [2.11.0](https://github.com/bitcoin/bitcoin/commit/01544dd78ccc0b0474571da854e27adef97137fb) | 2.3.0 | Yes |
| [qrencode](../depends/packages/qrencode.mk) | [link](https://fukuchi.org/works/qrencode/) | [4.1.1](https://github.com/bitcoin/bitcoin/pull/27312) | | No |
| [Qt](../depends/packages/qt.mk) | [link](https://download.qt.io/official_releases/qt/) | [5.15.14](https://github.com/bitcoin/bitcoin/pull/30198) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No |

### Notifications
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [ZeroMQ](../depends/packages/zeromq.mk) | [link](https://github.com/zeromq/libzmq/releases) | [4.3.4](https://github.com/bitcoin/bitcoin/pull/23956) | 4.0.0 | No |

### Wallet
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Berkeley DB](../depends/packages/bdb.mk) (legacy wallet) | [link](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.30 | 4.8.x | No |
| [SQLite](../depends/packages/sqlite.mk) | [link](https://sqlite.org) | [3.38.5](https://github.com/bitcoin/bitcoin/pull/25378) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) | No |
