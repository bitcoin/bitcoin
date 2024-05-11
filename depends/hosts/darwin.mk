OSX_MIN_VERSION=11.0
OSX_SDK_VERSION=14.0
XCODE_VERSION=15.0
XCODE_BUILD_ID=15A240d
LD64_VERSION=711

OSX_SDK=$(SDK_PATH)/Xcode-$(XCODE_VERSION)-$(XCODE_BUILD_ID)-extracted-SDK-with-libcxx-headers

darwin_native_binutils=native_cctools

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
# FORCE_USE_SYSTEM_CLANG is empty, so we use our depends-managed, pinned clang
# from llvm.org

# Clang is a dependency of native_cctools when FORCE_USE_SYSTEM_CLANG is empty
darwin_native_toolchain=native_cctools

clang_prog=$(build_prefix)/bin/clang
clangxx_prog=$(clang_prog)++
llvm_config_prog=$(build_prefix)/bin/llvm-config

darwin_OBJDUMP=$(build_prefix)/bin/$(host)-objdump

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
llvm_config_prog=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-config")

llvm_lib_dir=$(shell $(llvm_config_prog) --libdir)

darwin_OBJDUMP=$(shell $(SHELL) $(.SHELLFLAGS) "command -v llvm-objdump")
endif

cctools_TOOLS=AR RANLIB STRIP NM DSYMUTIL

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

darwin_CC=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
              -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
              -u LIBRARY_PATH \
              $(clang_prog) --target=$(host) \
              -B$(build_prefix)/bin \
              -isysroot$(OSX_SDK) -nostdlibinc \
              -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH \
               $(clangxx_prog) --target=$(host) \
               -B$(build_prefix)/bin \
               -isysroot$(OSX_SDK) -nostdlibinc \
               -iwithsysroot/usr/include/c++/v1 \
               -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks

darwin_CFLAGS=-pipe -std=$(C_STANDARD) -mmacosx-version-min=$(OSX_MIN_VERSION)
darwin_CXXFLAGS=-pipe -std=$(CXX_STANDARD) -mmacosx-version-min=$(OSX_MIN_VERSION)
darwin_LDFLAGS=-Wl,-platform_version,macos,$(OSX_MIN_VERSION),$(OSX_SDK_VERSION)

ifneq ($(build_os),darwin)
darwin_CFLAGS += -mlinker-version=$(LD64_VERSION)
darwin_CXXFLAGS += -mlinker-version=$(LD64_VERSION)
endif

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1 -g
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_cmake_system=Darwin
