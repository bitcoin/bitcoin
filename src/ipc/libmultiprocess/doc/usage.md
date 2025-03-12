# libmultiprocess Usage

## Overview

_libmultiprocess_ is a library and code generator that allows calling C++ class interfaces across different processes. For an interface to be available from other processes, it needs two definitions:

- An **API definition** declaring how the interface is called. Included examples: [calculator.h](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/calculator.h), [printer.h](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/printer.h), [init.h](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/init.h). Bitcoin examples: [node.h](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/interfaces/node.h), [wallet.h](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/interfaces/wallet.h), [echo.h](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/interfaces/echo.h), [init.h](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/interfaces/init.h).

- A **data definition** declaring how interface calls get sent across the wire. Included examples: [calculator.capnp](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/calculator.capnp), [printer.capnp](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/printer.capnp), [init.capnp](https://github.com/bitcoin-core/libmultiprocess/blob/master/example/init.capnp). Bitcoin examples: [node.capnp](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/ipc/capnp/node.capnp), [wallet.capnp](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/ipc/capnp/wallet.capnp), [echo.capnp](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/ipc/capnp/echo.capnp), [init.capnp](https://github.com/ryanofsky/bitcoin/blob/ipc-export/src/ipc/capnp/init.capnp).

The `*.capnp` data definition files are consumed by the _libmultiprocess_ code generator and each `X.capnp` file generates `X.capnp.c++`, `X.capnp.h`, `X.capnp.proxy-client.c++`, `X.capnp.proxy-server.c++`, `X.capnp.proxy-types.c++`, `X.capnp.proxy-types.h`, and `X.capnp.proxy.h` output files. The generated files include `mp::ProxyClient<Interface>` and `mp::ProxyServer<Interface>` class specializations for all the interfaces in the `.capnp` files. These allow methods on C++ objects in one process to be called from other processes over IPC sockets.

The `ProxyServer` objects help translate IPC requests from a socket to method calls on a local object. The `ProxyServer` objects are just used internally by the `mp::ServeStream(loop, socket, wrapped_object)` and `mp::ListenConnections(loop, socket, wrapped_object)` functions, and aren't exposed externally. The `ProxyClient` classes are exposed, and returned from the `mp::ConnectStream(loop, socket)` function and meant to be used directly. The classes implement methods described in `.capnp` definitions, and whenever any method is called, a request with the method arguments is sent over the associated IPC connection, and the corresponding `wrapped_object` method on the other end of the connection is called, with the `ProxyClient` method blocking until it returns and forwarding back any return value to the `ProxyClient` method caller.

## Example

A simple interface description can be found at [test/mp/test/foo.capnp](../test/mp/test/foo.capnp), implementation in [test/mp/test/foo.h](../test/mp/test/foo.h), and usage in [test/mp/test/test.cpp](../test/mp/test/test.cpp).

A more complete example can be found in [example](../example/) and run with:

```sh
make -C build example
build/example/mpexample
```
