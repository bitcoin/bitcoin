OSX_MIN_VERSION=10.14
OSX_SDK_VERSION=10.15.1
XCODE_VERSION=11.3.1
XCODE_BUILD_ID=11C505
LD64_VERSION=530

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

darwin_native_binutils=native_cctools

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
# FORCE_USE_SYSTEM_CLANG is empty, so we use our depends-managed, pinned clang
# from llvm.org

# Clang is a dependency of native_cctools when FORCE_USE_SYSTEM_CLANG is empty
darwin_native_toolchain=native_cctools

clang_prog=$(build_prefix)/bin/clang
clangxx_prog=$(clang_prog)++

clang_resource_dir=$(build_prefix)/lib/clang/$(native_clang_version)
else
# FORCE_USE_SYSTEM_CLANG is non-empty, so we use the clang from the user's
# system

darwin_native_toolchain=

# We can't just use $(shell command -v clang) because GNU Make handles builtins
# in a special way and doesn't know that `command` is a POSIX-standard builtin
# prior to 1af314465e5dfe3e8baa839a32a72e83c04f26ef, first released in v4.2.90.
# At the time of writing, GNU Make v4.2.1 is still being used in supported
# distro releases.
#
# Source: https://lists.gnu.org/archive/html/bug-make/2017-11/msg00017.html
clang_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang")
clangxx_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v clang++")

clang_resource_dir=$(shell clang -print-resource-dir)
endif

cctools_TOOLS=AR RANLIB STRIP NM LIBTOOL OTOOL INSTALL_NAME_TOOL

# Make-only lowercase function
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

# For well-known tools provided by cctools, make sure that their well-known
# variable is set to the full path of the tool, just like how AC_PATH_{TOO,PROG}
# would.
$(foreach TOOL,$(cctools_TOOLS),$(eval darwin_$(TOOL) = $$(build_prefix)/bin/$$(host)-$(call lc,$(TOOL))))

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
            $(clang_prog) --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
              -B$(build_prefix)/bin -mlinker-version=$(LD64_VERSION) \
              --sysroot=$(OSX_SDK) \
              -Xclang -internal-externc-isystem$(clang_resource_dir)/include \
              -Xclang -internal-externc-isystem$(OSX_SDK)/usr/include
darwin_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH \
             $(clangxx_prog) --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
               -B$(build_prefix)/bin -mlinker-version=$(LD64_VERSION) \
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

darwin_cmake_system=Darwin
