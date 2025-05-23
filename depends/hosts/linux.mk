linux_CFLAGS=-pipe -std=$(C_STANDARD)
linux_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

ifneq ($(LTO),)
linux_AR = $(host_toolchain)gcc-ar
linux_NM = $(host_toolchain)gcc-nm
linux_RANLIB = $(host_toolchain)gcc-ranlib
endif

linux_release_CFLAGS=-O2
linux_release_CXXFLAGS=$(linux_release_CFLAGS)

linux_debug_CFLAGS=-O1 -g
linux_debug_CXXFLAGS=$(linux_debug_CFLAGS)

# https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode.html
linux_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

# https://libcxx.llvm.org/Hardening.html
linux_debug_CPPFLAGS+=-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG

linux_cmake_system_name=Linux
# Refer to doc/dependencies.md for the minimum required kernel.
linux_cmake_system_version=3.17.0
