if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  find_program(BREW_COMMAND brew)
  execute_process(
    COMMAND ${BREW_COMMAND} --prefix valgrind
    OUTPUT_VARIABLE valgrind_brew_prefix
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

set(hints_paths)
if(valgrind_brew_prefix)
  set(hints_paths ${valgrind_brew_prefix}/include)
endif()

find_path(Valgrind_INCLUDE_DIR
  NAMES valgrind/memcheck.h
  HINTS ${hints_paths}
)

if(Valgrind_INCLUDE_DIR)
  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${Valgrind_INCLUDE_DIR})
  check_c_source_compiles("
    #include <valgrind/memcheck.h>
    #if defined(NVALGRIND)
    #  error \"Valgrind does not support this platform.\"
    #endif

    int main() {}
  " Valgrind_WORKS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Valgrind
  REQUIRED_VARS Valgrind_INCLUDE_DIR Valgrind_WORKS
)

mark_as_advanced(
  Valgrind_INCLUDE_DIR
)
