Dependencies
============

These are the dependencies currently used by Dash Core. You can find instructions for installing them in the `build-*.md` file for your platform.

| Dependency | Version used | Minimum required | CVEs | Shared | [Bundled Qt library](https://doc.qt.io/qt-5/configure-options.html#third-party-libraries) |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [4.8.30](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| Boost | [1.81.0](https://www.boost.org/users/download/) | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) | No |  |  |
| Clang<sup>[ \* ](#note1)</sup> |  | [5.0+](https://releases.llvm.org/download.html) (C++17 support) |  |  |  |
| fontconfig | [2.12.1](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.11.0](https://download.savannah.gnu.org/releases/freetype) |  | No |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) (Android only) |
| GCC |  | [7+](https://gcc.gnu.org/) (C++17 support) |  |  |  |
| glibc | | [2.31](https://www.gnu.org/software/libc/) |  |  |  |  |
| HarfBuzz-NG |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) |
| libevent | [2.1.12-stable](https://github.com/libevent/libevent/releases) | [2.0.21](https://github.com/bitcoin/bitcoin/pull/18676) | No |  |  |
| libnatpmp | git commit [07004b9...](https://github.com/miniupnp/libnatpmp/tree/07004b97cf691774efebe70404cf22201e4d330d) |  | No |  |  |
| libpng |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) |
| Linux Kernel | [N/A](https://www.kernel.org/) | 3.2.0 | | | |
| MiniUPnPc | [2.2.2](https://miniupnp.tuxfamily.org/files) |  | No |  |  |
| PCRE |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) |
| Python (tests) |  | [3.9](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [5.15.13](https://download.qt.io/official_releases/qt/) | [5.11.3](https://github.com/bitcoin/bitcoin/pull/24132) | No | |  |
| SQLite | [3.32.1](https://sqlite.org/download.html) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) |  |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) (Linux only) |
| systemtap ([tracing](tracing.md))| [4.8](https://sourceware.org/ftp/systemtap/releases/) |  |  |  | |
| xkbcommon |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) (Linux only) |
| ZeroMQ | [4.3.1](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib |  |  |  |  | [Yes](https://github.com/dashpay/dash/blob/develop/depends/packages/qt.mk) |

<a name="note1">Note \*</a> : When compiling with `-stdlib=libc++`, the minimum supported libc++ version is 7.0.

Controlling dependencies
------------------------
Some dependencies are not needed in all configurations. The following are some factors that affect the dependency list.

#### Options passed to `./configure`
* MiniUPnPc is not needed with `--without-miniupnpc`.
* Berkeley DB is not needed with `--disable-wallet` or `--without-bdb`.
* SQLite is not needed with `--disable-wallet` or `--without-sqlite`.
* libnatpmp is not needed with `--without-natpmp`.
* Qt is not needed with `--without-gui`.
* If the qrencode dependency is absent, QR support won't be added. To force an error when that happens, pass `--with-qrencode`.
* If the systemtap dependency is absent, USDT support won't compiled in.
* ZeroMQ is needed only with the `--with-zmq` option.

#### Other
* Not-Qt-bundled zlib is required to build the [DMG tool](../contrib/macdeploy/README.md#deterministic-macos-dmg-notes) from the libdmg-hfsplus project.
