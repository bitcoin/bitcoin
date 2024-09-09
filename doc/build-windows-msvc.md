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
```
git clone https://github.com/bitcoin/bitcoin.git
```


## Triplets and Presets

The Bitcoin Core project supports the following vcpkg triplets:
- `x64-windows` (both CRT and library linkage is dynamic)
- `x64-windows-static` (both CRT and library linkage is static)

To facilitate build process, the Bitcoin Core project provides presets, which are used in this guide.

Available presets can be listed as follows:
```
cmake --list-presets
```

By default, all presets:
- Set `BUILD_GUI` to `ON`.
- Set `WITH_QRENCODE` to `OFF`, due to known build issues when using vcpkg's `libqrencode` package.

## Building

CMake will put the resulting object files, libraries, and executables into a dedicated build directory.

In the following instructions, the "Debug" configuration can be specified instead of the "Release" one.

### 4. Building with Static Linking with GUI

```
cmake -B build --preset vs2022-static          # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Use "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
cmake --install build --config Release         # Optional.
```

### 5. Building with Dynamic Linking without GUI

```
cmake -B build --preset vs2022 -DBUILD_GUI=OFF # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Use "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
```

## Performance Notes

### 6. vcpkg Manifest Default Features

One can skip vcpkg manifest default features to speedup the configuration step.
For example, the following invocation will skip all features except for "wallet" and "tests" and their dependencies:
```
cmake -B build --preset vs2022 -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON -DVCPKG_MANIFEST_FEATURES="wallet;tests" -DBUILD_GUI=OFF
```

Available features are listed in the [`vcpkg.json`](/vcpkg.json) file.

### 7. Antivirus Software

To improve the build process performance, one might add the Bitcoin repository directory to the Microsoft Defender Antivirus exclusions.
