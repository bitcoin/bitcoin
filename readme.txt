BitCoin v0.1.5 ALPHA

Copyright (c) 2009 Satoshi Nakamoto
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Compilers Supported
-------------------
MinGW GCC (v3.4.5)
Microsoft Visual C++ 6.0 SP6


Dependencies
------------
Libraries you need to obtain separately to build:

              default path   download
wxWidgets      \wxWidgets     http://www.wxwidgets.org/downloads/
OpenSSL        \OpenSSL       http://www.openssl.org/source/
Berkeley DB    \DB            http://www.oracle.com/technology/software/products/berkeley-db/index.html
Boost          \Boost         http://www.boost.org/users/download/

Their licenses:
wxWidgets      LGPL 2.1 with very liberal exceptions
OpenSSL        Old BSD license with the problematic advertising requirement
Berkeley DB    New BSD license with additional requirement that linked software must be free open source
Boost          MIT-like license


OpenSSL
-------
Bitcoin does not use any encryption.  If you want to do a no-everything
build of OpenSSL to exclude encryption routines, a few patches are required.
(OpenSSL v0.9.8h)

Edit engines\e_gmp.c and put this #ifndef around #include <openssl/rsa.h>
  #ifndef OPENSSL_NO_RSA
  #include <openssl/rsa.h>
  #endif

Add this to crypto\err\err_all.c before the ERR_load_crypto_strings line:
  void ERR_load_RSA_strings(void) { }

Edit ms\mingw32.bat and replace the Configure line's parameters with this
no-everything list.  You have to put this in the batch file because batch
files can't handle more than 9 parameters.
  perl Configure mingw threads no-rc2 no-rc4 no-rc5 no-idea no-des no-bf no-cast no-aes no-camellia no-seed no-rsa no-dh

Also REM out the following line in ms\mingw32.bat.  The build fails after it's
already finished building libeay32, which is all we care about, but the
failure aborts the script before it runs dllwrap to generate libeay32.dll.
  REM  if errorlevel 1 goto end

Build
  ms\mingw32.bat

If you want to use it with MSVC, generate the .lib file
  lib /machine:i386 /def:ms\libeay32.def /out:out\libeay32.lib


Berkeley DB
-----------
MinGW with MSYS:
cd \DB\build_unix
sh ../dist/configure --enable-mingw --enable-cxx
make


Boost
-----
You may need Boost version 1.35 to build with MSVC 6.0.  I couldn't get
version 1.37 to compile with MSVC 6.0.
