OSX_MIN_VERSION=10.14
OSX_SDK_VERSION=10.15.1
XCODE_VERSION=11.3.1
XCODE_BUILD_ID=11C505
LD64_VERSION=530

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

# Flag explanations:
#
#     -mlinker-version
#
#         Ensures that modern linker features are enabled. See here for more
#         details: https://github.com/bitcoin/bitcoin/pull/19407.
#
#     -B$(build_prefix)/bin
#
#         Explicitly point to our binaries (e.g. cctools) so that they are
#         ensured to be found and preferred over other possibilities.
#
#     -nostdinc++ -isystem $(OSX_SDK)/usr/include/c++/v1
#
#         Forces clang to use the libc++ headers from our SDK and completely
#         forget about the libc++ headers from the standard directories
#
#         TODO: Once we start requiring a clang version that has the
#         -stdlib++-isystem<directory> flag first introduced here:
#         https://reviews.llvm.org/D64089, we should use that instead. Read the
#         differential summary there for more details.
#
darwin_CC=clang -target $(host) -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(OSX_SDK) -mlinker-version=$(LD64_VERSION) -B$(build_prefix)/bin
darwin_CXX=clang++ -target $(host) -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(OSX_SDK) -stdlib=libc++ -mlinker-version=$(LD64_VERSION) -B$(build_prefix)/bin -nostdinc++ -isystem $(OSX_SDK)/usr/include/c++/v1

darwin_CFLAGS=-pipe
darwin_CXXFLAGS=$(darwin_CFLAGS)

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_native_binutils=native_cctools
ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
darwin_native_toolchain=native_cctools
else
darwin_native_toolchain=
endif

darwin_cmake_system=Darwin
