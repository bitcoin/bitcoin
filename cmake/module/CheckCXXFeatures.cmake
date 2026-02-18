# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

#Checks for C++ features required to compile Bitcoin Core.

include(CheckCXXSourceCompiles)

function(check_cxx_features)
  set(CMAKE_REQUIRED_QUIET TRUE)

  message(STATUS "Checking for required C++ features")

  # Checks for Class Template Argument Deduction for aggregate types - used in src/util/overloaded.h
  check_cxx_source_compiles("
    #include <variant>

    template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };

    int main() {
      std::variant<int, double> v = 42;
      return std::visit(Overloaded{
        [](int) { return 0; },
        [](double) { return 1; }
      }, v);
    }
  " HAVE_CTAD_FOR_AGGREGATES)

  if(NOT HAVE_CTAD_FOR_AGGREGATES)
    message(FATAL_ERROR
      "Compiler lacks Class Template Argument Deduction (CTAD) for aggregates.\n"
      "This C++ feature is required for src/util/overloaded.h.\n"
      "You are probably using an old compiler version\n"
      "The recommended compiler versions can be checked in:\n"
      "  - GCC -> ${MIN_GCC_DOCS}\n"
      "  - Clang -> ${MIN_CLANG_DOCS}\n"
      "  - MSVC -> ${MIN_MSVC_DOCS}\n"
    )
  endif()

  message(STATUS "Checking for required C++ features - done")

endfunction()
