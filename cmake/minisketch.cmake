# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(add_minisketch subdir)
  message("")
  message("Configuring minisketch subtree...")
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
  # Hard-code subtree options; they won’t appear in the CMake cache.
  set(MINISKETCH_INSTALL OFF)
  set(MINISKETCH_BUILD_TESTS OFF)
  set(MINISKETCH_BUILD_BENCHMARK OFF)
  set(MINISKETCH_FIELDS 32)
  add_subdirectory(${subdir} EXCLUDE_FROM_ALL)
  target_link_libraries(minisketch
    PRIVATE
      core_interface
  )
endfunction()
