# Windows / MSVC Build Guide

This guide describes how to build bitcoind, command-line utilities, and GUI on Windows using Microsoft Visual Studio.

For cross-compiling options, please see [`build-windows.md`](./build-windows.md).

## Preparation

### 1. Visual Studio

This guide relies on using CMake and vcpkg package manager provided with the Visual Studio installation.
Here are requirements for the Visual Studio installation:
1. Minimum required version: Visual Studio 2022 version 17.6.
2. Installed components:
   - The "Desktop development with C++" workload.

The commands in this guide should be executed in "Developer PowerShell for VS 2022" or "Developer Command Prompt for VS 2022".
The former is assumed hereinafter.

### 2. Git

Download and install [Git for Windows](https://git-scm.com/download/win). Once installed, Git is available from PowerShell or the Command Prompt.

### 3. Clone Bitcoin Repository

Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.

```sh
git clone https://github.com/bitcoin/bitcoin.git
```

## Triplets and Presets

The Bitcoin Core project supports the following vcpkg triplets:
- `x64-windows` (both CRT and library linkage is dynamic)
- `x64-windows-static` (both CRT and library linkage is static)

To facilitate build process, the Bitcoin Core project provides presets, which are used in this guide.

Available presets can be listed as follows:

```sh
cmake --list-presets
```

By default, all presets set `BUILD_GUI` to `ON`.

## Building

CMake will put the resulting object files, libraries, and executables into a dedicated build directory.

In the following instructions, the "Debug" configuration can be specified instead of the "Release" one.

### 4. Building with Static Linking with GUI

```sh
cmake -B build --preset vs2022-static          # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Use "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
cmake --install build --config Release         # Optional.
```

### 5. Building with Dynamic Linking without GUI

```sh
cmake -B build --preset vs2022 -DBUILD_GUI=OFF # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Use "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
```

## Performance Notes

### 6. Build Caching with ccache

To speed up rebuilds using `ccache`:

1. **Install Prerequisites**
   
   ```sh
   # Install ninja generator (required for ccache)
   choco install ninja
   # Install ccache
   choco install ccache
   ```

2. **Configure with ccache**
   
   ```sh
   cmake -B build -G Ninja -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

3. **Set Debug Flags for Caching**
   
   ```sh
   cmake -B build -DCMAKE_CXX_FLAGS_DEBUG="/Z7" # Required for ccache compatibility
   ```
   
   - **Note:** Avoid using `/Zi` as it breaks [ccache's hashing mechanism](https://ccache.dev/manual/latest.html#_msvc_specific_notes)

4. **Build and Verify**
   
   ```sh
   cmake --build build --config Release
   ccache -s # Check cache statistics
   ```

### 7. vcpkg Manifest Default Features

One can skip vcpkg manifest default features to speed up the configuration step.
For example, the following invocation will skip all features except for "wallet" and "tests" and their dependencies:

```sh
cmake -B build --preset vs2022 -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON -DVCPKG_MANIFEST_FEATURES="wallet;tests" -DBUILD_GUI=OFF
```

Available features are listed in the [`vcpkg.json`](/vcpkg.json) file.

### 8. Antivirus Software

To improve the build process performance, one might add the Bitcoin repository directory to the Microsoft Defender Antivirus exclusions.


