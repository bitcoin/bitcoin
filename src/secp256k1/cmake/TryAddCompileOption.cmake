include(CheckCCompilerFlag)

function(try_add_compile_option option)
  string(MAKE_C_IDENTIFIER ${option} result)
  string(TOUPPER ${result} result)
  set(result "C_SUPPORTS${result}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "-Werror")
  endif()
  check_c_compiler_flag(${option} ${result})
  if(${result})
    get_property(compile_options
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      PROPERTY COMPILE_OPTIONS
    )
    list(APPEND compile_options "${option}")
    set_property(
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      PROPERTY COMPILE_OPTIONS "${compile_options}"
    )
  endif()
endfunction()
