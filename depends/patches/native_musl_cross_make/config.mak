TARGET =

BINUTILS_VER = 2.37
GCC_VER = 10.3.0
MUSL_VER = 1.2.3
LINUX_VER = headers-4.19.88-1

COMMON_CONFIG += --with-debug-prefix-map=$(CURDIR)=

COMMON_CONFIG += CFLAGS="-fPIC -g0 -O2"
COMMON_CONFIG += CXXFLAGS="-fPIC -g0 -O2"
COMMON_CONFIG += LDFLAGS="-s"

COMMON_CONFIG += --disable-shared
COMMON_CONFIG += --enable-static

DL_CMD = curl -C - -L -o

GCC_CONFIG += --disable-libquadmath
GCC_CONFIG += --disable-multilib
GCC_CONFIG += --disable-nls
GCC_CONFIG += --enable-default-pie
GCC_CONFIG += --enable-default-ssp
