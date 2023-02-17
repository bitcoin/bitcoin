
These are the dependencies currently used by Syscoin Core. You can find instructions for installing them in the `build-*.md` file for your platform.

| Dependency | Version used | Minimum required | CVEs | Shared | [Bundled Qt library](https://doc.qt.io/qt-5/configure-options.html#third-party-libraries) |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [4.8.30](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| Boost | [1.77.0](https://www.boost.org/users/download/) | [1.64.0](https://github.com/bitcoin/bitcoin/pull/22320) | No |  |  |
| Clang<sup>[ \* ](#note1)</sup> |  | [7.0](https://releases.llvm.org/download.html) (C++17 & std::filesystem support) |  |  |  |
| Fontconfig | [2.12.6](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.11.0](https://download.savannah.gnu.org/releases/freetype) |  | No |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Android only) |
| GCC |  | [8.1](https://gcc.gnu.org/) (C++17 & std::filesystem support) |  |  |  |
| glibc | | [2.18](https://www.gnu.org/software/libc/) |  |  |  |  |
| HarfBuzz-NG |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| libevent | [2.1.12-stable](https://github.com/libevent/libevent/releases) | [2.0.21](https://github.com/bitcoin/bitcoin/pull/18676) | No |  |  |
| libnatpmp | git commit [4536032...](https://github.com/miniupnp/libnatpmp/tree/4536032ae32268a45c073a4d5e91bbab4534773a) |  | No |  |  |
| libpng |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| MiniUPnPc | [2.2.2](https://miniupnp.tuxfamily.org/files) |  | No |  |  |
| PCRE |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| Python (tests) |  | [3.6](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [5.15.2](https://download.qt.io/official_releases/qt/) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No |  |  |
| SQLite | [3.32.1](https://sqlite.org/download.html) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) |  |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Linux only) |
| systemtap ([tracing](tracing.md))| [4.5](https://sourceware.org/systemtap/ftp/releases/) |  |  |  | |
| xkbcommon |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Linux only) |
| ZeroMQ | [4.3.1](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |

<a name="note1">Note \*</a> : When compiling with `-stdlib=libc++`, the minimum supported libc++ version is 7.0.

Controlling dependencies
------------------------
Some dependencies are not needed in all configurations. The following are some factors that affect the dependency list.

#### Options passed to `./configure`
* MiniUPnPc is not needed with `--without-miniupnpc`.
* libnatpmp is not needed with `--without-natpmp`.
* Berkeley DB is not needed with `--disable-wallet` or `--without-bdb`.
* SQLite is not needed with `--disable-wallet` or `--without-sqlite`.
* Qt is not needed with `--without-gui`.
* If the qrencode dependency is absent, QR support won't be added. To force an error when that happens, pass `--with-qrencode`.
* If the systemtap dependency is absent, USDT support won't compiled in.
* ZeroMQ is needed only with the `--with-zmq` option.

#### Other
* Not-Qt-bundled zlib is required to build the [DMG tool](../contrib/macdeploy/README.md#deterministic-macos-dmg-notes) from the libdmg-hfsplus project.

# Dependencies

These are the dependencies used by Syscoin Core.
You can find installation instructions in the `build-*.md` file for your platform.
"Runtime" and "Version Used" are both in reference to the release binaries.

| Dependency | Minimum required |
| --- | --- |
| [Autoconf](https://www.gnu.org/software/autoconf/) | [2.69](https://github.com/bitcoin/bitcoin/pull/17769) |
| [Automake](https://www.gnu.org/software/automake/) | [1.13](https://github.com/bitcoin/bitcoin/pull/18290) |
| [Clang](https://clang.llvm.org) | [8.0](https://github.com/bitcoin/bitcoin/pull/24164) |
| [GCC](https://gcc.gnu.org) | [8.1](https://github.com/bitcoin/bitcoin/pull/23060) |
| [Python](https://www.python.org) (tests) | [3.7](https://github.com/bitcoin/bitcoin/pull/26226) |
| [systemtap](https://sourceware.org/systemtap/) ([tracing](tracing.md))| N/A |

## Required

| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.81.0](https://github.com/bitcoin/bitcoin/pull/26557) | [1.64.0](https://github.com/bitcoin/bitcoin/pull/22320) | No |
| [libevent](../depends/packages/libevent.mk) | [link](https://github.com/libevent/libevent/releases) | [2.1.12-stable](https://github.com/bitcoin/bitcoin/pull/21991) | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) | No |
| glibc | [link](https://www.gnu.org/software/libc/) | N/A | [2.27](https://github.com/bitcoin/bitcoin/pull/27029) | Yes |
| Linux Kernel | [link](https://www.kernel.org/) | N/A | 3.2.0 | Yes |

## Optional

### GUI
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Fontconfig](../depends/packages/fontconfig.mk) | [link](https://www.freedesktop.org/wiki/Software/fontconfig/) | [2.12.6](https://github.com/bitcoin/bitcoin/pull/23495) | 2.6 | Yes |
| [FreeType](../depends/packages/freetype.mk) | [link](https://freetype.org) | [2.11.0](https://github.com/bitcoin/bitcoin/commit/01544dd78ccc0b0474571da854e27adef97137fb) | 2.3.0 | Yes |
| [qrencode](../depends/packages/qrencode.mk) | [link](https://fukuchi.org/works/qrencode/) | [3.4.4](https://github.com/bitcoin/bitcoin/pull/6373) | | No |
| [Qt](../depends/packages/qt.mk) | [link](https://download.qt.io/official_releases/qt/) | [5.15.5](https://github.com/bitcoin/bitcoin/pull/25719) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No |

### Networking
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [libnatpmp](../depends/packages/libnatpmp.mk) | [link](https://github.com/miniupnp/libnatpmp/) | commit [07004b9...](https://github.com/bitcoin/bitcoin/pull/25917) | | No |
| [MiniUPnPc](../depends/packages/miniupnpc.mk) | [link](https://miniupnp.tuxfamily.org/) | [2.2.2](https://github.com/bitcoin/bitcoin/pull/20421) | 2.1 | No |

### Notifications
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [ZeroMQ](../depends/packages/zeromq.mk) | [link](https://github.com/zeromq/libzmq/releases) | [4.3.4](https://github.com/bitcoin/bitcoin/pull/23956) | 4.0.0 | No |

### Wallet
| Dependency | Releases | Version used | Minimum required | Runtime |
| --- | --- | --- | --- | --- |
| [Berkeley DB](../depends/packages/bdb.mk) (legacy wallet) | [link](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.30 | 4.8.x | No |
| [SQLite](../depends/packages/sqlite.mk) | [link](https://sqlite.org) | [3.32.1](https://github.com/bitcoin/bitcoin/pull/19077) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) | No |
