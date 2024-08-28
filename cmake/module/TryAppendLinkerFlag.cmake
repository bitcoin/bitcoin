# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)

#[=[
Usage example:

  try_append_linker_flag("-Wl,--major-subsystem-version,6" TARGET core_interface)


In configuration output, this function prints a string by the following pattern:

  -- Performing Test LINKER_SUPPORTS_[flag]
  -- Performing Test LINKER_SUPPORTS_[flag] - Success

]=]
function(try_append_linker_flag flag)
  cmake_parse_arguments(PARSE_ARGV 1
    TALF                              # prefix
    ""                                # options
    "TARGET;VAR;SOURCE;RESULT_VAR"    # one_value_keywords
    "IF_CHECK_PASSED"                 # multi_value_keywords
  )

  # Create a unique identifier based on the flag and a global linker setting
  string(MAKE_C_IDENTIFIER "${flag} ${working_linker_werror_flag}" linker_result)
  string(TOUPPER "${linker_result}" linker_result)
  string(PREPEND linker_result "LINKER_SUPPORTS_")

  set(source "int main() { return 0; }")
  if(DEFINED TALF_SOURCE AND NOT TALF_SOURCE STREQUAL source)
    # If a custom source code is provided, use it and append its hash to the cache key to ensure uniqueness.
    set(source "${TALF_SOURCE}")
    string(SHA256 source_hash "${source}")
    string(TOUPPER "${source_hash}" upper_hash)
    string(APPEND linker_result "_${upper_hash}")
  endif()

  # This forces running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  set(CMAKE_REQUIRED_LINK_OPTIONS ${flag} ${working_linker_werror_flag})
  check_cxx_source_compiles("${source}" ${linker_result})

  if(${linker_result})
    if(DEFINED TALF_IF_CHECK_PASSED)
      if(DEFINED TALF_TARGET)
        target_link_options(${TALF_TARGET} INTERFACE ${TALF_IF_CHECK_PASSED})
      endif()
      if(DEFINED TALF_VAR)
        string(STRIP "${${TALF_VAR}} ${TALF_IF_CHECK_PASSED}" ${TALF_VAR})
      endif()
    else()
      if(DEFINED TALF_TARGET)
        target_link_options(${TALF_TARGET} INTERFACE ${flag})
      endif()
      if(DEFINED TALF_VAR)
        string(STRIP "${${TALF_VAR}} ${flag}" ${TALF_VAR})
      endif()
    endif()
  endif()

  if(DEFINED TALF_VAR)
    set(${TALF_VAR} "${${TALF_VAR}}" PARENT_SCOPE)
  endif()

  if(DEFINED TALF_RESULT_VAR)
    set(${TALF_RESULT_VAR} "${${linker_result}}" PARENT_SCOPE)
  endif()
endfunction()

if(MSVC)
  try_append_linker_flag("/WX" VAR working_linker_werror_flag)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  try_append_linker_flag("-Wl,-fatal_warnings" VAR working_linker_werror_flag)
else()
  try_append_linker_flag("-Wl,--fatal-warnings" VAR working_linker_werror_flag)
endif()
