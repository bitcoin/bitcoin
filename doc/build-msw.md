Copyright (c) 2018 Aithercoin Developers    
Copyright (c) 2009-2012 Bitcoin Developers    
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com) and UPnP
software written by Thomas Bernard.


See readme-qt.rst for instructions on building Aithercoin QT, the
graphical user interface.

WINDOWS BUILD NOTES
===================

Required software
-------------------
####Perl 5: 
http://www.activestate.com/activeperl/downloads    
####Python: 
https://www.python.org/downloads/windows/    
####Msys 2: 
Download from https://www.msys2.org/    
Then run on msys2 environment:
```
pacman -S --needed base-devel mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain git subversion mercurial mingw-w64-i686-cmake mingw-w64-x86_64-cmake
```


Dependencies
------------
Libraries you need to download separately and build:
````
                default path                download
OpenSSL         C:\openssl-1.0.2o           http://www.openssl.org/source/
Berkeley DB     C:\db-4.8.30                http://www.oracle.com/technology/software/products/berkeley-db/index.html
Boost           C:\boost_1_67_0             http://www.boost.org/users/download/
miniupnpc       C:\miniupnpc-1.6            http://miniupnp.tuxfamily.org/files/
````
Their licenses:
OpenSSL        Old BSD license with the problematic advertising requirement
Berkeley DB    New BSD license with additional requirement that linked software must be free open source
Boost          MIT-like license
miniupnpc      New (3-clause) BSD license

Versions used in this release:
OpenSSL      l-1.0.2o
Berkeley DB  4.8.30
Boost        1.67.0
miniupnpc    1.6


OpenSSL
-------
MSYS2 shell:
un-tar sources with MSYS 'tar xfz' to avoid issue with symlinks (OpenSSL ticket 2377)
change 'MAKE' env. variable from 'C:\MinGW32\bin\mingw32-make.exe' to '/c/MinGW32/bin/mingw32-make.exe'
```
cd /c/openssl-1.0.2o
./config
make
```
Berkeley DB
-----------
MSYS2 shell:
```
cd /c/db-4.8.30.NC-mgw/build_unix
sh ../dist/configure --enable-mingw --enable-cxx
make
```
Boost
-----
DOS prompt:
```
cd C:\boost_1_67_0\tools\build\src\engine
PATH=C:\msys32\mingw32\bin;%PATH%
build.bat gcc
copy bin.ntx86\*.exe ..\..\..\..
cd ..\..\..\..
bjam toolset=gcc --build-type=complete stage
```
MiniUPnPc
---------
UPnP support is optional, make with USE_UPNP= to disable it.

MSYS2 shell:
```
cd /c/miniupnpc-1.6-mgw
make -f Makefile.mingw
mkdir miniupnpc
cp *.h miniupnpc/
```
Aithercoin
-------
DOS prompt:
cd \aithercoin\src
mingw32-make -f makefile.mingw
strip aithercoind.exe
