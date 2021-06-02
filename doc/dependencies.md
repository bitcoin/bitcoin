Dependencies
============

These are the dependencies currently used by Syscoin Core. You can find instructions for installing them in the `build-*.md` file for your platform.

| Dependency | Version used | Minimum required | CVEs | Shared | [Bundled Qt library](https://doc.qt.io/qt-5/configure-options.html#third-party-libraries) |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [4.8.30](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| Boost | [1.71.0](https://www.boost.org/users/download/) | [1.58.0](https://github.com/bitcoin/bitcoin/pull/19667) | No |  |  |
| Clang |  | [5.0+](https://releases.llvm.org/download.html) (C++17 support) |  |  |  |
| Expat | [2.2.7](https://libexpat.github.io/) |  | No | Yes |  |
| fontconfig | [2.12.1](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.7.1](https://download.savannah.gnu.org/releases/freetype) |  | No |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Android only) |
| GCC |  | [7+](https://gcc.gnu.org/) (C++17 support) |  |  |  |
| HarfBuzz-NG |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| libevent | [2.1.12-stable](https://github.com/libevent/libevent/releases) | [2.0.21](https://github.com/bitcoin/bitcoin/pull/18676) | No |  |  |
| libnatpmp | git commit [4536032...](https://github.com/miniupnp/libnatpmp/tree/4536032ae32268a45c073a4d5e91bbab4534773a) |  | No |  |  |
| libpng |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| librsvg | |  |  |  |  |
| MiniUPnPc | [2.2.2](https://miniupnp.tuxfamily.org/files) |  | No |  |  |
| PCRE |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |
| Python (tests) |  | [3.6](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [5.12.11](https://download.qt.io/official_releases/qt/) | [5.9.5](https://github.com/bitcoin/bitcoin/issues/20104) | No |  |  |
| SQLite | [3.32.1](https://sqlite.org/download.html) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) |  |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Linux only) |
| xkbcommon |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) (Linux only) |
| ZeroMQ | [4.3.1](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk) |

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
* ZeroMQ is needed only with the `--with-zmq` option.

#### Other
* librsvg is only needed if you need to run `make deploy` on (cross-compilation to) macOS.
* Not-Qt-bundled zlib is required to build the [DMG tool](../contrib/macdeploy/README.md#deterministic-macos-dmg-notes) from the libdmg-hfsplus project.
