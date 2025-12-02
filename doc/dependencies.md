# Dependencies

These are the dependencies used by Bitcoin Core.
You can find installation instructions in the `/doc/build-*.md` file for your platform, or self-compile
them using [depends](/depends/README.md).

## Compiler

Bitcoin Core requires one of the following compilers.

| Dependency | Minimum required |
| --- | --- |
| [Clang](https://clang.llvm.org) | [17.0](https://github.com/bitcoin/bitcoin/pull/33555) |
| [GCC](https://gcc.gnu.org) | [12.1](https://github.com/bitcoin/bitcoin/pull/33842) |

## Required

### Build

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.73.0](https://github.com/bitcoin/bitcoin/pull/29066) |
| CMake | [link](https://cmake.org/) | [3.22](https://github.com/bitcoin/bitcoin/pull/30454) |
| [libevent](../depends/packages/libevent.mk) | [link](https://github.com/libevent/libevent/releases) | [2.1.8](https://github.com/bitcoin/bitcoin/pull/24681) |

### Runtime

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| glibc | [link](https://www.gnu.org/software/libc/) | [2.31](https://github.com/bitcoin/bitcoin/pull/29987)

## Optional

### Build

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| [Cap'n Proto](../depends/packages/capnp.mk) | [link](https://capnproto.org) | [0.7.1](https://github.com/bitcoin/bitcoin/pull/28907) |
| Python (scripts, tests) | [link](https://www.python.org) | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) |
| [Qt](../depends/packages/qt.mk) (gui) | [link](https://download.qt.io/archive/qt/) | [6.2](https://github.com/bitcoin/bitcoin/pull/30997) |
| [qrencode](../depends/packages/qrencode.mk) (gui) | [link](https://fukuchi.org/works/qrencode/) | N/A |
| [SQLite](../depends/packages/sqlite.mk) (wallet) | [link](https://sqlite.org) | [3.7.17](https://github.com/bitcoin/bitcoin/pull/19077) |
| [systemtap](../depends/packages/systemtap.mk) ([tracing](tracing.md)) | [link](https://sourceware.org/systemtap/) | N/A |
| [ZeroMQ](../depends/packages/zeromq.mk) (notifications) | [link](https://github.com/zeromq/libzmq/releases) | 4.0.0 |

### Runtime

| Dependency | Releases | Minimum required |
| --- | --- | --- |
| [Fontconfig](../depends/packages/fontconfig.mk) (gui) | [link](https://www.freedesktop.org/wiki/Software/fontconfig/) | 2.6 |
| [FreeType](../depends/packages/freetype.mk) (gui) | [link](https://freetype.org) | 2.3.0 |
