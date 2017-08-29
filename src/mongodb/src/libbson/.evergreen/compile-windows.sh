#!/bin/sh
set -o igncr    # Ignore CR in this script
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#       CC       Which compiler to use
#       RELEASE  Enable release-build MSVC flags and build from release
#                tarball (default: debug flags, build from git)

CC=${CC:-"Visual Studio 14 2015 Win64"}

echo "CC: $CC"
echo "RELEASE: $RELEASE"

CONFIGURE_FLAGS="-DCMAKE_INSTALL_PREFIX=C:/libbson -DENABLE_EXAMPLES:BOOL=ON"
BUILD_FLAGS="/m"  # Number of concurrent processes. No value=# of cpus

if [ "$RELEASE" ]; then
   CONFIGURE_FLAGS="$CONFIGURE_FLAGS -DCMAKE_BUILD_TYPE=Release"
   BUILD_FLAGS="$BUILD_FLAGS /p:Configuration=Release"
   TEST_PATH="./Release/test-libbson.exe"
   # Build from the release tarball.
   mkdir build-dir
   tar xf ../libbson.tar.gz -C build-dir --strip-components=1
   cd build-dir
else
   CONFIGURE_FLAGS="$CONFIGURE_FLAGS -DCMAKE_BUILD_TYPE=Debug"
   BUILD_FLAGS="$BUILD_FLAGS /p:Configuration=Debug"
   TEST_PATH="./Debug/test-libbson.exe"
fi

case "$CC" in
   mingw*)
      if [ "$RELEASE" ]; then
         cmd.exe /c ..\\.evergreen\\compile.bat
      else
         cmd.exe /c .evergreen\\compile.bat
      fi
      exit 0
   ;;
   # Resolve the compiler name to correct MSBuild location
   "Visual Studio 10 2010")
      BUILD="/cygdrive/c/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
   ;;
   "Visual Studio 10 2010 Win64")
      BUILD="/cygdrive/c/Windows/Microsoft.NET/Framework64/v4.0.30319/MSBuild.exe"
   ;;
   "Visual Studio 12 2013" | "Visual Studio 12 2013 Win64")
      BUILD="/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe"
   ;;
   "Visual Studio 14 2015" | "Visual Studio 14 2015 Win64")
      BUILD="/cygdrive/c/Program Files (x86)/MSBuild/14.0/Bin/MSBuild.exe"
   ;;
esac

rm -rf cbuild
mkdir cbuild
cd cbuild

CMAKE="/cygdrive/c/cmake/bin/cmake"
"$CMAKE" -G "$CC" $CONFIGURE_FLAGS ..
"$BUILD" $BUILD_FLAGS ALL_BUILD.vcxproj
"$BUILD" $BUILD_FLAGS INSTALL.vcxproj

"$TEST_PATH" --no-fork -d
