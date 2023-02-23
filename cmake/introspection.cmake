# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)
include(TestBigEndian)

test_big_endian(WORDS_BIGENDIAN)

# The following HAVE_{HEADER}_H variables go to the bitcoin-config.h header.
check_include_file_cxx(byteswap.h HAVE_BYTESWAP_H)
check_include_file_cxx(endian.h HAVE_ENDIAN_H)
check_include_file_cxx(sys/endian.h HAVE_SYS_ENDIAN_H)
check_include_file_cxx(sys/prctl.h HAVE_SYS_PRCTL_H)
check_include_file_cxx(sys/resources.h HAVE_SYS_RESOURCES_H)
check_include_file_cxx(sys/vmmeter.h HAVE_SYS_VMMETER_H)
check_include_file_cxx(vm/vm_param.h HAVE_VM_VM_PARAM_H)

check_cxx_source_compiles("
  int main()
  {
    (void) __builtin_clzl(0);
  }
  " HAVE_BUILTIN_CLZL
)

check_cxx_source_compiles("
  int main()
  {
    (void) __builtin_clzll(0);
  }
  " HAVE_BUILTIN_CLZLL
)

check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" HAVE_O_CLOEXEC)

if(HAVE_BYTESWAP_H)
  check_cxx_symbol_exists(bswap_16 "byteswap.h" HAVE_DECL_BSWAP_16)
  check_cxx_symbol_exists(bswap_32 "byteswap.h" HAVE_DECL_BSWAP_32)
  check_cxx_symbol_exists(bswap_64 "byteswap.h" HAVE_DECL_BSWAP_64)
endif()

if(HAVE_ENDIAN_H OR HAVE_SYS_ENDIAN_H)
  if(HAVE_ENDIAN_H)
    set(ENDIAN_HEADER "endian.h")
  else()
    set(ENDIAN_HEADER "sys/endian.h")
  endif()
  check_cxx_symbol_exists(be16toh ${ENDIAN_HEADER} HAVE_DECL_BE16TOH)
  check_cxx_symbol_exists(be32toh ${ENDIAN_HEADER} HAVE_DECL_BE32TOH)
  check_cxx_symbol_exists(be64toh ${ENDIAN_HEADER} HAVE_DECL_BE64TOH)
  check_cxx_symbol_exists(htobe16 ${ENDIAN_HEADER} HAVE_DECL_HTOBE16)
  check_cxx_symbol_exists(htobe32 ${ENDIAN_HEADER} HAVE_DECL_HTOBE32)
  check_cxx_symbol_exists(htobe64 ${ENDIAN_HEADER} HAVE_DECL_HTOBE64)
  check_cxx_symbol_exists(htole16 ${ENDIAN_HEADER} HAVE_DECL_HTOLE16)
  check_cxx_symbol_exists(htole32 ${ENDIAN_HEADER} HAVE_DECL_HTOLE32)
  check_cxx_symbol_exists(htole64 ${ENDIAN_HEADER} HAVE_DECL_HTOLE64)
  check_cxx_symbol_exists(le16toh ${ENDIAN_HEADER} HAVE_DECL_LE16TOH)
  check_cxx_symbol_exists(le32toh ${ENDIAN_HEADER} HAVE_DECL_LE32TOH)
  check_cxx_symbol_exists(le64toh ${ENDIAN_HEADER} HAVE_DECL_LE64TOH)
endif()

