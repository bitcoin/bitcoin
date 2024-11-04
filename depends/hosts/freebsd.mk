freebsd_CFLAGS=-pipe -std=$(C_STANDARD)
freebsd_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

ifneq ($(LTO),)
freebsd_CFLAGS += -flto
freebsd_CXXFLAGS += -flto
freebsd_LDFLAGS += -flto
endif

freebsd_release_CFLAGS=-O2
freebsd_release_CXXFLAGS=$(freebsd_release_CFLAGS)

freebsd_debug_CFLAGS=-O1
freebsd_debug_CXXFLAGS=$(freebsd_debug_CFLAGS)

ifeq (86,$(findstring 86,$(build_arch)))
i686_freebsd_CC=clang -m32
i686_freebsd_CXX=clang++ -m32
i686_freebsd_AR=ar
i686_freebsd_RANLIB=ranlib
i686_freebsd_NM=nm
i686_freebsd_STRIP=strip

x86_64_freebsd_CC=clang -m64
x86_64_freebsd_CXX=clang++ -m64
x86_64_freebsd_AR=ar
x86_64_freebsd_RANLIB=ranlib
x86_64_freebsd_NM=nm
x86_64_freebsd_STRIP=strip
else
i686_freebsd_CC=$(default_host_CC) -m32
i686_freebsd_CXX=$(default_host_CXX) -m32
x86_64_freebsd_CC=$(default_host_CC) -m64
x86_64_freebsd_CXX=$(default_host_CXX) -m64
endif

freebsd_cmake_system=FreeBSD
