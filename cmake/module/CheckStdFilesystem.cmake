# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# GCC 8.x (libstdc++) requires -lstdc++fs
# Clang 8.x (libc++) requires -lc++fs

function(check_std_filesystem)
  set(source "
    #include <filesystem>

    int main()
    {
      (void)std::filesystem::current_path().root_name();
    }
  ")

  include(CheckCXXSourceCompiles)
  check_cxx_source_links("${source}" STD_FILESYSTEM_NO_EXTRA_LIBS_NEEDED)
  if(STD_FILESYSTEM_NO_EXTRA_LIBS_NEEDED)
    return()
  endif()

  add_library(std_filesystem INTERFACE)
  check_cxx_source_links_with_libs(stdc++fs "${source}" STD_FILESYSTEM_NEEDS_LINK_TO_LIBSTDCXXFS)
  if(STD_FILESYSTEM_NEEDS_LINK_TO_LIBSTDCXXFS)
    target_link_libraries(std_filesystem INTERFACE stdc++fs)
    return()
  endif()
  check_cxx_source_links_with_libs(c++fs "${source}" STD_FILESYSTEM_NEEDS_LINK_TO_LIBCXXFS)
  if(STD_FILESYSTEM_NEEDS_LINK_TO_LIBCXXFS)
    target_link_libraries(std_filesystem INTERFACE c++fs)
    return()
  endif()
  message(FATAL_ERROR "Cannot figure out how to use std::filesystem.")
endfunction()