check_include_file_cxx(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
  check_cxx_symbol_exists(fdatasync "unistd.h" HAVE_FDATASYNC)
  check_cxx_symbol_exists(fork "unistd.h" HAVE_DECL_FORK)
  check_cxx_symbol_exists(pipe2 "unistd.h" HAVE_DECL_PIPE2)
  check_cxx_symbol_exists(setsid "unistd.h" HAVE_DECL_SETSID)
endif()

check_include_file_cxx(sys/types.h HAVE_SYS_TYPES_H)
check_include_file_cxx(ifaddrs.h HAVE_IFADDRS_H)
if(HAVE_SYS_TYPES_H AND HAVE_IFADDRS_H)
  check_cxx_symbol_exists(freeifaddrs "sys/types.h;ifaddrs.h" HAVE_DECL_FREEIFADDRS)
  check_cxx_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" HAVE_DECL_GETIFADDRS)
  # Illumos/SmartOS requires linking with -lsocket if
  # using getifaddrs & freeifaddrs.
  # See: https://github.com/bitcoin/bitcoin/pull/21486
  if(HAVE_DECL_GETIFADDRS AND HAVE_DECL_FREEIFADDRS)
    set(check_socket_source "
      #include <sys/types.h>
      #include <ifaddrs.h>

      int main() {
        struct ifaddrs* ifaddr;
        getifaddrs(&ifaddr);
        freeifaddrs(ifaddr);
      }
    ")
    check_cxx_source_links("${check_socket_source}" IFADDR_LINKS_WITHOUT_LIBSOCKET)
    if(NOT IFADDR_LINKS_WITHOUT_LIBSOCKET)
      check_cxx_source_links_with_libs(socket "${check_socket_source}" IFADDR_NEEDS_LINK_TO_LIBSOCKET)
      if(IFADDR_NEEDS_LINK_TO_LIBSOCKET)
        link_libraries(socket)
      else()
        message(FATAL_ERROR "Cannot figure out how to use getifaddrs/freeifaddrs.")
      endif()
    endif()
  endif()
endif()

# Check for gmtime_r(), fallback to gmtime_s() if that is unavailable.
# Fail if neither are available.
check_cxx_source_compiles("
  #include <ctime>

  int main()
  {
    gmtime_r((const time_t*)nullptr, (struct tm*)nullptr);
  }
  " HAVE_GMTIME_R
)
if(NOT HAVE_GMTIME_R)
  check_cxx_source_compiles("
    #include <ctime>

    int main()
    {
      gmtime_s((struct tm*)nullptr, (const time_t*)nullptr);
    }
    " HAVE_GMTIME_S
  )
  if(NOT HAVE_GMTIME_S)
    message(FATAL_ERROR "Both gmtime_r and gmtime_s are unavailable.")
  endif()
endif()

check_cxx_symbol_exists(std::system "cstdlib" HAVE_STD_SYSTEM)
check_cxx_symbol_exists(::_wsystem "stdlib.h" HAVE__WSYSTEM)
if(HAVE_STD_SYSTEM OR HAVE__WSYSTEM)
  set(HAVE_SYSTEM 1)
endif()

check_include_file_cxx(string.h HAVE_STRING_H)
if(HAVE_STRING_H)
  check_cxx_source_compiles("
    #include <string.h>

    int main()
    {
      char buf[100];
      char* p{strerror_r(0, buf, sizeof buf)};
      (void)p;
    }
    " STRERROR_R_CHAR_P
  )
endif()

# Check for malloc_info (for memory statistics information in getmemoryinfo).
check_cxx_symbol_exists(malloc_info "malloc.h" HAVE_MALLOC_INFO)

# Check for mallopt(M_ARENA_MAX) (to set glibc arenas).
check_cxx_source_compiles("
  #include <malloc.h>

  int main()
  {
    mallopt(M_ARENA_MAX, 1);
  }
  " HAVE_MALLOPT_ARENA_MAX
)

# Check for posix_fallocate().
check_cxx_source_compiles("
  // same as in src/util/fs_helpers.cpp
  #ifdef __linux__
  #ifdef _POSIX_C_SOURCE
  #undef _POSIX_C_SOURCE
  #endif
  #define _POSIX_C_SOURCE 200112L
  #endif // __linux__
  #include <fcntl.h>

  int main()
  {
    return posix_fallocate(0, 0, 0);
  }
  " HAVE_POSIX_FALLOCATE
)

# Check for strong getauxval() support in the system headers.
check_cxx_source_compiles("
  #include <sys/auxv.h>

  int main()
  {
    getauxval(AT_HWCAP);
  }
  " HAVE_STRONG_GETAUXVAL
)

# Check for different ways of gathering OS randomness:
# - Linux getrandom()
check_cxx_source_compiles("
  #include <unistd.h>
  #include <sys/syscall.h>
  #include <linux/random.h>

  int main()
  {
    syscall(SYS_getrandom, nullptr, 32, 0);
  }
  " HAVE_SYS_GETRANDOM
)

# - BSD getentropy()
check_cxx_symbol_exists(getentropy "unistd.h;sys/random.h" HAVE_GETENTROPY_RAND)

# - BSD sysctl()
check_cxx_source_compiles("
  #include <sys/types.h>
  #include <sys/sysctl.h>

  #ifdef __linux__
  #error Don't use sysctl on Linux, it's deprecated even when it works
  #endif

  int main()
  {
    sysctl(nullptr, 2, nullptr, nullptr, nullptr, 0);
  }
  " HAVE_SYSCTL
)

# - BSD sysctl(KERN_ARND)
check_cxx_source_compiles("
  #include <sys/types.h>
  #include <sys/sysctl.h>

  #ifdef __linux__
  #error Don't use sysctl on Linux, it's deprecated even when it works
  #endif

  int main()
  {
    static int name[2] = {CTL_KERN, KERN_ARND};
    sysctl(name, 2, nullptr, nullptr, nullptr, 0);
  }
  " HAVE_SYSCTL_ARND
)

check_cxx_source_compiles("
  #include <cstdint>
  #include <type_traits>

  int main()
  {
    static_assert(std::is_same<int8_t, char>::value);
  }
  " CHAR_EQUALS_INT8
)

check_cxx_source_compiles("
  int foo(void) __attribute__((visibility(\"default\")));
  int main(){}
  " HAVE_DEFAULT_VISIBILITY_ATTRIBUTE
)

check_cxx_source_compiles("
  __declspec(dllexport) int foo(void);
  int main(){}
  " HAVE_DLLEXPORT_ATTRIBUTE
)
