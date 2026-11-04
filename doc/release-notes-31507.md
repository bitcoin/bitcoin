Build System
------------

- Support for building with MSVC (`cl.exe`) on Windows has been dropped.
  Building natively on Windows is now supported only with Clang
  (`clang-cl.exe`), which remains compatible with the MSVC toolchain,
  including the Visual Studio build environment, the MSVC standard
  library and runtime, and vcpkg-provided dependencies. (#31507)

  Clang is preferred over MSVC for the following reasons:
  - MSVC does not support inline assembly on x64 and ARM64, which
    precludes some hardware-accelerated implementations.
  - MSVC has repeatedly caused maintenance overhead due to internal
    compiler errors that required workarounds not needed for any
    other supported compiler.
