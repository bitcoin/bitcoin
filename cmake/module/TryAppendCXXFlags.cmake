# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)

#[=[
Add language-wide flags, which will be passed to all invocations of the compiler.
This includes invocations that drive compiling and those that drive linking.

Usage examples:

  try_append_cxx_flags("-Wformat -Wformat-security" VAR warn_cxx_flags)


  try_append_cxx_flags("-Wsuggest-override" VAR warn_cxx_flags
    SOURCE "struct A { virtual void f(); }; struct B : A { void f() final; };"
  )


  try_append_cxx_flags("-fsanitize=${SANITIZERS}" TARGET core_interface
    RESULT_VAR cxx_supports_sanitizers
  )
  if(NOT cxx_supports_sanitizers)
    message(FATAL_ERROR "Compiler did not accept requested flags.")
  endif()


  try_append_cxx_flags("-Wunused-parameter" TARGET core_interface
    IF_CHECK_PASSED "-Wno-unused-parameter"
  )


In configuration output, this function prints a string by the following pattern:

  -- Performing Test CXX_SUPPORTS_[flags]
  -- Performing Test CXX_SUPPORTS_[flags] - Success

]=]
function(try_append_cxx_flags flags)
  cmake_parse_arguments(PARSE_ARGV 1
    TACXXF                            # prefix
    "SKIP_LINK"                       # options
    "TARGET;VAR;SOURCE;RESULT_VAR"    # one_value_keywords
    "IF_CHECK_PASSED"                 # multi_value_keywords
  )

  set(flags_as_string "${flags}")
  separate_arguments(flags)

  string(MAKE_C_IDENTIFIER "${flags_as_string}" id_string)
  string(TOUPPER "${id_string}" id_string)

  set(source "int main() { return 0; }")
  if(DEFINED TACXXF_SOURCE AND NOT TACXXF_SOURCE STREQUAL source)
    set(source "${TACXXF_SOURCE}")
    string(SHA256 source_hash "${source}")
    string(SUBSTRING ${source_hash} 0 4 source_hash_head)
    string(APPEND id_string _${source_hash_head})
  endif()

  # This avoids running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  set(CMAKE_REQUIRED_FLAGS "${flags_as_string} ${working_compiler_werror_flag}")
  set(compiler_result CXX_SUPPORTS_${id_string})
  check_cxx_source_compiles("${source}" ${compiler_result})

  if(${compiler_result})
    if(DEFINED TACXXF_IF_CHECK_PASSED)
      if(DEFINED TACXXF_TARGET)
        target_compile_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_PASSED})
      endif()
      if(DEFINED TACXXF_VAR)
        list(JOIN TACXXF_IF_CHECK_PASSED " " flags_if_check_passed_as_string)
        string(STRIP "${${TACXXF_VAR}} ${flags_if_check_passed_as_string}" ${TACXXF_VAR})
      endif()
    else()
      if(DEFINED TACXXF_TARGET)
        target_compile_options(${TACXXF_TARGET} INTERFACE ${flags})
      endif()
      if(DEFINED TACXXF_VAR)
        string(STRIP "${${TACXXF_VAR}} ${flags_as_string}" ${TACXXF_VAR})
      endif()
    endif()
  endif()

  if(DEFINED TACXXF_VAR)
    set(${TACXXF_VAR} "${${TACXXF_VAR}}" PARENT_SCOPE)
  endif()

  if(DEFINED TACXXF_RESULT_VAR)
    set(${TACXXF_RESULT_VAR} "${${compiler_result}}" PARENT_SCOPE)
  endif()

  if(NOT ${compiler_result} OR TACXXF_SKIP_LINK)
    return()
  endif()

  # This forces running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  set(CMAKE_REQUIRED_FLAGS "${flags_as_string}")
  set(CMAKE_REQUIRED_LINK_OPTIONS ${working_linker_werror_flag})
  set(linker_result LINKER_SUPPORTS_${id_string})
  check_cxx_source_compiles("${source}" ${linker_result})

  if(${linker_result})
    if(DEFINED TACXXF_IF_CHECK_PASSED)
      if(DEFINED TACXXF_TARGET)
        target_link_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_PASSED})
      endif()
    else()
      if(DEFINED TACXXF_TARGET)
        target_link_options(${TACXXF_TARGET} INTERFACE ${flags})
      endif()
    endif()
  else()
    message(WARNING "'${flags_as_string}' fail(s) to link.")
  endif()
endfunction()

if(MSVC)
  try_append_cxx_flags("/WX /options:strict" VAR working_compiler_werror_flag SKIP_LINK)
else()
  try_append_cxx_flags("-Werror" VAR working_compiler_werror_flag SKIP_LINK)
endif()
