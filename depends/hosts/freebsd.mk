FREEBSD_VERSION ?= 15.0
FREEBSD_SDK=$(SDK_PATH)/freebsd-$(host)-$(FREEBSD_VERSION)/

# We can't just use $(shell command -v clang) because GNU Make handles builtins
# in a special way and doesn't know that `command` is a POSIX-standard builtin
# prior to 1af314465e5dfe3e8baa839a32a72e83c04f26ef, first released in v4.2.90.
# At the time of writing, GNU Make v4.2.1 is still being used in supported
# distro releases.
#
# Source: https://lists.gnu.org/archive/html/bug-make/2017-11/msg00017.html
clang_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang")
clangxx_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang++")

freebsd_AR=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ar")
freebsd_NM=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-nm")
freebsd_OBJCOPY=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-objcopy")
freebsd_OBJDUMP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-objdump")
freebsd_RANLIB=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ranlib")
freebsd_STRIP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-strip")


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
