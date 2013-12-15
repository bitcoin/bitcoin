WINDOWS BUILD GUIDE (via [nitrogenetics and icy](https://bitcointalk.org/index.php?topic=149479.0))
===================

## <br/>1. Prepare your build system.
I suggest setting up a clean virtual machine via Virtualbox or similar.

### 1.1 Install [MSYS](http://sourceforge.net/projects/mingw/files/Installer/mingw-get-setup.exe/download)
Select msys-base package from basic setup then apply changes.

### 1.2 Install [Perl](http://downloads.activestate.com/ActivePerl/releases/5.18.1.1800/ActivePerl-5.18.1.1800-MSWin32-x64-297570.msi) and Install [Python](http://www.python.org/ftp/python/3.3.2/python-3.3.2.amd64.msi)
You can exclude Perl Script, PPM, documentation and examples if you don't need them for other purposes.

### 1.3 Install GCC from [Mingw-builds](http://sourceforge.net/projects/mingwbuilds/files/host-windows/releases/4.8.1/32-bit/threads-posix/dwarf/x32-4.8.1-release-posix-dwarf-rev5.7z/download)
Download and unpack x32-4.8.1-release-posix-dwarf-rev5.7z

### 1.4. Ensure that Mingw-builds
Perl and Python bin folders are set in your PATH environment variable.


## 2. Download, unpack and build required dependencies.
I'll save them in `c:\deps` folder.

### 2.1 Install [OpenSSL](http://www.openssl.org/source/openssl-1.0.1e.tar.gz)
From a MinGw shell `C:\MinGW\msys\1.0\msys.bat`, unpack the source archive with tar (this will avoid symlink issues) then configure and make:

	cd /c/deps/
	tar xvfz openssl-1.0.1e.tar.gz
	cd openssl-1.0.1e
	./config
	make

### 2.2 Install [Berkeley DB](http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz) 
We'll use version 4.8 to preserve binary wallet compatibility.
From a MinGW shell unpack the source archive, configure, edit line 113 of db.h then make:

	cd /c/deps/
	tar xvfz db-4.8.30.NC.tar.gz
	cd db-4.8.30.NC/build_unix
	../dist/configure --disable-replication --enable-mingw --enable-cxx

after configuring make sure to edit your build_unix/db.h by replacing line 113:

	typedef pthread_t db_threadid_t;

with

	typedef u_int32_t db_threadid_t;

now you can:

	make

### 2.3 [Install Boost](http://sourceforge.net/projects/boost/files/boost/1.54.0/)
Unpack boost inside your `C:\deps` folder, then bootstrap and compile from a Windows command prompt:

	cd C:\deps\boost_1_54_0\
	bootstrap.bat mingw
	b2 --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc stage

This will compile the required boost libraries and put them into the stage folder `C:\deps\boost_1_54_0\stage`.

### 2.4 [Install Miniupnpc](http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.8.tar.gz)
Unpack Miniupnpc inside your `C:\deps` folder, then double click on mingw32make.bat


## 3. Download Bitcoin 0.8.5 from Github
Compile leveldb then compile [bitcoind](https://github.com/bitcoin/bitcoin/archive/v0.8.5.zip).
Leveldb libraries will not compile automatically so we'll need to compile them first.

### 3.1 Extract bitcoin-0.8.5.zip (for example to C:\) 
Then start MinGW shell and change into leveldb folder:

	cd /C/bitcoin-0.8.5/src/leveldb
	TARGET_OS=NATIVE_WINDOWS make libleveldb.a libmemenv.a

this will compile both libleveldb.a and libmemenv.a libraries required by bitcoin.

### 3.2 Now Edit
With a texteditor edit BOOST_SUFFIX, INCLUDEPATHS and LIBPATHS in your `C:\bitcoin-0.8.5\src\makefile.mingw `according to your dependencies location:

	BOOST_SUFFIX?=-mgw48-mt-s-1_54

	INCLUDEPATHS= \
	 -I"$(CURDIR)" \
	 -I"c:/deps/boost_1_54_0" \
	 -I"c:/deps/db-4.8.30.NC/build_unix" \
	 -I"c:/deps/openssl-1.0.1e/include"
 
	LIBPATHS= \
	 -L"$(CURDIR)/leveldb" \
	 -L"c:/deps/boost_1_54_0/stage/lib" \
	 -L"c:/deps/db-4.8.30.NC/build_unix" \
	 -L"c:/deps/openssl-1.0.1e"

When compiling dinamically you will need to distribute libgcc_s_dw2-1.dll and libstdc++-6.dll along with the executable.You can compile a statically linked executable by adding -static option to LDFLAGS in makefile.mingw.

	LDFLAGS=-Wl,--dynamicbase -Wl,--nxcompat -Wl,--large-address-aware -static

### 3.3 Net.cpp will need to be patched in order to compile with gcc 4.8.1:
Fix invalid conversion error with MinGW 4.8.1 in net.cpp. See [Github Issue](https://github.com/bitcoin/bitcoin/commit/6c6255edb54bed780f0879c906dccf6cfa98b4db).

### 3.4 From a Windows command prompt run:

	cd C:\bitcoin\bitcoin-master\src
	mingw32-make -f makefile.mingw
	strip bitcoind.exe


## 4. Compiling Qt 5.1.1 libraries and Bitcoin-Qt 

### 4.1 Download and unpack [Qt sources](http://download.qt-project.org/official_releases/qt/5.1/5.1.1/single/qt-everywhere-opensource-src-5.1.1.7z)

### 4.2 Apply patches for the following bugs:
- [https://bugreports.qt-project.org/browse/QTBUG-33155](https://bugreports.qt-project.org/browse/QTBUG-33155)
- [https://bugreports.qt-project.org/browse/QTBUG-33225](https://bugreports.qt-project.org/browse/QTBUG-33225)

### 4.3 From a windows command prompt configure and make:
	cd C:\Qt\5.1.1
	if not exist qtbase\.gitignore type nul>qtbase\.gitignore
	configure -release -opensource -confirm-license -make libs -no-opengl -no-icu -no-vcproj -no-openssl
	mingw32-make

To build static libraries use -static option instead:

	configure -release -static -opensource -confirm-license -make libs -no-opengl -no-icu -no-vcproj -no-openssl


### 4.4 Edit `C:\bitcoin-0.8.5\bitcoin-qt.pro` with your favorite text editor and replace:
	QT += network
with

	QT += network widgets

add dependency library locations:

	# Dependency library locations can be customized with:
	#    BOOST_INCLUDE_PATH, BOOST_LIB_PATH, BDB_INCLUDE_PATH,
	#    BDB_LIB_PATH, OPENSSL_INCLUDE_PATH and OPENSSL_LIB_PATH respectively
	
	BOOST_LIB_SUFFIX=-mgw48-mt-s-1_54
	BOOST_INCLUDE_PATH=C:/deps/boost_1_54_0
	BOOST_LIB_PATH=C:/deps/boost_1_54_0/stage/lib
	BDB_INCLUDE_PATH=C:/deps/db-4.8.30.NC/build_unix
	BDB_LIB_PATH=C:/deps/db-4.8.30.NC/build_unix
	OPENSSL_INCLUDE_PATH=C:/deps/openssl-1.0.1e/include
	OPENSSL_LIB_PATH=C:/deps/openssl-1.0.1e
	MINIUPNPC_INCLUDE_PATH=C:/deps/
	MINIUPNPC_LIB_PATH=C:/deps/miniupnpc

Comment out genleveldb.commands for win32

	   LIBS += -lshlwapi
	    #genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_WINDOWS_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libleveldb.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libmemenv.a
	}

### 4.5 Bitcoin 0.8.5 will need some patches in order to compile with Qt5:
Remove #define loop from util.h and replace loop with while (true) in

	src/bitcoinrpc.cpp
	src/main.cpp
	src/net.cpp
	src/script.cpp
	src/util.cpp
	src/wallet.cpp
	src/walletdb.cpp
See [this commit](https://github.com/bitcoin/bitcoin/commit/8351d55cd3955c95c5e3fe065a456db08cc8a559).

Qt5 compatibility. #if QT_VERSION < 0x050000 ... in
src/qt/addressbookpage.cpp
	src/qt/bitcoin.cpp
	src/qt/bitcoingui.cpp
	src/qt/guiutil.cpp
	src/qt/paymentserver.cpp
	src/qt/qrcodedialog.cpp
	src/qt/rpcconsole.cpp
	src/qt/sendcoinsdialog.cpp
	src/qt/transactionview.cpp
	src/qt/walletview.cpp
See [this commit](https://github.com/bitcoin/bitcoin/commit/25c0cce7fb494fcb871d134e28b26504d30e34d3).

- [http://qt-project.org/doc/qt-5.0/qtdoc/sourcebreaks.html](http://qt-project.org/doc/qt-5.0/qtdoc/sourcebreaks.html)
- [http://qt-project.org/wiki/Transition_from_Qt_4.x_to_Qt5](http://qt-project.org/wiki/Transition_from_Qt_4.x_to_Qt5)


### 4.6 Finish Up
Ensure your qtbase bin folder (eg `C:\Qt\5.1.1\qtbase\bin`) is in PATH then from a windows command prompt configure and make:

	cd C:\bitcoin\bitcoin-master
	qmake bitcoin-qt.pro
	mingw32-make -f Makefile.Release

QtCore5.dll, QtGui5.dll, QtNetwork5.dll, libgcc_s_dw2-1.dll and libstdc++-6.dll libraries will need to be distributed along with the executable when compiling dynamically. To build a statically linked executable edit bitcoin-qt.pro by adding the necessary static flags:

	CONFIG += static
and

	win32:QMAKE_LFLAGS *= -Wl,--large-address-aware -static

Make sure your Qt folder in PATH points to statically compiled Qt libraries, clean up `C:\bitcoin-0.8.5\build` folder then configure and make again.