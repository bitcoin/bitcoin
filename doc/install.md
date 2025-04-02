# libmultiprocess Installation

Installation currently requires Cap'n Proto:

```sh
apt install libcapnp-dev capnproto
brew install capnp cmake
dnf install capnproto
```

Installation steps are:

```sh
mkdir build
cd build
cmake ..
make
make check # Optionally build and run tests
make install
```

To build with libmultiprocess in a CMake project can specify:

```cmake
find_package(Libmultiprocess)
target_capnp_sources(mytarget ${CMAKE_CURRENT_SOURCE_DIR} myschema.capnp)
```

Which will locate the libmultiprocess cmake package, and call the
`target_capnp_sources` function to generate C++ files and link them into a
library or executable target. See `example/CMakeLists.txt` for a complete
example.

To build with libmultiprocess in a non-CMake project can use installed
`<prefix>/include/mpgen.mk` Makefile rule to generate C++ files, and
`<prefix>/lib/pkgconfig/libmultiprocess.pc` pkg-config definition to link
against the runtime library.

For cross-compilation, it may be useful to build the runtime library and code
generation binaries separately, which can be done with:

```sh
make install-bin # install bin/mpgen and related files
make install-lib # install lib/libmultiprocess.a and related files
```

It is also possible to import CMake targets separately with:

```cmake
find_package(Libmultiprocess COMPONENTS Bin)
find_package(Libmultiprocess COMPONENTS Lib)
```
