# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Gets the version of the MinGW-w64 used by the C++ compiler.
# Sets ${version_var} in the parent scope, or leaves it unset if
# the `_mingw_mac.h` header cannot be located or parsed.
function(get_mingw_version version_var)
  if(NOT MINGW)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} is invoked in non-MinGW context.")
  endif()
  find_file(version_file _mingw_mac.h
    HINTS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
    NO_CACHE
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(EXISTS ${version_file})
    file(STRINGS ${version_file} contents REGEX "#define __MINGW64_VERSION")
    if(contents MATCHES "#define __MINGW64_VERSION_MAJOR ([0-9]+)")
      set(version_major ${CMAKE_MATCH_1})
    endif()
    if(contents MATCHES "#define __MINGW64_VERSION_MINOR ([0-9]+)")
      set(version_minor ${CMAKE_MATCH_1})
    endif()
    if(contents MATCHES "#define __MINGW64_VERSION_BUGFIX ([0-9]+)")
      set(version_bugfix ${CMAKE_MATCH_1})
    endif()
    set(${version_var} ${version_major}.${version_minor}.${version_bugfix} PARENT_SCOPE)
  endif()
endfunction()
