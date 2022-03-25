# Dependencies

These are the dependencies used by Bitcoin Core.
You can find installation instructions in the `build-*.md` file for your platform.
"Runtime" and "Version Used" are both in reference to the release binaries.

| Dependency | Minimum required |
| --- | --- |
| [Autoconf](https://www.gnu.org/software/autoconf/) | [2.69](https://github.com/bitcoin/bitcoin/pull/17769) |
| [Automake](https://www.gnu.org/software/automake/) | [1.13](https://github.com/bitcoin/bitcoin/pull/18290) |
| [Clang](https://clang.llvm.org) | [8.0](https://github.com/bitcoin/bitcoin/pull/24164) |
| [GCC](https://gcc.gnu.org) | [8.1](https://github.com/bitcoin/bitcoin/pull/23060) |
| [Python](https://www.python.org) (tests) | [3.6](https://github.com/bitcoin/bitcoin/pull/19504) |
| [systemtap](https://sourceware.org/systemtap/) ([tracing](tracing.md))| N/A |

## Required

| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Boost](https://www.boost.org/users/download/) | 1.77.0 | [1.64.0](https://github.com/bitcoin/bitcoin/pull/22320) | No |
| [libevent](https://github.com/libevent/libevent/releases) | 2.1.12-stable | [2.0.21](https://github.com/bitcoin/bitcoin/pull/18676) | No |
| [glibc](https://www.gnu.org/software/libc/) | N/A | [2.18](https://github.com/bitcoin/bitcoin/pull/23511) | Yes |

## Optional

### GUI
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Fontconfig](https://www.freedesktop.org/wiki/Software/fontconfig/) | 2.12.6 | 2.6 | Yes |
| [FreeType](https://freetype.org) | 2.11.0 | 2.3.0 | Yes |
| [qrencode](https://fukuchi.org/works/qrencode/) | [3.4.4](https://fukuchi.org/works/qrencode) | | No |
| [Qt](https://www.qt.io) | [5.15.3](https://download.qt.io/official_releases/qt/) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No |

### Networking
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [libnatpmp](https://github.com/miniupnp/libnatpmp/) | commit [4536032...](https://github.com/miniupnp/libnatpmp/tree/4536032ae32268a45c073a4d5e91bbab4534773a) | | No |
| [MiniUPnPc](https://miniupnp.tuxfamily.org/) | 2.2.2 | 1.9 | No |

### Notifications
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [ZeroMQ](https://zeromq.org) | 4.3.4 | 4.0.0 | No |

### Wallet
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Berkeley DB](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) (legacy wallet) | 4.8.30 | 4.8.x | No |
| [SQLite](https://sqlite.org) | 3.32.1 | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) | No |
