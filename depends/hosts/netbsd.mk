netbsd_CFLAGS=-pipe
netbsd_CXXFLAGS=$(netbsd_CFLAGS)

netbsd_release_CFLAGS=-O2
netbsd_release_CXXFLAGS=$(netbsd_release_CFLAGS)

netbsd_debug_CFLAGS=-O1
netbsd_debug_CXXFLAGS=$(netbsd_debug_CFLAGS)

ifeq (86,$(findstring 86,$(build_arch)))
i686_netbsd_CC=gcc -m32
i686_netbsd_CXX=g++ -m32
i686_netbsd_AR=ar
i686_netbsd_RANLIB=ranlib
i686_netbsd_NM=nm
i686_netbsd_STRIP=strip

x86_64_netbsd_CC=gcc -m64
x86_64_netbsd_CXX=g++ -m64
x86_64_netbsd_AR=ar
x86_64_netbsd_RANLIB=ranlib
x86_64_netbsd_NM=nm
x86_64_netbsd_STRIP=strip
else
i686_netbsd_CC=$(default_host_CC) -m32
i686_netbsd_CXX=$(default_host_CXX) -m32
x86_64_netbsd_CC=$(default_host_CC) -m64
x86_64_netbsd_CXX=$(default_host_CXX) -m64
endif

netbsd_cmake_system=NetBSD
