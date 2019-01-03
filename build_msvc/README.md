Building Bitcoin Core with Visual Studio
========================================

Introduction
---------------------
Solution and project files to build the Bitcoin Core applications (except Qt dependent ones) with Visual Studio 2017 can be found in the build_msvc directory.

Building with Visual Studio is an alternative to the Linux based [cross-compiler build](https://github.com/bitcoin/bitcoin/blob/master/doc/build-windows.md).

Dependencies
---------------------
A number of [open source libraries](https://github.com/bitcoin/bitcoin/blob/master/doc/dependencies.md) are required in order to be able to build Bitcoin.

Options for installing the dependencies in a Visual Studio compatible manner are:

- Use Microsoft's [vcpkg](https://docs.microsoft.com/en-us/cpp/vcpkg) to download the source packages and build locally. This is the recommended approach.
- Download the source code, build each dependency, add the required include paths, link libraries and binary tools to the Visual Studio project files.
- Use [nuget](https://www.nuget.org/) packages with the understanding that any binary files have been compiled by an untrusted third party.

The external dependencies required for the Visual Studio build are (see the [dependencies doc](https://github.com/bitcoin/bitcoin/blob/master/doc/dependencies.md) for versions):

- Berkeley DB,
- OpenSSL,
- Boost,
- libevent,
- ZeroMQ

Additional dependencies required from the [bitcoin-core](https://github.com/bitcoin-core) github repository are:
- SECP256K1,
- LevelDB

Building
---------------------
The instructions below use `vcpkg` to install the dependencies.

- Clone `vcpkg` from the [github repository](https://github.com/Microsoft/vcpkg) and install as per the instructions in the main README.md.
- Install the required packages (replace x64 with x86 as required):

```
    PS >.\vcpkg install --triplet x64-windows-static boost-filesystem boost-signals2 boost-test libevent openssl zeromq berkeleydb secp256k1 leveldb
```

- Use Python to generate *.vcxproj from Makefile

```
    PS >python msvc-autogen.py
```

- Build in Visual Studio.
