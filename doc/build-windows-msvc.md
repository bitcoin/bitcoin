# Windows / MSVC Build Guide

This guide describes how to build bitcoind, command-line utilities, and GUI on Windows using Microsoft Visual Studio.

For cross-compiling options, please see [`build-windows.md`](./build-windows.md).

## Preparation

### 1. Install Required Dependencies

The first step is to install the required build applications. The instructions below use WinGet to install the applications.

WinGet is available on all supported Windows versions. The applications mentioned can also be installed manually.

#### Visual Studio

This guide relies on using CMake and vcpkg package manager provided with the Visual Studio installation.

Minimum required version: Visual Studio 2026 version 18.3 with the "Desktop development with C++" workload.

To install Visual Studio Community Edition with the necessary components, run:

```powershell
winget install --id Microsoft.VisualStudio.Community --override "--wait --quiet --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.Git --includeRecommended"
```

This installs:
- Visual Studio
- The "Desktop development with C++" workload (NativeDesktop)
- Git component

After installation, the commands in this guide should be executed in "Developer PowerShell for VS" or "Developer Command Prompt for VS".
The former is assumed hereinafter.

#### Python

Python is required for running the test suite.

To install Python, run:

```powershell
winget install python3
```

### 2. Clone Bitcoin Repository

`git` should already be installed as a component of Visual Studio. If not, download and install [Git for Windows](https://git-scm.com/downloads/win).

Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.

```powershell
git clone https://github.com/bitcoin/bitcoin.git
```


## Triplets and Presets

The Bitcoin Core project supports the following vcpkg triplets:
- `x64-windows` (both CRT and library linkage is dynamic)
- `x64-windows-static` (both CRT and library linkage is static)

To facilitate build process, the Bitcoin Core project provides presets, which are used in this guide.

Available presets can be listed as follows:
```powershell
cmake --list-presets
```

By default, all presets set `BUILD_GUI` to `ON`.

## Building

CMake will put the resulting object files, libraries, and executables into a dedicated build directory.

In the following instructions, the "Debug" configuration can be specified instead of the "Release" one.

Run `cmake -B build -LH` to see the full list of available options.

### Building with Static Linking with GUI

```powershell
cmake -B build --preset vs2026-static          # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Append "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Append "-j N" for N parallel tests.
cmake --install build --config Release         # Optional.
```

### Building with Dynamic Linking without GUI

```powershell
cmake -B build --preset vs2026 -DBUILD_GUI=OFF # It might take a while if the vcpkg binary cache is unpopulated or invalidated.
cmake --build build --config Release           # Append "-j N" for N parallel jobs.
ctest --test-dir build --build-config Release  # Append "-j N" for N parallel tests.
```

### vcpkg-specific Issues and Workarounds

vcpkg installation during the configuration step might fail for various reasons unrelated to Bitcoin Core.

If the failure is due to a "Buildtrees path … is too long" error, which is often encountered when building
with `BUILD_GUI=ON` and using the default vcpkg installation provided by Visual Studio, you can
specify a shorter path to store intermediate build files by using
the [`--x-buildtrees-root`](https://learn.microsoft.com/en-us/vcpkg/commands/common-options#buildtrees-root) option:

```powershell
cmake -B build --preset vs2026-static -DVCPKG_INSTALL_OPTIONS="--x-buildtrees-root=C:\vcpkg"
```

If vcpkg installation fails with the message "Paths with embedded space may be handled incorrectly", which
can occur if your local Bitcoin Core repository path contains spaces, you can override the vcpkg install directory
by setting the [`VCPKG_INSTALLED_DIR`](https://github.com/microsoft/vcpkg-docs/blob/main/vcpkg/users/buildsystems/cmake-integration.md#vcpkg_installed_dir) variable:

```powershell
cmake -B build --preset vs2026-static -DVCPKG_INSTALLED_DIR="C:\path_without_spaces"
```

## Performance Notes

### vcpkg Manifest Default Features

One can skip vcpkg manifest default features to speed up the configuration step.
For example, the following invocation will skip all features except for "wallet" and "tests" and their dependencies:
```powershell
cmake -B build --preset vs2026 -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON -DVCPKG_MANIFEST_FEATURES="wallet;tests" -DBUILD_GUI=OFF -DWITH_ZMQ=OFF
```

Available features are listed in the [`vcpkg.json`](/vcpkg.json) file.

### Antivirus Software

To improve the build process performance, one might add the Bitcoin repository directory to the Microsoft Defender Antivirus exclusions.
