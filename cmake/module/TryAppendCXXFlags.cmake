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


  try_append_cxx_flags("-Werror=return-type" TARGET core_interface
    IF_CHECK_FAILED "-Wno-error=return-type"
    SOURCE "#include <cassert>\nint f(){ assert(false); }"
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
    "IF_CHECK_PASSED;IF_CHECK_FAILED" # multi_value_keywords
  )

  string(MAKE_C_IDENTIFIER "${flags}" result)
  string(TOUPPER "${result}" result)
  string(PREPEND result CXX_SUPPORTS_)

  set(source "int main() { return 0; }")
  if(DEFINED TACXXF_SOURCE AND NOT TACXXF_SOURCE STREQUAL source)
    set(source "${TACXXF_SOURCE}")
    string(SHA256 source_hash "${source}")
    string(SUBSTRING ${source_hash} 0 4 source_hash_head)
    string(APPEND result _${source_hash_head})
  endif()

  # This avoids running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  set(CMAKE_REQUIRED_FLAGS "${flags} ${working_compiler_werror_flag}")
  check_cxx_source_compiles("${source}" ${result})

  if(${result})
    if(DEFINED TACXXF_IF_CHECK_PASSED)
      if(DEFINED TACXXF_TARGET)
        target_compile_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_PASSED})
      endif()
      if(DEFINED TACXXF_VAR)
        string(STRIP "${${TACXXF_VAR}} ${TACXXF_IF_CHECK_PASSED}" ${TACXXF_VAR})
      endif()
    else()
      if(DEFINED TACXXF_TARGET)
        target_compile_options(${TACXXF_TARGET} INTERFACE ${flags})
      endif()
      if(DEFINED TACXXF_VAR)
        string(STRIP "${${TACXXF_VAR}} ${flags}" ${TACXXF_VAR})
      endif()
    endif()
  elseif(DEFINED TACXXF_IF_CHECK_FAILED)
    if(DEFINED TACXXF_TARGET)
      target_compile_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_FAILED})
    endif()
    if(DEFINED TACXXF_VAR)
      string(STRIP "${${TACXXF_VAR}} ${TACXXF_IF_CHECK_FAILED}" ${TACXXF_VAR})
    endif()
  endif()

  if(DEFINED TACXXF_VAR)
    set(${TACXXF_VAR} "${${TACXXF_VAR}}" PARENT_SCOPE)
  endif()

  if(DEFINED TACXXF_RESULT_VAR)
    set(${TACXXF_RESULT_VAR} "${${result}}" PARENT_SCOPE)
  endif()

  if(NOT ${result} OR TACXXF_SKIP_LINK)
    return()
  endif()

  # This forces running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  set(CMAKE_REQUIRED_FLAGS "${flags} ${working_linker_werror_flag}")
  check_cxx_source_compiles("${source}" ${result})

  if(${result})
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
    message(WARNING "The ${flags} fail(s) to link.")
  endif()
endfunction()

if(MSVC)
  try_append_cxx_flags("/WX /options:strict" VAR working_compiler_werror_flag SKIP_LINK)
else()
  try_append_cxx_flags("-Werror" VAR working_compiler_werror_flag SKIP_LINK)
endif()
