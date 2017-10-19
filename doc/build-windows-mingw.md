Building Bitcoin on Native Windows
==================================

This document describes the installation of the mysys and mingw tools as well as 
the build dependencies and then finally the compilation of the bitcoin binaries.

NOTE: These instructions will allow you to build bitcoin as either 32-bit or 64-bit.
      For the most part, the build instructions are the same after the toolchain and
      PATH variables are configured.  Any differences between the 32-bit and 64-bit
      build instructions have been noted inline at each instruction step.

Prepare your build system
-------------------------

1.1 Install msys shell:
http://sourceforge.net/projects/mingw/files/Installer/mingw-get-setup.exe/download
From MinGW installation manager -> All packages -> MSYS
mark the following for installation:

msys-base-bin
msys-autoconf-bin
msys-automake-bin
msys-libtool-bin

Make sure no mingw packages are checked for installation or present from a previous install. Only the above msys packages should be installed. Also make sure that msys-gcc and msys-w32api packages are not installed.

then click on Installation -> Apply changes


1.2 Install MinGW-builds project toolchain:
For 32-bit download:
http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.9.2/threads-posix/dwarf/i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z/download
and unpack it to C:\

For 64-bit download:
http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/4.9.2/threads-posix/seh/x86_64-4.9.2-release-posix-seh-rt_v3-rev1.7z/download
and unpack it to C:\

1.3. Ensure that the desired toolchain's bin folder is set in your PATH environment variable. This will control which toolchain, 32-bit or 64-bit, is used for building.  On Windows 7 your path should look something like:

For 32-bit builds:
C:\mingw32\bin;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SYSTEMROOT%\System32\WindowsPowerShell\v1.0\

For 64-bit builds:
C:\mingw64\bin;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SYSTEMROOT%\System32\WindowsPowerShell\v1.0\

IMPORTANT: It is fine to have both the 32-bit and 64-bit toolchains installed at the same time, however you should only have one in your PATH variable at a time.  If you place both in your PATH, then the build will be made with whichever toolchain path that is listed first.

NOTE: If you install both toolchains, when you switch between them remember that you need to rebuild all of the dependencies with the same toolchain as you are building the bitcoin client, otherwise linking will fail if you try to mix 32-bit and 64-bit binaries.


1.4 Additional checks:
C:\MinGW\bin should contain nothing but mingw-get.exe.

Create a shortcut to C:\MinGw\msys\1.0\msys.bat on your desktop.
Double-click on the shortcut to start a msys shell.

For the 32-bit toolchain your gcc -v output should be:

$ gcc -v
Using built-in specs.
COLLECT_GCC=c:\mingw32\bin\gcc.exe
COLLECT_LTO_WRAPPER=c:/mingw32/bin/../libexec/gcc/i686-w64-mingw32/4.9.2/lto-wrapper.exe
Target: i686-w64-mingw32
Configured with: ../../../src/gcc-4.9.2/configure --host=i686-w64-mingw32 --build=i686-w64-mingw32 --target=i686-w64-mingw32 --prefix=/mingw32 --with-sysroot=/c/mingw492/i686-492-posix-dwarf-rt_v3-rev1/mingw32 --with-gxx-include-dir=/mingw32/i686-w64-mingw32/include/c++ --enable-shared --enable-static --disable-multilib --enable-languages=ada,c,c++,fortran,objc,obj-c++,lto --enable-libstdcxx-time=yes --enable-threads=posix --enable-libgomp --enable-libatomic --enable-lto --enable-graphite --enable-checking=release --enable-fully-dynamic-string --enable-version-specific-runtime-libs --disable-sjlj-exceptions --with-dwarf2 --disable-isl-version-check --disable-cloog-version-check --disable-libstdcxx-pch --disable-libstdcxx-debug --enable-bootstrap --disable-rpath --disable-win32-registry --disable-nls --disable-werror --disable-symvers --with-gnu-as --with-gnu-ld --with-arch=i686 --with-tune=generic --with-libiconv --with-system-zlib --with-gmp=/c/mingw492/prerequisites/i686-w64-mingw32-static --with-mpfr=/c/mingw492/prerequisites/i686-w64-mingw32-static --with-mpc=/c/mingw492/prerequisites/i686-w64-mingw32-static --with-isl=/c/mingw492/prerequisites/i686-w64-mingw32-static --with-cloog=/c/mingw492/prerequisites/i686-w64-mingw32-static --enable-cloog-backend=isl --with-pkgversion='i686-posix-dwarf-rev1, Built by MinGW-W64 project' --with-bugurl=http://sourceforge.net/projects/mingw-w64 CFLAGS='-O2 -pipe -I/c/mingw492/i686-492-posix-dwarf-rt_v3-rev1/mingw32/opt/include -I/c/mingw492/prerequisites/i686-zlib-static/include -I/c/mingw492/prerequisites/i686-w64-mingw32-static/include' CXXFLAGS='-O2 -pipe -I/c/mingw492/i686-492-posix-dwarf-rt_v3-rev1/mingw32/opt/include -I/c/mingw492/prerequisites/i686-zlib-static/include -I/c/mingw492/prerequisites/i686-w64-mingw32-static/include' CPPFLAGS= LDFLAGS='-pipe -L/c/mingw492/i686-492-posix-dwarf-rt_v3-rev1/mingw32/opt/lib -L/c/mingw492/prerequisites/i686-zlib-static/lib -L/c/mingw492/prerequisites/i686-w64-mingw32-static/lib -Wl,--large-address-aware'
Thread model: posix
gcc version 4.9.2 (i686-posix-dwarf-rev1, Built by MinGW-W64 project)


