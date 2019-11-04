GENERIC BUILD NOTES
====================
Some notes on how to build Dash Core based on the [depends](../depends/README.md) build system.

Note on old build instructions
------------------------------
In the past, the build documentation contained instructions on how to build Dash with system-wide installed dependencies
like BerkeleyDB 4.8, boost and Qt. Building this way is considered deprecated and only building with the `depends` prefix
is supported today.

Required build tools and environment
------------------------------------
Building the dependencies and Dash Core requires some essential build tools to be installed before. Please see
[build-unix](build-unix.md), [build-osx](build-osx.md) and [build-windows](build-windows.md) for details.

Building dependencies
---------------------
Dash inherited the `depends` folder from Bitcoin, which contains all dependencies required to build Dash. These
dependencies must be built before Dash can actually be built. To do so, perform the following:

```bash
$ cd depends
$ make -j4 # Choose a good -j value, depending on the number of CPU cores available
$ cd ..
```

This will download and build all dependencies required to build Dash Core. Caching of build results will ensure that only
the packages are rebuilt which have changed since the last depends build.

It is required to re-run the above commands from time to time when dependencies have been updated or added. If this is
not done, build failures might occur when building Dash.

Please read the [depends](../depends/README.md) documentation for more details on supported hosts and configuration
options. If no host is specified (as in the above example) when calling `make`, the depends system will default to your
local host system. 

Building Dash Core
---------------------

```bash
$ ./autogen.sh
$ ./configure --prefix=`pwd`/depends/<host>
$ make
$ make install # optional
```

Please replace `<host>` with your local system's `host-platform-triplet`. The following triplets are usually valid:
- `i686-pc-linux-gnu` for Linux32
- `x86_64-pc-linux-gnu` for Linux64
- `i686-w64-mingw32` for Win32
- `x86_64-w64-mingw32` for Win64
- `x86_64-apple-darwin11` for MacOSX
- `arm-linux-gnueabihf` for Linux ARM 32 bit
- `aarch64-linux-gnu` for Linux ARM 64 bit

If you want to cross-compile for another platform, choose the appropriate `<host>` and make sure to build the
dependencies with the same host before.

If you want to build for the same host but different distro, add `--enable-glibc-back-compat` when calling `./configure`.


ccache
------
`./configure` of Dash Core will autodetect the presence of ccache and enable use of it. To disable ccache, use
`./configure --prefix=<prefix> --disable-ccache`. When installed and enabled, [ccache](https://ccache.samba.org/) will
cache build results on source->object level.

The default maximum cache size is 5G, which might not be enough to cache multiple builds when switching Git branches
very often. It is advised to increase the maximum cache size:

```bash
$ ccache -M20G
```

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

```bash
./configure --help
```
