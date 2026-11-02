# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)
include(TryAppendCXXFlags)

#[=[
Check once if C++ source code can be compiled with no diagnostics.

Options:

  CXXFLAGS - A list of additional flags to pass to the compiler.

  LDFLAGS - A list of additional flags to pass to the linker.

  LINK_LIBRARIES - A list of libraries to add to the link command.

For historical reasons, among the CMake `CMAKE_REQUIRED_*` variables that influence
`check_cxx_source_compiles()`, only `CMAKE_REQUIRED_FLAGS` is a string rather than
a list. Additionally, `target_compile_options()` also expects a list of options.

The `check_cxx_source_compiles_with_flags()` function handles this case and accepts
`CXXFLAGS` as a list, simplifying the code at the caller site.

#]=]
function(check_cxx_source_compiles_with_flags source result_var)
  cmake_parse_arguments(PARSE_ARGV 2 _ "" "" "CXXFLAGS;LDFLAGS;LINK_LIBRARIES")
  list(APPEND __CXXFLAGS ${working_compiler_werror_flag})
  list(JOIN __CXXFLAGS " " CMAKE_REQUIRED_FLAGS)
  set(CMAKE_REQUIRED_LINK_OPTIONS ${__LDFLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${__LINK_LIBRARIES})
  check_cxx_source_compiles("${source}" ${result_var})
  set(${result_var} ${${result_var}} PARENT_SCOPE)
endfunction()
