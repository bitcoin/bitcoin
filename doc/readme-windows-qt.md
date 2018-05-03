## Dependencies  
### 7-zip
https://www.7-zip.org/
### Perl 5: 
http://www.activestate.com/activeperl/downloads    
### Python: 
https://www.python.org/downloads/windows/    
### Mingw: 
Download from `mingw-get-inst-20120426.exe`
Download `mingw32-gcc-4.6.2-release-c,c++,objc,objc++,fortran-sjlj.7z` extract to somewhere, rename it from `mingw` to `mingw32`, then copy it to `C:/MingW`     
### QT SDK
http://qt-project.org/downloads
### OpenSSL
https://www.openssl.org/source/openssl-1.0.1b.tar.gz   
and put it in `C:/deps`    
then run `MSYS`
```
tar xvfz openssl-1.0.1b.tar.gz
cd openssl-1.0.1b
./config
make
```
### Berkeley DB
http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz   
and put it in `C:/deps`   
then run `MSYS`
````
tar xvfz db-4.8.30.NC.tar.gz
cd db-4.8.30.NC/build_unix 
sh ../dist/configure --enable-mingw --enable-cxx --disable-shared --disable-replication
make
````
### Boost
```
wget -O boost_1_55_0.7z http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.7z/download
7z x boost_1_55_0.7z
```
(From windows CMD shell)
```
cd C:\deps\boost_1_55_0
PATH=C:\MinGW\mingw32\bin;%PATH%
bootstrap.bat mingw
b2 --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc variant=release link=static threading=multi runtime-link=static stage
```
### MiniUPNP
```
wget -O miniupnpc-1.9.tar.gz http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.9.tar.gz
tar xvfz miniupnpc-1.9.tar.gz
cd miniupnpc-1.9
vim Makefile.mingw
	change: wingenminiupnpcstrings $< $@  
	to: ./wingenminiupnpcstrings.exe $< $@  
make -f Makefile.mingw
mkdir -p include/miniupnpc
cp *.h include/miniupnpc
cd ..
```

### QREncode
```
wget http://fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz
tar xvzf qrencode-3.4.4.tar.gz
cd qrencode-3.4.4
./configure --enable-static --enable-shared --without-tools
make  
cd ..
```
### Build Binaries
```
cd $HOME
git clone https://github.com/aithercoin/aithercoin.git
cd aithercoin
qmake Aithercoin-qt.pro -r -spec win32-g++ "CONFIG+=release" USE_UPNP=- USE_QRCODE=1 USE_IPV6=1
make clean
cd src/leveldb
make TARGET_OS=OS_WINDOWS_CROSSCOMPILE clean
make TARGET_OS=OS_WINDOWS_CROSSCOMPILE libleveldb.a libmemenv.a
cd ../..
make release
```

### Create deployment
```
cd release
rm qrc_bitcoin.cpp
windeployqt aithercoin-qt.exe
cp ../contrib/release/opengl32sw.dll ../contrib/release/qt.conf .
cp /usr/src/deps32/boost_1_67_0/stage/lib/libboost*.dll .
cp /mingw32/bin/libpcre16-0.dll /mingw32/bin/zlib1.dll /mingw32/bin/libharfbuzz-0.dll /mingw32/bin/libpng16-16.dll /mingw32/bin/libicudt56.dll /mingw32/bin/libfreetype-6.dll .
cp /mingw32/bin/libglib-2.0-0.dll /mingw32/bin/libbz2-1.dll /mingw32/bin/libintl-8.dll /mingw32/bin/libpcre-1.dll /mingw32/bin/libiconv-2.dll .
```