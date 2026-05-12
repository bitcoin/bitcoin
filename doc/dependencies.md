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
| [Boost](../depends/packages/boost.mk) | [link](https://www.boost.org/users/download/) | [1.74.0](https://github.com/bitcoin/bitcoin/pull/34107) |
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
| [Cap'n Proto](../depends/packages/native_capnp.mk) ([IPC](multiprocess.md)) | [link](https://capnproto.org/) | [0.7.1](https://github.com/bitcoin/bitcoin/pull/33241) |
| [libmultiprocess](../depends/packages/native_libmultiprocess.mk) ([IPC](multiprocess.md)) | [link](https://github.com/bitcoin-core/libmultiprocess/tags) | [v7.0-pre1](https://github.com/bitcoin/bitcoin/pull/33517) |
| Python (scripts, tests) | [link](https://www.python.org) | [3.10](https://github.com/bitcoin/bitcoin/pull/30527) |
| [systemtap](../depends/packages/systemtap.mk) ([tracing](tracing.md)) | [link](https://sourceware.org/systemtap/) | N/A |
| [ZeroMQ](../depends/packages/zeromq.mk) (notifications) | [link](https://github.com/zeromq/libzmq/releases) | 4.0.0 |