For the 64-bit toolchain your gcc -v output should be:

$ gcc -v
Using built-in specs.
COLLECT_GCC=C:\mingw64\bin\gcc.exe
COLLECT_LTO_WRAPPER=C:/mingw64/bin/../libexec/gcc/x86_64-w64-mingw32/4.9.2/lto-wrapper.exe
Target: x86_64-w64-mingw32
Configured with: ../../../src/gcc-4.9.2/configure --host=x86_64-w64-mingw32 --build=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 --prefix=/mingw64 --with-sysroot=/c/mingw492/x86_64-492-posix-seh-rt_v3-rev1/mingw64 --with-gxx-include-dir=/mingw64/x86_64-w64-mingw32/include/c++ --enable-shared --enable-static --disable-multilib --enable-languages=ada,c,c++,fortran,objc,obj-c++,lto --enable-libstdcxx-time=yes --enable-threads=posix --enable-libgomp --enable-libatomic --enable-lto --enable-graphite --enable-checking=release --enable-fully-dynamic-string --enable-version-specific-runtime-libs --disable-isl-version-check --disable-cloog-version-check --disable-libstdcxx-pch --disable-libstdcxx-debug --enable-bootstrap --disable-rpath --disable-win32-registry --disable-nls --disable-werror --disable-symvers --with-gnu-as --with-gnu-ld --with-arch=nocona --with-tune=core2 --with-libiconv --with-system-zlib --with-gmp=/c/mingw492/prerequisites/x86_64-w64-mingw32-static --with-mpfr=/c/mingw492/prerequisites/x86_64-w64-mingw32-static --with-mpc=/c/mingw492/prerequisites/x86_64-w64-mingw32-static --with-isl=/c/mingw492/prerequisites/x86_64-w64-mingw32-static --with-cloog=/c/mingw492/prerequisites/x86_64-w64-mingw32-static --enable-cloog-backend=isl --with-pkgversion='x86_64-posix-seh-rev1, Built by MinGW-W64 project' --with-bugurl=http://sourceforge.net/projects/mingw-w64 CFLAGS='-O2 -pipe -I/c/mingw492/x86_64-492-posix-seh-rt_v3-rev1/mingw64/opt/include -I/c/mingw492/prerequisites/x86_64-zlib-static/include -I/c/mingw492/prerequisites/x86_64-w64-mingw32-static/include' CXXFLAGS='-O2 -pipe -I/c/mingw492/x86_64-492-posix-seh-rt_v3-rev1/mingw64/opt/include -I/c/mingw492/prerequisites/x86_64-zlib-static/include -I/c/mingw492/prerequisites/x86_64-w64-mingw32-static/include' CPPFLAGS= LDFLAGS='-pipe -L/c/mingw492/x86_64-492-posix-seh-rt_v3-rev1/mingw64/opt/lib -L/c/mingw492/prerequisites/x86_64-zlib-static/lib -L/c/mingw492/prerequisites/x86_64-w64-mingw32-static/lib '
Thread model: posix
gcc version 4.9.2 (x86_64-posix-seh-rev1, Built by MinGW-W64 project)


