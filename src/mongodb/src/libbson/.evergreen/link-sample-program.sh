#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#   BUILD_LIBBSON_WITH_CMAKE Build libbson with CMake. Default: use Autotools.
#   LINK_STATIC              Whether to statically link to libbson
#   BUILD_SAMPLE_WITH_CMAKE  Link program w/ CMake. Default: use pkg-config.


echo "BUILD_LIBBSON_WITH_CMAKE=$BUILD_LIBBSON_WITH_CMAKE LINK_STATIC=$LINK_STATIC BUILD_SAMPLE_WITH_CMAKE=$BUILD_SAMPLE_WITH_CMAKE"

CMAKE=${CMAKE:-/opt/cmake/bin/cmake}

if command -v gtar 2>/dev/null; then
  TAR=gtar
else
  TAR=tar
fi

if [ $(uname) = "Darwin" ]; then
  SO=dylib
  LIB_SO=libbson-1.0.0.dylib
  LDD="otool -L"
else
  SO=so
  LIB_SO=libbson-1.0.so.0
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
$TAR xf ../../libbson.tar.gz -C . --strip-components=1

if [ "$LINK_STATIC" ]; then
  if [ "$BUILD_LIBBSON_WITH_CMAKE" ]; then
    # Our CMake system builds shared and static by default.
    $CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_TESTS=OFF .
  else
    ./configure --prefix=$INSTALL_DIR --enable-static
  fi
  make
  make install
else
  if [ "$BUILD_LIBBSON_WITH_CMAKE" ]; then
    # Our CMake system builds shared and static by default.
    $CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF .
  else
    ./configure --prefix=$INSTALL_DIR
  fi
  make
  make install

  set +o xtrace

  if test -f $INSTALL_DIR/lib/libbson-static-1.0.a; then
    echo "libbson-static-1.0.a shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/libbson-1.0.a; then
    echo "libbson-1.0.a shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/pkgconfig/libbson-static-1.0.pc; then
    echo "libbson-static-1.0.pc shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/cmake/libbson-static-1.0/libbson-static-1.0-config.cmake; then
    echo "libbson-static-1.0-config.cmake shouldn't have been installed"
    exit 1
  fi
  if test -f $INSTALL_DIR/lib/cmake/libbson-static-1.0/libbson-static-1.0-config-version.cmake; then
    echo "libbson-static-1.0-config-version.cmake shouldn't have been installed"
    exit 1
  fi

fi

set +o xtrace

LIB=$INSTALL_DIR/lib/libbson-1.0.$SO
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

if test ! -f $INSTALL_DIR/lib/pkgconfig/libbson-1.0.pc; then
  echo "libbson-1.0.pc missing!"
  exit 1
else
  echo "libbson-1.0.pc check ok"
fi
if test ! -f $INSTALL_DIR/lib/cmake/libbson-1.0/libbson-1.0-config.cmake; then
  echo "libbson-1.0-config.cmake missing!"
  exit 1
else
  echo "libbson-1.0-config.cmake check ok"
fi
if test ! -f $INSTALL_DIR/lib/cmake/libbson-1.0/libbson-1.0-config-version.cmake; then
  echo "libbson-1.0-config-version.cmake missing!"
  exit 1
else
  echo "libbson-1.0-config-version.cmake check ok"
fi

if [ "$LINK_STATIC" ]; then
  if test ! -f $INSTALL_DIR/lib/libbson-static-1.0.a; then
    echo "libbson-static-1.0.a missing!"
    exit 1
  else
    echo "libbson-static-1.0.a check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/pkgconfig/libbson-static-1.0.pc; then
    echo "libbson-static-1.0.pc missing!"
    exit 1
  else
    echo "libbson-static-1.0.pc check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/cmake/libbson-static-1.0/libbson-static-1.0-config.cmake; then
    echo "libbson-static-1.0-config.cmake missing!"
    exit 1
  else
    echo "libbson-static-1.0-config.cmake check ok"
  fi
  if test ! -f $INSTALL_DIR/lib/cmake/libbson-static-1.0/libbson-static-1.0-config-version.cmake; then
    echo "libbson-static-1.0-config-version.cmake missing!"
    exit 1
  else
    echo "libbson-static-1.0-config-version.cmake check ok"
  fi
fi

set -o xtrace
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
    echo $(pkg-config --libs --cflags libbson-static-1.0)
    sh compile-with-pkg-config-static.sh
  else
    echo "pkg-config output:"
    echo $(pkg-config --libs --cflags libbson-1.0)
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

echo "ldd hello_bson:"
$LDD hello_bson

./hello_bson
