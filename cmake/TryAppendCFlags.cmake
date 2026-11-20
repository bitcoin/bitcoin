include(CheckCCompilerFlag)

# try_append_c_flags(<flags> TARGET <target>)
#
# Append <flags> to the INTERFACE_COMPILE_OPTIONS of <target>
# if the C compiler accepts them.
function(try_append_c_flags flags)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "TARGET" "")
  if(NOT DEFINED arg_TARGET OR arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Usage: ${CMAKE_CURRENT_FUNCTION}(<flags> TARGET <target>)")
  endif()

  string(MAKE_C_IDENTIFIER "${flags}" result)
  string(TOUPPER "${result}" result)
  set(result "C_SUPPORTS_${result}")
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "-Werror")
  endif()

  # This avoids running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  check_c_compiler_flag("${flags}" ${result})
  if(${result})
    target_compile_options(${arg_TARGET} INTERFACE ${flags})
  endif()
endfunction()