2. Download, unpack and build required dependencies.
Save them in c:\deps folder.

2.1 OpenSSL: http://www.openssl.org/source/openssl-1.0.1k.tar.gz
From a MinGw shell (C:\MinGW\msys\1.0\msys.bat), unpack the source archive with tar (this will avoid symlink issues) 
then configure and make:

NOTE: For 64-bit builds, when you run the ./Configure command below, you must change mingw to mingw64.


cd /c/deps/
tar xvfz openssl-1.0.1k.tar.gz
cd openssl-1.0.1k
./Configure no-zlib no-shared no-dso no-krb5 no-camellia no-capieng no-cast no-cms no-dtls1 no-gost no-gmp no-heartbeats no-idea no-jpake no-md2 no-mdc2 no-rc5 no-rdrand no-rfc3779 no-rsax no-sctp no-seed no-sha0 no-static_engine no-whirlpool no-rc2 no-rc4 no-ssl2 no-ssl3 mingw
make


2.2 Berkeley DB: http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz
We'll use version 4.8 to preserve binary wallet compatibility.
From a MinGW shell unpack the source archive, configure and make:


cd /c/deps/
tar xvfz db-4.8.30.NC.tar.gz
cd db-4.8.30.NC/build_unix
../dist/configure --enable-mingw --enable-cxx --disable-shared --disable-replication
make



2.3 Boost: http://sourceforge.net/projects/boost/files/boost/1.61.0/
Download either the zip or the 7z archive, unpack boost inside your C:\deps folder, then bootstrap and compile 
from a Windows command prompt:


cd C:\deps\boost_1_61_0\
bootstrap.bat gcc
b2 --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc variant=release link=static threading=multi runtime-link=static stage

This will compile the required boost libraries and put them into the stage folder (C:\deps\boost_1_61_0\stage).
NOTE: make sure you don't use tarballs, as unix EOL markers can break batch files.


2.3.1  Libevent 2.0.22: https://sourceforge.net/projects/levent/files/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz/download
Download and unpack, then run from the mysys shell:

tar -xvfz libevent-2.0.22-stable.tar.gz 
mv ./libevent-2.0.22-stable ./libevent-2.0.22
cd libevent-2.0.22

./configure --disable-shared

make



2.4 Miniupnpc: http://miniupnp.free.fr/files/download.php?file=miniupnpc-2.0.20170509.tar.gz
Unpack Miniupnpc to C:\deps, rename containing folder from "miniupnpc-2.0.20170509" to "miniupnpc" then 
from a Windows command prompt:


cd C:\deps\miniupnpc
set "OLD_CC=%CC%"
set "CC=gcc"
mingw32-make -f Makefile.mingw init upnpc-static
set "CC=%OLD_CC%"

NOTE: Miniupnpc version has been updated to 2.0.20170509 due to CVE-2017-8798
In v2.0.20170509 Makefile.mingw changed the CC definition to "CC ?= gcc" which can cause issues,
so the value for CC needs to be explicitly set prior to calling mingw32-make.


2.5 protoc and libprotobuf:
Download https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz
Then from msys shell:


tar xvfz protobuf-2.6.1.tar.gz
cd /c/deps/protobuf-2.6.1
configure --disable-shared
make



2.6 qrencode:
Download and unpack http://download.sourceforge.net/libpng/libpng-1.6.16.tar.gz inside your deps folder
then configure and make:


cd /c/deps/libpng-1.6.16
configure --disable-shared
make
cp .libs/libpng16.a .libs/libpng.a


Download and unpack http://fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz inside your deps folder 
then configure and make:

NOTE: If you are using the 64-bit toolchain, use the following LIBS line instead:
LIBS="../libpng-1.6.16/.libs/libpng.a ../../x86_64-w64-mingw32/lib/libz.a" \


cd /c/deps/qrencode-3.4.4

