Building Bitcoin Core with Visual Studio
========================================

Introduction
---------------------
Visual Studio 2022 is minimum required to build Bitcoin Core.

Solution and project files to build with `msbuild` or Visual Studio can be found in the `build_msvc` directory.

To build Bitcoin Core from the command-line, it is sufficient to only install the [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) component.

The "Desktop development with C++" workload must be installed as well.

Building with Visual Studio is an alternative to the Linux based [cross-compiler build](../doc/build-windows.md).


Prerequisites
---------------------
To build [dependencies](../doc/dependencies.md) (except for [Qt](#qt)),
the default approach is to use the [vcpkg](https://vcpkg.io) package manager from Microsoft:

1. [Install](https://vcpkg.io/en/getting-started.html) vcpkg.

2. By default, vcpkg makes both `release` and `debug` builds for each package.
To save build time and disk space, one could skip `debug` builds (example uses PowerShell):
```powershell

Add-Content -Path "vcpkg\triplets\x64-windows-static.cmake" -Value "set(VCPKG_BUILD_TYPE release)"
```

Qt
---------------------
To build Bitcoin Core with the GUI, a static build of Qt is required.

1. Download a single ZIP archive of Qt source code from https://download.qt.io/official_releases/qt/ (e.g., [`qt-everywhere-opensource-src-5.15.11.zip`](https://download.qt.io/official_releases/qt/5.15/5.15.11/single/qt-everywhere-opensource-src-5.15.11.zip)), and expand it into a dedicated folder. The following instructions assume that this folder is `C:\dev\qt-source`.

> 💡 **Tip:** If you use the default path with "Extract All" for the Qt source code zip file, and end up with something like `C:\dev\qt-everywhere-opensource-src-5.15.11\qt-everywhere-src-5.15.11`, you are likely to encounter a "path too long" error when building. To fix the problem move the source files to a shorter path such as the recommended `C:\dev\qt-source`.

2. Open "x64 Native Tools Command Prompt for VS 2022", and input the following commands:
```cmd
cd C:\dev\qt-source
mkdir build
cd build
..\configure -release -silent -opensource -confirm-license -opengl desktop -static -static-runtime -mp -qt-zlib -qt-pcre -qt-libpng -nomake examples -nomake tests -nomake tools -no-angle -no-dbus -no-gif -no-gtk -no-ico -no-icu -no-libjpeg -no-libudev -no-sql-sqlite -no-sql-odbc -no-sqlite -no-vulkan -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip doc -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtlottie -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquick3d -skip qtquickcontrols -skip qtquickcontrols2 -skip qtquicktimeline -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtsvg -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview -skip qtx11extras -skip qtxmlpatterns -no-openssl -no-feature-bearermanagement -no-feature-printdialog -no-feature-printer -no-feature-printpreviewdialog -no-feature-printpreviewwidget -no-feature-sql -no-feature-sqlmodel -no-feature-textbrowser -no-feature-textmarkdownwriter -no-feature-textodfwriter -no-feature-xml -prefix C:\Qt_static
nmake
nmake install
```

One could speed up building with [`jom`](https://wiki.qt.io/Jom), a replacement for `nmake` which makes use of all CPU cores.

To build Bitcoin Core without Qt, unload or disable the `bitcoin-qt`, `libbitcoin_qt` and `test_bitcoin-qt` projects.


Building
---------------------
1. Use Python to generate `*.vcxproj` for the Visual Studio 2022 toolchain from Makefile:

```cmd
python build_msvc\msvc-autogen.py
```

2. An optional step is to adjust the settings in the `build_msvc` directory and the `common.init.vcxproj` file. This project file contains settings that are common to all projects such as the runtime library version and target Windows SDK version. The Qt directories can also be set. To specify a non-default path to a static Qt package directory, use the `QTBASEDIR` environment variable.

3. To build from the command-line with the Visual Studio toolchain use:

```cmd
msbuild build_msvc\bitcoin.sln -property:Configuration=Release -maxCpuCount -verbosity:minimal
```

Alternatively, open the `build_msvc/bitcoin.sln` file in Visual Studio.

Security
---------------------
[Base address randomization](https://learn.microsoft.com/en-us/cpp/build/reference/dynamicbase-use-address-space-layout-randomization) is used to make Bitcoin Core more secure. When building Bitcoin using the `build_msvc` process base address randomization can be disabled by editing `common.init.vcproj` to change `RandomizedBaseAddress` from `true` to `false` and then rebuilding the project.

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
