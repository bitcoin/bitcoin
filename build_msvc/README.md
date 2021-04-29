Building Syscoin Core with Visual Studio
========================================

Introduction
---------------------
Solution and project files to build the Syscoin Core applications `msbuild` or Visual Studio can be found in the `build_msvc` directory. The build has been tested with Visual Studio 2017 and 2019.

Building with Visual Studio is an alternative to the Linux based [cross-compiler build](https://github.com/syscoin/syscoin/blob/master/doc/build-windows.md).

Quick Start
---------------------
The minimal steps required to build Syscoin Core with the msbuild toolchain are below. More detailed instructions are contained in the following sections.

```
cd build_msvc
py -3 msvc-autogen.py
msbuild /m syscoin.sln /p:Platform=x64 /p:Configuration=Release /t:build
```

Dependencies
---------------------
A number of [open source libraries](https://github.com/syscoin/syscoin/blob/master/doc/dependencies.md) are required in order to be able to build Syscoin Core.

Options for installing the dependencies in a Visual Studio compatible manner are:

- Use Microsoft's [vcpkg](https://docs.microsoft.com/en-us/cpp/vcpkg) to download the source packages and build locally. This is the recommended approach.
- Download the source code, build each dependency, add the required include paths, link libraries and binary tools to the Visual Studio project files.
- Use [nuget](https://www.nuget.org/) packages with the understanding that any binary files have been compiled by an untrusted third party.

The [external dependencies](https://github.com/syscoin/syscoin/blob/master/doc/dependencies.md) required for building are listed in the `build_msvc/vcpkg.json` file. To ensure `msbuild` project files automatically install the `vcpkg` dependencies use:

```
vcpkg integrate install
```

Qt
---------------------
In order to build the Syscoin Core a static build of Qt is required. The runtime library version (e.g. v141, v142) and platform type (x86 or x64) must also match.

Some prebuilt x64 versions of Qt can be downloaded from [here](https://github.com/syscoin/syscoin/releases). Please be aware these downloads are NOT officially sanctioned by Syscoin Core and are provided for developer convenience only. They should NOT be used for builds that will be used in a production environment or with real funds.


To build Syscoin Core without Qt unload or disable the `syscoin-qt`, `libsyscoin_qt` and `test_syscoin-qt` projects.

Building
---------------------
The instructions below use `vcpkg` to install the dependencies.

- Install [`vcpkg`](https://github.com/Microsoft/vcpkg).

- Use Python to generate `*.vcxproj` from Makefile

```
PS >py -3 msvc-autogen.py
```

- An optional step is to adjust the settings in the `build_msvc` directory and the `common.init.vcxproj` file. This project file contains settings that are common to all projects such as the runtime library version and target Windows SDK version. The Qt directories can also be set.

- To build from the command line with the Visual Studio 2017 toolchain use:

```
msbuild /m syscoin.sln /p:Platform=x64 /p:Configuration=Release /p:PlatformToolset=v141 /t:build
```

- To build from the command line with the Visual Studio 2019 toolchain use:

```
msbuild /m syscoin.sln /p:Platform=x64 /p:Configuration=Release /t:build
```

- Alternatively open the `build_msvc/bitcoin.sln` file in Visual Studio.

AppVeyor
---------------------
The .appveyor.yml in the root directory is suitable to perform builds on [AppVeyor](https://www.appveyor.com/) Continuous Integration servers. The simplest way to perform an AppVeyor build is to fork Syscoin Core and then configure a new AppVeyor Project pointing to the forked repository.

For safety reasons the Syscoin Core .appveyor.yml file has the artifact options disabled. The build will be performed but no executable files will be available. To enable artifacts on a forked repository uncomment the lines shown below:

```
    #- 7z a syscoin-%APPVEYOR_BUILD_VERSION%.zip %APPVEYOR_BUILD_FOLDER%\build_msvc\%platform%\%configuration%\*.exe
    #- path: syscoin-%APPVEYOR_BUILD_VERSION%.zip
```

Security
---------------------
[Base address randomization](https://docs.microsoft.com/en-us/cpp/build/reference/dynamicbase-use-address-space-layout-randomization?view=msvc-160) is used to make Bitcoin Core more secure. When building Bitcoin using the `build_msvc` process base address randomization can be disabled by editing `common.init.vcproj` to change `RandomizedBaseAddress` from `true` to `false` and then rebuilding the project.

To check if `bitcoind` has `RandomizedBaseAddress` enabled or disabled run

```
.\dumpbin.exe /headers src/bitcoind.exe
```

If is it enabled then in the output `Dynamic base` will be listed in the `DLL characteristics` under `OPTIONAL HEADER VALUES` as shown below

```
            8160 DLL characteristics
                   High Entropy Virtual Addresses
                   Dynamic base
                   NX compatible
                   Terminal Server Aware
```

This may not disable all stack randomization as versions of windows employ additional stack randomization protections. These protections must be turned off in the OS configuration.