# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Checks for C++ features required to compile Bitcoin Core.

include(CheckCXXSourceCompiles)

function(check_cxx_features)
  set(CMAKE_REQUIRED_QUIET TRUE)

  message(STATUS "Checking for required C++ features")

  # Checks for Class Template Argument Deduction for alias templates.
  check_cxx_source_compiles("
    template<class T> struct Template { Template(T) {} };
    template<class T> using Alias = Template<T>;

    int main() {
      Alias value{42};
      return sizeof(value);
    }
  " HAVE_CTAD_FOR_ALIAS_TEMPLATES)

  if(NOT HAVE_CTAD_FOR_ALIAS_TEMPLATES)
    message(FATAL_ERROR
      "Compiler lacks Class Template Argument Deduction (CTAD) for alias templates.\n"
      "You are probably using an old compiler version\n"
      "The recommended compiler versions can be checked in\n"
      "doc/dependencies.md#compiler.\n"
    )
  endif()

  message(STATUS "Checking for required C++ features - done")

endfunction()
