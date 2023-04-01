# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(try_append_linker_flag flags_var flag)
  cmake_parse_arguments(PARSE_ARGV 2 TRY_APPEND_LINKER_FLAG
    "" "SOURCE" "CHECK_PASSED_FLAG;CHECK_FAILED_FLAG"
  )
  string(MAKE_C_IDENTIFIER "${flag}" result)
  string(TOUPPER "${result}" result)
  set(result "LINKER_SUPPORTS${result}")
  unset(${result})
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  if(CMAKE_VERSION VERSION_LESS 3.14)
    set(CMAKE_REQUIRED_LIBRARIES "${flag}")
  else()
    set(CMAKE_REQUIRED_LINK_OPTIONS "${flag}")
  endif()

  if(TRY_APPEND_LINKER_FLAG_SOURCE)
    set(source "${TRY_APPEND_LINKER_FLAG_SOURCE}")
    unset(${result} CACHE)
  else()
    set(source "int main() { return 0; }")
  endif()

  # Normalize locale during test compilation.
  set(locale_vars LC_ALL LC_MESSAGES LANG)
  foreach(v IN LISTS locale_vars)
    set(locale_vars_saved_${v} "$ENV{${v}}")
    set(ENV{${v}} C)
  endforeach()

  include(CMakeCheckCompilerFlagCommonPatterns)
  check_compiler_flag_common_patterns(common_patterns)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${source}" ${result} ${common_patterns})

  foreach(v IN LISTS locale_vars)
    set(ENV{${v}} ${locale_vars_saved_${v}})
  endforeach()

  if(${result})
    if(DEFINED TRY_APPEND_LINKER_FLAG_CHECK_PASSED_FLAG)
      string(STRIP "${${flags_var}} ${TRY_APPEND_LINKER_FLAG_CHECK_PASSED_FLAG}" ${flags_var})
    else()
      string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    endif()
  elseif(DEFINED TRY_APPEND_LINKER_FLAG_CHECK_FAILED_FLAG)
    string(STRIP "${${flags_var}} ${TRY_APPEND_LINKER_FLAG_CHECK_FAILED_FLAG}" ${flags_var})
  endif()

  set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  set(${result} "${${result}}" PARENT_SCOPE)
  set(try_append_linker_flag_result "${${result}}" PARENT_SCOPE)
endfunction()
