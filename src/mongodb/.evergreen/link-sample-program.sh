#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#   BUILD_MONGOC_WITH_CMAKE    Build mongoc with CMake. Default: use Autotools.
#   LINK_STATIC                Whether to statically link to libmongoc
#   BUILD_SAMPLE_WITH_CMAKE    Link program w/ CMake. Default: use pkg-config.
#   ENABLE_SSL                 Set -DENABLE_SSL or --enable-ssl.


echo "BUILD_MONGOC_WITH_CMAKE=$BUILD_MONGOC_WITH_CMAKE LINK_STATIC=$LINK_STATIC BUILD_SAMPLE_WITH_CMAKE=$BUILD_SAMPLE_WITH_CMAKE"

CMAKE=${CMAKE:-/opt/cmake/bin/cmake}

if command -v gtar 2>/dev/null; then
  TAR=gtar
else
  TAR=tar
fi

if [ $(uname) = "Darwin" ]; then
  SO=dylib
  LIB_SO=libmongoc-1.0.0.dylib
  LDD="otool -L"
else
  SO=so
  LIB_SO=libmongoc-1.0.so.0
  LDD=ldd
fi

SRCROOT=`pwd`

BUILD_DIR=$(pwd)/build-dir
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

INSTALL_DIR=$(pwd)/install-dir
rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

cd $BUILD_DIR
$TAR xf ../../mongoc.tar.gz -C . --strip-components=1

if [ "$ENABLE_SSL" ]; then
  SSL_CONFIGURE_OPTION="--enable-ssl"
  SSL_CMAKE_OPTION="-DENABLE_SSL:BOOL=ON"
else
  SSL_CONFIGURE_OPTION="--disable-ssl"
  SSL_CMAKE_OPTION="-DENABLE_SSL:BOOL=OFF"
fi


if [ "$LINK_STATIC" ]; then
  STATIC_CONFIGURE_OPTION="--enable-static"
  STATIC_CMAKE_OPTION="-DENABLE_STATIC=ON -DENABLE_TESTS=ON"
else
  STATIC_CMAKE_OPTION="-DENABLE_STATIC=OFF -DENABLE_TESTS=OFF"
fi


if [ "$BUILD_MONGOC_WITH_CMAKE" ]; then
  # Our CMake script doesn't build bundled libbson (CDRIVER-1948) so fake it.
  cd src/libbson
  $CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR $STATIC_CMAKE_OPTION .
  make
  make install
  cd ../..

  $CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake $SSL_CMAKE_OPTION $STATIC_CMAKE_OPTION .
  make
  make install
else
  ./configure --prefix=$INSTALL_DIR --disable-examples $SSL_CONFIGURE_OPTION $STATIC_CONFIGURE_OPTION --with-libbson=bundled
  make
  make install
fi


LIB=$INSTALL_DIR/lib/libmongoc-1.0.$SO
if test ! -L $LIB; then
   echo "$LIB should be a symlink"
   exit 1
fi

TARGET=$(readlink $LIB)
if test ! -f $INSTALL_DIR/lib/$TARGET; then
  echo "$LIB target $INSTALL_DIR/lib/$TARGET is missing!"
  exit 1
else
  echo "$LIB target $INSTALL_DIR/lib/$TARGET check ok"
fi


if test ! -f $INSTALL_DIR/lib/$LIB_SO; then
  echo "$LIB_SO missing!"
  exit 1
else
  echo "$LIB_SO check ok"
fi

if test ! -f $INSTALL_DIR/lib/pkgconfig/libmongoc-1.0.pc; then
  echo "libmongoc-1.0.pc missing!"
  exit 1
else
  echo "libmongoc-1.0.pc check ok"
fi
if test ! -f $INSTALL_DIR/lib/cmake/libmongoc-1.0/libmongoc-1.0-config.cmake; then
  echo "libmongoc-1.0-config.cmake missing!"
  exit 1
else
  echo "libmongoc-1.0-config.cmake check ok"
fi
if test ! -f $INSTALL_DIR/lib/cmake/libmongoc-1.0/libmongoc-1.0-config-version.cmake; then
  echo "libmongoc-1.0-config-version.cmake missing!"
  exit 1
else
  echo "libmongoc-1.0-config-version.cmake check ok"
fi


if [ "$LINK_STATIC" ]; then
  if test ! -f $INSTALL_DIR/lib/libmongoc-static-1.0.a; then
    echo "libmongoc-static-1.0.a missing!"
    exit 1
  else
    echo "libmongoc-static-1.0.a check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/pkgconfig/libmongoc-static-1.0.pc; then
    echo "libmongoc-static-1.0.pc missing!"
    exit 1
  else
    echo "libmongoc-static-1.0.pc check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config.cmake; then
    echo "libmongoc-static-1.0-config.cmake missing!"
    exit 1
  else
    echo "libmongoc-static-1.0-config.cmake check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config-version.cmake; then
    echo "libmongoc-static-1.0-config-version.cmake missing!"
    exit 1
  else
    echo "libmongoc-static-1.0-config-version.cmake check ok"
  fi
else
  if test -f $INSTALL_DIR/lib/libmongoc-static-1.0.a; then
    echo "libmongoc-static-1.0.a shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/libmongoc-1.0.a; then
    echo "libmongoc-1.0.a shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/pkgconfig/libmongoc-static-1.0.pc; then
    echo "libmongoc-static-1.0.pc shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config.cmake; then
    echo "libmongoc-static-1.0-config.cmake shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config-version.cmake; then
    echo "libmongoc-static-1.0-config-version.cmake shouldn't have been installed"
    exit 1
  fi
fi

cd $SRCROOT

if [ "$BUILD_SAMPLE_WITH_CMAKE" ]; then
  # Test our CMake package config file with CMake's find_package command.
  EXAMPLE_DIR=$SRCROOT/examples/cmake/find_package

  if [ "$LINK_STATIC" ]; then
    EXAMPLE_DIR="${EXAMPLE_DIR}_static"
  fi

  cd $EXAMPLE_DIR
  $CMAKE -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake .
  make
else
  # Test our pkg-config file.
  export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig
  cd $SRCROOT/examples

  if [ "$LINK_STATIC" ]; then
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags libmongoc-static-1.0)
    sh compile-with-pkg-config-static.sh
  else
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags libmongoc-1.0)
    sh compile-with-pkg-config.sh
  fi
fi

if [ ! "$LINK_STATIC" ]; then
  if [ $(uname) = "Darwin" ]; then
    export DYLD_LIBRARY_PATH=$INSTALL_DIR/lib
  else
    export LD_LIBRARY_PATH=$INSTALL_DIR/lib
  fi
fi

echo "ldd hello_mongoc:"
$LDD hello_mongoc

./hello_mongoc
