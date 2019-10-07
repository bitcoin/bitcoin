#!/usr/bin/env sh
#
# Install musl and musl-sanitized linux kernel headers
# to musl-{$1} directory

set -ex

MUSL_VERSION=1.1.22
MUSL="musl-${MUSL_VERSION}"

# Download, configure, build, and install musl:
curl --retry 5 https://www.musl-libc.org/releases/${MUSL}.tar.gz | tar xzf -

cd $MUSL
case ${1} in
    aarch64)
        musl_arch=aarch64
        kernel_arch=arm64
        CC=aarch64-linux-gnu-gcc \
          ./configure --prefix="/musl-${musl_arch}" --enable-wrapper=yes
        make install -j4
        ;;
    arm)
        musl_arch=arm
        kernel_arch=arm
        CC=arm-linux-gnueabihf-gcc CFLAGS="-march=armv6 -marm -mfpu=vfp" \
          ./configure --prefix="/musl-${musl_arch}" --enable-wrapper=yes
        make install -j4
        ;;
    i686)
        # cross-compile musl for i686 using the system compiler on an x86_64
        # system.
        musl_arch=i686
        kernel_arch=i386
        # Specifically pass -m32 in CFLAGS and override CC when running
        # ./configure, since otherwise the script will fail to find a compiler.
        CC=gcc CFLAGS="-m32" \
          ./configure --prefix="/musl-${musl_arch}" --disable-shared --target=i686
        # unset CROSS_COMPILE when running make; otherwise the makefile will
        # call the non-existent binary 'i686-ar'.
        make CROSS_COMPILE= install -j4
        ;;
    x86_64)
        musl_arch=x86_64
        kernel_arch=x86_64
        ./configure --prefix="/musl-${musl_arch}"
        make install -j4
        ;;
    mips64)
        musl_arch=mips64
        kernel_arch=mips
        CC=mips64-linux-gnuabi64-gcc CFLAGS="-march=mips64r2 -mabi=64" \
          ./configure --prefix="/musl-${musl_arch}" --enable-wrapper=yes
        make install -j4
        ;;
    mips64el)
        musl_arch=mips64el
        kernel_arch=mips
        CC=mips64el-linux-gnuabi64-gcc CFLAGS="-march=mips64r2 -mabi=64" \
          ./configure --prefix="/musl-${musl_arch}" --enable-wrapper=yes
        make install -j4
        ;;
    *)
        echo "Unknown target arch: \"${1}\""
        exit 1
        ;;
esac


# shellcheck disable=SC2103
cd ..
rm -rf $MUSL

# Download, configure, build, and install musl-sanitized kernel headers:
KERNEL_HEADER_VER="4.4.2-2"
curl --retry 5 -L \
     "https://github.com/sabotage-linux/kernel-headers/archive/v${KERNEL_HEADER_VER}.tar.gz" | \
    tar xzf -
(
    cd kernel-headers-${KERNEL_HEADER_VER}
    make ARCH="${kernel_arch}" prefix="/musl-${musl_arch}" install -j4
)
rm -rf kernel-headers-${KERNEL_HEADER_VER}