LIBS="../libpng-1.6.16/.libs/libpng.a ../../mingw32/i686-w64-mingw32/lib/libz.a" \
png_CFLAGS="-I../libpng-1.6.16" \
png_LIBS="-L../libpng-1.6.16/.libs" \
configure --enable-static --disable-shared --without-tools

make


2.7 Qt 5 libraries:
Qt must be configured with ssl and zlib support.
Download and unpack Qt base and tools sources:
http://download.qt-project.org/archive/qt/5.3/5.3.2/submodules/qtbase-opensource-src-5.3.2.7z
http://download.qt-project.org/archive/qt/5.3/5.3.2/submodules/qttools-opensource-src-5.3.2.7z
Then from a windows command prompt (note that the following assumes qtbase has been unpacked to C:\deps\Qt\5.3.2 and 
qttools have been unpacked to C:\deps\Qt\qttools-opensource-src-5.3.2):


set INCLUDE=C:\deps\libpng-1.6.16;C:\deps\openssl-1.0.1k\include
set LIB=C:\deps\libpng-1.6.16\.libs;C:\deps\openssl-1.0.1k

cd C:\deps\Qt\5.3.2
configure.bat -release -opensource -confirm-license -static -make libs -no-sql-sqlite -no-opengl -system-zlib -qt-pcre -no-icu -no-gif -system-libpng -no-libjpeg -no-freetype -no-angle -no-vcproj -openssl -no-dbus -no-audio-backend -no-wmf-backend -no-qml-debug
mingw32-make -j4

set PATH=%PATH%;C:\deps\Qt\5.3.2\bin
set PATH=%PATH%;C:\deps\Qt\qttools-opensource-src-5.3.2

cd C:\deps\Qt\qttools-opensource-src-5.3.2
qmake qttools.pro
mingw32-make -j4

NOTE: consider using -j switch with mingw32-make to speed up compilation process. On a quad core -j4 or -j5 should give the best results.


then 

make a BOOST_ROOT environment variable.  best to place it in your mysys.bat file which should
be on your desktop now.  that way it's there whenever you startup the shell.
Enter the following in the mysys shell or add it to the top (after the comment section) of the mysys.bat file.

set BOOST_ROOT=/c/deps/boost_1_61_0



Build bitcoin
-------------

From the mysys shell prompt cd to the root of the source code directory
and type the following command (for example, cd c:/bitcoin and NOT c:/bitcoin/src):

./autogen.sh

Once autogen completes, from the mysys shell prompt enter the following:


CPPFLAGS="-I/c/deps/db-4.8.30.NC/build_unix \
-I/c/deps/openssl-1.0.1k/include \
-I/c/deps/libevent-2.0.22/include \
-I/c/deps \
-I/c/deps/protobuf-2.6.1/src \
-I/c/deps/libpng-1.6.16 \
-I/c/deps/qrencode-3.4.4" \
LDFLAGS="-L/c/deps/db-4.8.30.NC/build_unix \
-L/c/deps/openssl-1.0.1k \
-L/c/deps/libevent-2.0.22/.libs \
-L/c/deps/miniupnpc \
-L/c/deps/protobuf-2.6.1/src/.libs \
-L/c/deps/libpng-1.6.16/.libs \
-L/c/deps/qrencode-3.4.4/.libs" \
BOOST_ROOT=/c/deps/boost_1_61_0 \
./configure \
--disable-upnp-default \
--disable-tests \
--with-qt-incdir=/c/Deps/Qt/5.3.2/include \
--with-qt-libdir=/c/Deps/Qt/5.3.2/lib \
--with-qt-plugindir=/c/Deps/Qt/5.3.2/plugins \
--with-qt-bindir=/c/Deps/Qt/5.3.2/bin \
--with-protoc-bindir=/c/deps/protobuf-2.6.1/src


After the configure script finishes it's finally time to compile.
From the msys shell prompt enter the following (this is the same for both 32-bit and 64-bit):

make -j4

NOTE: j is followed by the number of cores your machine has, so two core would be -j2 etc..


Finally you can strip the executables if you wish.  Stripping will remove debug symbols and greately reduce the final file size of the executables.  For example, the v12.1 bitcoin-qt.exe will go from ~241MB to ~26MB.

strip src/bitcoin-tx.exe
strip src/bitcoin-cli.exe
strip src/bitcoind.exe
strip src/qt/bitcoin-qt.exe
