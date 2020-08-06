OSX_MIN_VERSION=10.14
OSX_SDK_VERSION=10.15.1
XCODE_VERSION=11.3.1
XCODE_BUILD_ID=11C505
LD64_VERSION=530

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
clang_resource_dir=$(build_prefix)/lib/clang/$(native_cctools_clang_version)
else
clang_resource_dir=$(shell clang -print-resource-dir)
endif

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
#     -stdlib=libc++ -nostdinc++ -Xclang -cxx-isystem$(OSX_SDK)/usr/include/c++/v1
#
#         Forces clang to use the libc++ headers from our SDK and completely
#         forget about the libc++ headers from the standard directories
#
#         TODO: Once we start requiring a clang version that has the
#         -stdlib++-isystem<directory> flag first introduced here:
#         https://reviews.llvm.org/D64089, we should use that instead. Read the
#         differential summary there for more details.
#
#     -Xclang -*system<path_a> \
#     -Xclang -*system<path_b> \
#     -Xclang -*system<path_c> ...
#
#         Adds path_a, path_b, and path_c to the bottom of clang's list of
#         include search paths. This is used to explicitly specify the list of
#         system include search paths and its ordering, rather than rely on
#         clang's autodetection routine. This routine has been shown to:
#             1. Fail to pickup libc++ headers in $SYSROOT/usr/include/c++/v1
#                when clang was built manually (see: https://github.com/bitcoin/bitcoin/pull/17919#issuecomment-656785034)
#             2. Fail to pickup C headers in $SYSROOT/usr/include when
#                C_INCLUDE_DIRS was specified at configure time (see: https://gist.github.com/dongcarl/5cdc6990b7599e8a5bf6d2a9c70e82f9)
#
#         Talking directly to cc1 with -Xclang here grants us access to specify
#         more granular categories for these system include search paths, and we
#         can use the correct categories that these search paths would have been
#         placed in if the autodetection routine had worked correctly. (see:
#         https://gist.github.com/dongcarl/5cdc6990b7599e8a5bf6d2a9c70e82f9#the-treatment)
#
#         Furthermore, it places these search paths after any "non-Xclang"
#         specified search paths. This prevents any additional clang options or
#         environment variables from coming after or in between these system
#         include search paths, as that would be wrong in general but would also
#         break #include_next's.
#
darwin_CC=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
              -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
              -u LIBRARY_PATH \
            clang --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
              -B$(build_prefix)/bin -mlinker-version=$(LD64_VERSION) \
              -fuse-ld=$(build_prefix)/bin/x86_64-apple-darwin16-ld \
              --sysroot=$(OSX_SDK) \
              -Xclang -internal-externc-isystem$(clang_resource_dir)/include \
              -Xclang -internal-externc-isystem$(OSX_SDK)/usr/include
darwin_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH \
             clang++ --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
               -B$(build_prefix)/bin -mlinker-version=$(LD64_VERSION) \
               -fuse-ld=$(build_prefix)/bin/x86_64-apple-darwin16-ld \
               --sysroot=$(OSX_SDK) \
               -stdlib=libc++ -nostdinc++ \
               -Xclang -cxx-isystem$(OSX_SDK)/usr/include/c++/v1 \
               -Xclang -internal-externc-isystem$(clang_resource_dir)/include \
               -Xclang -internal-externc-isystem$(OSX_SDK)/usr/include

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
