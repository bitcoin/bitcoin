# Dependencies

These are the dependencies used by Dash Core.
You can find installation instructions in the `build-*.md` file for your platform.
"Runtime" and "Version Used" are both in reference to the release binaries.

| Compiler | Minimum required |
| --- | --- |
| [Autoconf](https://www.gnu.org/software/autoconf/) | [2.69](https://github.com/bitcoin/bitcoin/pull/17769) |
| [Automake](https://www.gnu.org/software/automake/) | [1.13](https://github.com/bitcoin/bitcoin/pull/18290) |
| [Clang](https://clang.llvm.org) | [16.0](https://github.com/bitcoin/bitcoin/pull/30263) |
| [GCC](https://gcc.gnu.org) | [11.1](https://github.com/bitcoin/bitcoin/pull/29091) |
| [Python](https://www.python.org) (scripts, tests) | [3.9](https://github.com/bitcoin/bitcoin/pull/28211) |
| [systemtap](https://sourceware.org/systemtap/) ([tracing](tracing.md))| N/A |

## Required

| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Boost](https://www.boost.org/users/download/) | 1.81.0 | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) | No |
| [libevent](https://github.com/libevent/libevent/releases) | 2.1.12-stable | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) | No |
| [glibc](https://www.gnu.org/software/libc/) | N/A | [2.31](https://github.com/bitcoin/bitcoin/pull/29987) | Yes |

## Optional

| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [libgmp](https://gmplib.org/download/gmp/)<sup>[ \* ](#note1)</sup> | 6.3.0 | [6.2.0](https://github.com/dashpay/bls-signatures/pull/92) | No |

### GUI
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Fontconfig](https://www.freedesktop.org/wiki/Software/fontconfig/) | 2.12.6 | 2.6 | Yes |
| [FreeType](https://freetype.org) | 2.11.0 | 2.3.0 | Yes |
| [qrencode](https://fukuchi.org/works/qrencode/) | [4.1.1](https://fukuchi.org/works/qrencode) | | No |
| [Qt](https://www.qt.io) | [5.15.13](https://download.qt.io/official_releases/qt/) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No |

### Networking
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [libnatpmp](https://github.com/miniupnp/libnatpmp/) | commit [07004b9...](https://github.com/miniupnp/libnatpmp/tree/07004b97cf691774efebe70404cf22201e4d330d) | | No |
| [MiniUPnPc](https://miniupnp.tuxfamily.org/) | 2.2.2 | 1.9 | No |

### Notifications
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [ZeroMQ](https://zeromq.org) | 4.3.5 | 4.0.0 | No |

### Wallet
| Dependency | Version used | Minimum required | Runtime |
| --- | --- | --- | --- |
| [Berkeley DB](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) (legacy wallet) | 4.8.30 | 4.8.x | No |
| [SQLite](https://sqlite.org) | 3.38.5 | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) | No |

<a name="note1">Note \*</a> : The minimum supported version on macOS is 6.3.0
