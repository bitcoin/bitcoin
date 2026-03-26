FREEBSD_VERSION ?= 15.0
FREEBSD_SDK=$(SDK_PATH)/freebsd-$(host)-$(FREEBSD_VERSION)/

clang_prog=$(shell command -v clang)
clangxx_prog=$(shell command -v clang++)

freebsd_AR=$(shell command -v llvm-ar)
freebsd_NM=$(shell command -v llvm-nm)
freebsd_OBJCOPY=$(shell command -v llvm-objcopy)
freebsd_OBJDUMP=$(shell command -v llvm-objdump)
freebsd_RANLIB=$(shell command -v llvm-ranlib)
freebsd_STRIP=$(shell command -v llvm-strip)


freebsd_CC=$(clang_prog) --target=$(host) \
              --sysroot=$(FREEBSD_SDK)

freebsd_CXX=$(clangxx_prog) --target=$(host) \
              --sysroot=$(FREEBSD_SDK) -stdlib=libc++

freebsd_CFLAGS=
freebsd_CXXFLAGS=
freebsd_LDFLAGS=-fuse-ld=lld

freebsd_release_CFLAGS=-O2
freebsd_release_CXXFLAGS=$(freebsd_release_CFLAGS)

freebsd_debug_CFLAGS=-O1 -g
freebsd_debug_CXXFLAGS=$(freebsd_debug_CFLAGS)

freebsd_cmake_system_name=FreeBSD
