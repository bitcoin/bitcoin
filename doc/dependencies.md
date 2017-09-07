Dependencies
============

These are the dependencies currently used by Bitcoin Core. You can find instructions for installing them in the `build-*.md` file for your platform.

| Dependency | Version used | Minimum Required | CVEs? | Shared | [Bundled Qt Library](https://doc.qt.io/qt-5/configure-options.html) |
| --- | --- | --- | --- | --- | --- | --- |
| openssl | [1.0.1k]](https://www.openssl.org/source) |  | Yes |  |  |
| ccache | [3.3.4](https://ccache.samba.org/download.html) |  | No |  |  |
| libevent | [2.1.8-stable](https://github.com/libevent/libevent/releases) | 2.0.22 | No |  |  |
| Qt | [5.7.1](https://download.qt.io/official_releases/qt/) | 4.7+ | No |  |  |
| Freetype | [2.7.1](http://download.savannah.gnu.org/releases/freetype) |  | No |  |  |
| Boost | [1.64.0](http://www.boost.org/users/download/) | [1.47.0](https://github.com/bitcoin/bitcoin/pull/8920) | No |  |  |
| Protobuf | [2.6.3](https://github.com/google/protobuf/releases) |  | No |  |  |
| Zeromq | [4.1.5](https://github.com/zeromq/libzmq/releases) |  | No |  |  |
| miniupnpc | [2.0.20170509](http://miniupnp.free.fr/files) |  | No |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| berkeley-db | [4.8.30](http://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| dbus | [1.10.18](https://cgit.freedesktop.org/dbus/dbus/tree/NEWS?h=dbus-1.10) |  | No | yes |  |
| expat | [2.2.1](https://libexpat.github.io/) |  | No | yes |  |
| fontconfig | [2.12.1](https://www.freedesktop.org/software/fontconfig/release/) |  | No | yes |  |
| freetype |  |  |  |  | [no](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L38) (linux uses system) |
| zlib | [1.2.11](http://zlib.net/) |  |  |  | no |
| libjpeg |  |  |  |  | [yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L75) |
| libpng |  |  |  |  | [yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L74) |
| PCRE |  |  |  |  | [yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L76) |
| xcb |  |  |  |  | [yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L94) (linux only) |
| xkbcommon |  |  |  |  | [yes](https://github.com/bitcoin/bitcoin/blob/master/depends/packages/qt.mk#L93) (linux only) |
| HarfBuzz-NG |  |  |  |  | ? |
| Python (tests) |  | [3.4](https://www.python.org/downloads) |  |  |  |
| GCC |  | [4.7+](https://gcc.gnu.org/) |  |  |  |
| Clang |  | [3.3+](http://llvm.org/releases/download.html) (C++11 support) |  |  |  |
