OSX_MIN_VERSION=11.0
OSX_SDK_VERSION=14.0
XCODE_VERSION=15.0
XCODE_BUILD_ID=15A240d
LD64_VERSION=711

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

CC = clang
CXX = clang++

darwin_AR=llvm-ar
darwin_NM=llvm-nm
darwin_OBJDUMP=llvm-objdump
darwin_RANLIB=llvm-ranlib
darwin_STRIP=llvm-strip

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

host_and_SDK = --target=$(host) \
              -isysroot$(OSX_SDK) -nostdlibinc \
              -iwithsysroot/usr/include/c++/v1 \
              -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

override CC += $(host_and_SDK)
override CXX += $(host_and_SDK)

darwin_CFLAGS=-pipe -std=$(C_STANDARD) -mmacosx-version-min=$(OSX_MIN_VERSION)
darwin_CXXFLAGS=-pipe -std=$(CXX_STANDARD) -mmacosx-version-min=$(OSX_MIN_VERSION)
darwin_LDFLAGS=-Wl,-platform_version,macos,$(OSX_MIN_VERSION),$(OSX_SDK_VERSION)

ifneq ($(build_os),darwin)
darwin_CFLAGS += -mlinker-version=$(LD64_VERSION)
darwin_CXXFLAGS += -mlinker-version=$(LD64_VERSION)
darwin_LDFLAGS += -fuse-ld=lld
endif

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_cmake_system=Darwin
