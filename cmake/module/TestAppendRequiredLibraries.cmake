# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Illumos/SmartOS requires linking with -lsocket if
# using getifaddrs & freeifaddrs.
# See:
# - https://github.com/bitcoin/bitcoin/pull/21486
# - https://smartos.org/man/3socket/getifaddrs
function(test_append_socket_library target)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}() called with non-existent target \"${target}\".")
  endif()

  set(check_socket_source "
    #include <sys/types.h>
    #include <ifaddrs.h>

    int main() {
      struct ifaddrs* ifaddr;
      getifaddrs(&ifaddr);
      freeifaddrs(ifaddr);
    }
  ")

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${check_socket_source}" IFADDR_LINKS_WITHOUT_LIBSOCKET)
  if(NOT IFADDR_LINKS_WITHOUT_LIBSOCKET)
    include(CheckSourceCompilesWithFlags)
    check_cxx_source_compiles_with_flags("${check_socket_source}" IFADDR_NEEDS_LINK_TO_LIBSOCKET
      LINK_LIBRARIES socket
    )
    if(IFADDR_NEEDS_LINK_TO_LIBSOCKET)
      target_link_libraries(${target} INTERFACE socket)
    else()
      message(FATAL_ERROR "Cannot figure out how to use getifaddrs/freeifaddrs.")
    endif()
  endif()
  set(HAVE_DECL_GETIFADDRS TRUE PARENT_SCOPE)
  set(HAVE_DECL_FREEIFADDRS TRUE PARENT_SCOPE)
endfunction()

# Clang, when building for 32-bit,
# and linking against libstdc++, requires linking with
# -latomic if using the C++ atomic library.
# Can be tested with: clang++ -std=c++20 test.cpp -m32
#
# Sourced from http://bugs.debian.org/797228
function(test_append_atomic_library target)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}() called with non-existent target \"${target}\".")
  endif()

  set(check_atomic_source "
    #include <atomic>
    #include <cstdint>
    #include <chrono>

    using namespace std::chrono_literals;

    int main() {
      std::atomic<bool> lock{true};
      lock.exchange(false);

      std::atomic<std::chrono::seconds> t{0s};
      t.store(2s);
      auto t1 = t.load();
      t.compare_exchange_strong(t1, 3s);

      std::atomic<double> d{};
      d.store(3.14);
      auto d1 = d.load();

      std::atomic<int64_t> a{};
      int64_t v = 5;
      int64_t r = a.fetch_add(v);
      return static_cast<int>(r);
    }
  ")

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${check_atomic_source}" STD_ATOMIC_LINKS_WITHOUT_LIBATOMIC)
  if(NOT STD_ATOMIC_LINKS_WITHOUT_LIBATOMIC)
    include(CheckSourceCompilesWithFlags)
    check_cxx_source_compiles_with_flags("${check_atomic_source}" STD_ATOMIC_NEEDS_LINK_TO_LIBATOMIC
      LINK_LIBRARIES atomic
    )
    if(STD_ATOMIC_NEEDS_LINK_TO_LIBATOMIC)
      target_link_libraries(${target} INTERFACE atomic)
    else()
      message(FATAL_ERROR "Cannot figure out how to use std::atomic.")
    endif()
  endif()
endfunction()
