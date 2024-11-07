OSX_MIN_VERSION=13.0
OSX_SDK_VERSION=14.0
XCODE_VERSION=15.0
XCODE_BUILD_ID=15A240d
LLD_VERSION=711

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

# We can't just use $(shell command -v clang) because GNU Make handles builtins
# in a special way and doesn't know that `command` is a POSIX-standard builtin
# prior to 1af314465e5dfe3e8baa839a32a72e83c04f26ef, first released in v4.2.90.
# At the time of writing, GNU Make v4.2.1 is still being used in supported
# distro releases.
#
# Source: https://lists.gnu.org/archive/html/bug-make/2017-11/msg00017.html
clang_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang")
clangxx_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang++")

darwin_AR=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ar")
darwin_DSYMUTIL=$(shell $(SHELL) $(.SHELLFLAGS) "command -v dsymutil")
darwin_NM=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-nm")
darwin_OBJDUMP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-objdump")
darwin_RANLIB=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-ranlib")
darwin_STRIP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-strip")

# Flag explanations:
#
#     -mlinker-version
#
#         Ensures that modern linker features are enabled. See here for more
#         details: https://github.com/bitcoin/bitcoin/pull/19407.
#
#     -isysroot$(OSX_SDK) -nostdlibinc
#
#         Disable default include paths built into the compiler as well as
#         those normally included for libc and libc++. The only path that
#         remains implicitly is the clang resource dir.
#
#     -iwithsysroot / -iframeworkwithsysroot
#
#         Adds the desired paths from the SDK
#
#     -platform_version
#
#         Indicate to the linker the platform, the oldest supported version,
#         and the SDK used.
#
#     -no_adhoc_codesign
#
#         Disable adhoc codesigning (for now) when using LLVM tooling, to avoid
#         non-determinism issues with the Identifier field.

darwin_CC=$(clang_prog) --target=$(host) \
              -isysroot$(OSX_SDK) -nostdlibinc \
              -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CXX=$(clangxx_prog) --target=$(host) \
               -isysroot$(OSX_SDK) -nostdlibinc \
               -iwithsysroot/usr/include/c++/v1 \
               -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CFLAGS=-pipe -std=$(C_STANDARD) -mmacos-version-min=$(OSX_MIN_VERSION)
darwin_CXXFLAGS=-pipe -std=$(CXX_STANDARD) -mmacos-version-min=$(OSX_MIN_VERSION)
darwin_LDFLAGS=-Wl,-platform_version,macos,$(OSX_MIN_VERSION),$(OSX_SDK_VERSION)

ifneq ($(build_os),darwin)
darwin_CFLAGS += -mlinker-version=$(LLD_VERSION)
darwin_CXXFLAGS += -mlinker-version=$(LLD_VERSION)
darwin_LDFLAGS += -Wl,-no_adhoc_codesign -fuse-ld=lld
endif

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1 -g
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_cmake_system_name=Darwin
# Darwin version, which corresponds to OSX_MIN_VERSION.
# See https://en.wikipedia.org/wiki/Darwin_(operating_system)
darwin_cmake_system_version=20.1
