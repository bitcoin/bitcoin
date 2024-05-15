openbsd_CFLAGS=-pipe -std=$(C_STANDARD)
openbsd_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

openbsd_release_CFLAGS=-O2
openbsd_release_CXXFLAGS=$(openbsd_release_CFLAGS)

openbsd_debug_CFLAGS=-O1
openbsd_debug_CXXFLAGS=$(openbsd_debug_CFLAGS)

ifeq (86,$(findstring 86,$(build_arch)))
i686_openbsd_CC=clang -m32
i686_openbsd_CXX=clang++ -m32
i686_openbsd_AR=ar
i686_openbsd_RANLIB=ranlib
i686_openbsd_NM=nm
i686_openbsd_STRIP=strip

x86_64_openbsd_CC=clang -m64
x86_64_openbsd_CXX=clang++ -m64
x86_64_openbsd_AR=ar
x86_64_openbsd_RANLIB=ranlib
x86_64_openbsd_NM=nm
x86_64_openbsd_STRIP=strip
else
i686_openbsd_CC=$(default_host_CC) -m32
i686_openbsd_CXX=$(default_host_CXX) -m32
x86_64_openbsd_CC=$(default_host_CC) -m64
x86_64_openbsd_CXX=$(default_host_CXX) -m64
endif

openbsd_cmake_system_name=OpenBSD
