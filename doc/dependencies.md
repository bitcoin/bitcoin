Dependencies
============

These are the dependencies currently used by Bitcoin Core. You can find instructions for installing them in the `build-*.md` file for your platform.

| Dependency | Version used | Minimum required | CVEs | Shared | [Bundled Qt library](https://doc.qt.io/qt-5/configure-options.html) |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [4.8.30](http://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| Boost | [1.64.0](https://www.boost.org/users/download/) | [1.47.0](https://github.com/bitcoin/bitcoin/pull/8920) | No |  |  |
| Clang |  | [3.3+](https://llvm.org/releases/download.html) (C++11 support) |  |  |  |
| D-Bus | [1.10.18](https://cgit.freedesktop.org/dbus/dbus/tree/NEWS?h=dbus-1.10) |  | No | Yes |  |
| Expat | [2.2.5](https://libexpat.github.io/) |  | No | Yes |  |
| fontconfig | [2.12.1](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.7.1](http://download.savannah.gnu.org/releases/freetype) |  | No |  |  |
| GCC |  | [4.8+](https://gcc.gnu.org/) (C++11 support) |  |  |  |
| HarfBuzz-NG |  |  |  |  |  |
| libevent | [2.1.8-stable](https://github.com/libevent/libevent/releases) | 2.0.22 | No |  |  |
| libjpeg |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L65) |
| libpng |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L64) |
| MiniUPnPc | [2.0.20180203](http://miniupnp.free.fr/files) |  | No |  |  |
| OpenSSL | [1.0.1k](https://www.openssl.org/source) |  | Yes |  |  |
| PCRE |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L66) |
| protobuf | [2.6.1](https://github.com/google/protobuf/releases) |  | No |  |  |
| Python (tests) |  | [3.4](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [5.9.6](https://download.qt.io/official_releases/qt/) | 5.x | No |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L87) (Linux only) |
| xkbcommon |  |  |  |  | [Yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L86) (Linux only) |
| ZeroMQ | [4.2.5](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib | [1.2.11](https://zlib.net/) |  |  |  | No |
