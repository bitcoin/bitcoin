{ pkgs ? import <nixpkgs> {}
, crossPkgs ? null         # null means same as pkgs; overrides windows when set explicitly
, windows ? false          # Cross-compile for Windows using MinGW; implies crossPkgs = pkgs.pkgsCross.mingwW64
, enableLibcxx ? false # Whether to use libc++ toolchain and libraries instead of libstdc++
, minimal ? false # Whether to create minimal shell without extra tools (faster when cross compiling)
, capnprotoVersion ? null
, capnprotoSanitizers ? null # Optional sanitizers to build cap'n proto with
, cmakeVersion ? null
, libcxxSanitizers ? null # Optional LLVM_USE_SANITIZER value to use for libc++, see https://llvm.org/docs/CMake.html
}:

let
  lib  = pkgs.lib;
  effectiveCrossPkgs =
    if crossPkgs != null then crossPkgs
    else if windows then pkgs.pkgsCross.mingwW64
    else pkgs;
  llvmBase = pkgs.llvmPackages_21;
  llvm = llvmBase // lib.optionalAttrs (libcxxSanitizers != null) {
    libcxx = llvmBase.libcxx.override {
      devExtraCmakeFlags = [ "-DLLVM_USE_SANITIZER=${libcxxSanitizers}" ];
    };
  };
  capnprotoHashes = {
    "0.7.0" = "sha256-Y/7dUOQPDHjniuKNRw3j8dG1NI9f/aRWpf8V0WzV9k8=";
    "0.7.1" = "sha256-3cBpVmpvCXyqPUXDp12vCFCk32ZXWpkdOliNH37UwWE=";
    "0.8.0" = "sha256-rfiqN83begjJ9eYjtr21/tk1GJBjmeVfa3C3dZBJ93w=";
    "0.8.1" = "sha256-OZqNVYdyszro5rIe+w6YN00g6y8U/1b8dKYc214q/2o=";
    "0.9.0" = "sha256-yhbDcWUe6jp5PbIXzn5EoKabXiWN8lnS08hyfxUgEQ0=";
    "0.9.2" = "sha256-BspWOPZcP5nCTvmsDE62Zutox+aY5pw42d6hpH3v4cM=";
    "0.10.0" = "sha256-++F4l54OMTDnJ+FO3kV/Y/VLobKVRk461dopanuU3IQ=";
    "0.10.4" = "sha256-45sxnVyyYIw9i3sbFZ1naBMoUzkpP21WarzR5crg4X8=";
    "1.0.0" = "sha256-NLTFJdeOzqhk4ATvkc17Sh6g/junzqYBBEoXYGH/czo=";
    "1.0.2" = "sha256-LVdkqVBTeh8JZ1McdVNtRcnFVwEJRNjt0JV2l7RkuO8=";
    "1.1.0" = "sha256-gxkko7LFyJNlxpTS+CWOd/p9x/778/kNIXfpDGiKM2A=";
    "1.2.0" = "sha256-aDcn4bLZGq8915/NPPQsN5Jv8FRWd8cAspkG3078psc=";
  };
  capnprotoBase = if capnprotoVersion == null then effectiveCrossPkgs.capnproto else effectiveCrossPkgs.capnproto.overrideAttrs (old: {
    version = capnprotoVersion;
    src = effectiveCrossPkgs.fetchFromGitHub {
      owner = "capnproto";
      repo  = "capnproto";
      rev   = "v${capnprotoVersion}";
      hash  = lib.attrByPath [capnprotoVersion] "" capnprotoHashes;
    };
    patches = lib.optionals (lib.versionAtLeast capnprotoVersion "0.9.0" && lib.versionOlder capnprotoVersion "0.10.4") [ ./ci/patches/spaceship.patch ];
    cmakeFlags = (old.cmakeFlags or []) ++ (lib.optionals (lib.versionAtLeast "1.1.0" capnprotoVersion) ["-DCMAKE_POLICY_VERSION_MINIMUM=3.5"]);
  } // (lib.optionalAttrs (lib.versionOlder capnprotoVersion "0.10") {
    env = { }; # Drop -std=c++20 flag forced by nixpkgs
  }));
  capnproto = (capnprotoBase.overrideAttrs (old: lib.optionalAttrs (capnprotoSanitizers != null) {
    env = (old.env or { }) // {
      CXXFLAGS =
        lib.concatStringsSep " " [
          (old.env.CXXFLAGS or "")
          "-fsanitize=${capnprotoSanitizers}"
          "-fno-omit-frame-pointer"
          "-g"
        ];
    };
  } // lib.optionalAttrs windows {
    # Two CXXFLAGS additions needed for the MinGW cross-build:
    # - _WIN32_WINNT=0x0601: mcfgthread/fwd.h hard-errors if this isn't defined
    #   to at least Windows 7; it's pulled in transitively via <atomic> → gthr.h.
    # - Wno-class-memaccess: GCC promotes this to an error on the memset(&addr,0,…)
    #   call in kj/async-io-win32.c++; the memset intentionally zeroes a network
    #   address struct, so the warning is a false positive here.
    env = (old.env or { }) // {
      CXXFLAGS = "${old.env.CXXFLAGS or ""} -D_WIN32_WINNT=0x0601 -Wno-class-memaccess";
    };
    # Cross-compiling capnproto for Windows with the nixpkgs llvm-mingw toolchain
    # requires several cmake/nix workarounds:
    #
    # - BUILD_SHARED_LIBS=FALSE: static libs mean mptest.exe is self-contained,
    #   simplifying wine execution (no DLL search path needed).
    # - WITH_FIBERS=FALSE: kj fiber support on Windows/MinGW may not build.
    #
    # Two nix environment issues also need fixing:
    # 1. The clang wrapper uses GCC's C++ headers (libstdc++), which on this
    #    GCC version use the MCF thread model.  MCF headers are in a separate
    #    package (windows.mcfgthreads.dev) not in capnproto's default buildInputs.
    # 2. The clang wrapper's cc-ldflags is missing the GCC target lib directory
    #    that contains libgcc_s.a.  We add it in preConfigure.
    buildInputs = (old.buildInputs or []) ++ [
      effectiveCrossPkgs.windows.mcfgthreads.dev  # provides mcfgthread/gthr.h
      effectiveCrossPkgs.windows.mcfgthreads      # provides libmcfgthread.a for linking
    ];
    preConfigure = (old.preConfigure or "") + ''
      # The nixpkgs clang-mingw wrapper omits the GCC target lib directory
      # ($gcc/x86_64-w64-mingw32/lib) from its search path, so the linker
      # can't find libgcc_s.a even though the file exists.  Add it explicitly.
      # Must use 'export' so child processes (cmake, linker) inherit the value.
      export NIX_LDFLAGS_x86_64_w64_mingw32="''${NIX_LDFLAGS_x86_64_w64_mingw32:-} -L${effectiveCrossPkgs.buildPackages.gcc.cc}/x86_64-w64-mingw32/lib"
    '';
    cmakeFlags = (old.cmakeFlags or []) ++ [
      "-DBUILD_SHARED_LIBS=FALSE"
      "-DWITH_FIBERS=FALSE"
    ];
  })).override (lib.optionalAttrs enableLibcxx { clangStdenv = llvm.libcxxStdenv; }
             # Switch capnproto's cross-build to effectiveCrossPkgs.stdenv (GCC) instead of
             # the default clangStdenv. Clang with GCC's libstdc++.a causes MCF thread
             # symbol errors (_MCF_mutex_lock_slow etc.) because clang doesn't automatically
             # link GCC's MCF runtime. GCC knows its own runtime and links it correctly.
             // lib.optionalAttrs windows    { clangStdenv = effectiveCrossPkgs.stdenv; });
  clang = if enableLibcxx then llvm.libcxxClang else llvm.clang;
  clang-tools = llvm.clang-tools.override { inherit enableLibcxx; };
  cmakeHashes = {
    "3.12.4" = "sha256-UlVYS/0EPrcXViz/iULUcvHA5GecSUHYS6raqbKOMZQ=";
  };
  cmakeBuild = if cmakeVersion == null then pkgs.cmake else (pkgs.cmake.overrideAttrs (old: {
    version = cmakeVersion;
    src = pkgs.fetchurl {
      url = "https://cmake.org/files/v${lib.versions.majorMinor cmakeVersion}/cmake-${cmakeVersion}.tar.gz";
      hash = lib.attrByPath [cmakeVersion] "" cmakeHashes;
    };
    patches = [];
  })).override { isMinimalBuild = true; };
in effectiveCrossPkgs.mkShell {
  buildInputs = [
    capnproto
  ];
  nativeBuildInputs = with pkgs; [
    cmakeBuild
    include-what-you-use
    ninja
  ] ++ lib.optionals (!minimal) [
    clang
    clang-tools
  ] ++ lib.optionals windows [
    pkgs.capnproto             # native capnp + capnpc-c++ in PATH for cmake code generation
    pkgs.wine64Packages.staging # run cross-compiled mptest.exe in ctest
    pkgs.gcc                   # native C++ compiler for the native mpgen pre-build
    pkgs.openssl.dev           # native capnproto cmake config: find_dependency(OpenSSL) headers
    pkgs.openssl.out           # native capnproto cmake config: find_dependency(OpenSSL) libs
    pkgs.zlib.dev              # native capnproto cmake config: find_dependency(ZLIB) headers
    pkgs.zlib                  # native capnproto cmake config: find_dependency(ZLIB) libs
  ];

  # Tell IWYU where its libc++ mapping lives
  IWYU_MAPPING_FILE = if enableLibcxx then "${llvm.libcxx.dev}/include/c++/v1/libcxx.imp" else null;

  # When cross-compiling, expose native package prefixes so ci.sh can point
  # the native mpgen pre-build's CMAKE_PREFIX_PATH at them.  CMAKE_PREFIX_PATH
  # in the cross shell points at the Windows packages, not the native ones.
  NATIVE_CAPNPROTO_PREFIX = if windows then "${pkgs.capnproto}" else null;
  # OpenSSL and zlib are dependencies of the native capnproto cmake config
  # (find_dependency calls); the native cmake build needs to find them too.
  # FindOpenSSL.cmake needs both the dev output (headers) and out (libs).
  NATIVE_OPENSSL_DEV = if windows then "${pkgs.openssl.dev}" else null;
  NATIVE_OPENSSL_LIB = if windows then "${pkgs.openssl.out}" else null;
  NATIVE_ZLIB_DEV    = if windows then "${pkgs.zlib.dev}" else null;
  NATIVE_ZLIB_LIB    = if windows then "${pkgs.zlib}" else null;
  # GCC and MCF thread runtime DLL directories needed by wine to run mptest.exe.
  # libstdc++-6.dll and libgcc_s_seh-1.dll are in the GCC cross-compiler lib dir;
  # libmcfgthread-2.dll is in the mcfgthreads package's bin dir.
  WIN_RUNTIME_DLLS = if windows then
    "${effectiveCrossPkgs.buildPackages.gcc.cc.lib}/x86_64-w64-mingw32/lib:${effectiveCrossPkgs.windows.mcfgthreads}/bin"
    else null;
}
