OSX_MIN_VERSION=14.0
OSX_SDK_VERSION=14.0
XCODE_VERSION=26.1.1
XCODE_BUILD_ID=17B100
LLD_VERSION=711

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

clang_prog=$(shell command -v clang)
clangxx_prog=$(shell command -v clang++)

darwin_AR=$(shell command -v llvm-ar)
darwin_NM=$(shell command -v llvm-nm)
darwin_OBJCOPY=$(shell command -v llvm-objcopy)
darwin_OBJDUMP=$(shell command -v llvm-objdump)
darwin_RANLIB=$(shell command -v llvm-ranlib)
darwin_STRIP=$(shell command -v llvm-strip)

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
#
#     -Xclang -fno-cxx-modules
#
#         Disable C++ modules. We don't use these, and modules cause definition issues
#         in the SDK, where __has_feature(modules) is used to define USE_CLANG_TYPES,
#         which is in turn used as an include guard.

darwin_CC=$(clang_prog) --target=$(host) \
              -isysroot$(OSX_SDK) -nostdlibinc \
              -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CXX=$(clangxx_prog) --target=$(host) \
               -isysroot$(OSX_SDK) -nostdlibinc \
               -iwithsysroot/usr/include/c++/v1 \
               -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CFLAGS=-mmacos-version-min=$(OSX_MIN_VERSION)
darwin_CXXFLAGS=-mmacos-version-min=$(OSX_MIN_VERSION) -Xclang -fno-cxx-modules
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
