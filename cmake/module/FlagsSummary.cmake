# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

function(indent_message header content indent_num)
  if(indent_num GREATER 0)
    string(REPEAT " " ${indent_num} indentation)
    string(REPEAT "." ${indent_num} tail)
    string(REGEX REPLACE "${tail}$" "" header "${header}")
  endif()
  message("${indentation}${header} ${content}")
endfunction()

# Print tools' flags on best-effort. Include the abstracted
# CMake flags that we touch ourselves.
function(print_flags_per_config config indent_num)
  string(TOUPPER "${config}" config_uppercase)

  include(GetTargetInterface)
  get_target_interface(definitions "${config}" core_interface COMPILE_DEFINITIONS)
  indent_message("Preprocessor defined macros ..........." "${definitions}" ${indent_num})

  string(STRIP "${CMAKE_CXX_COMPILER_ARG1} ${CMAKE_CXX_FLAGS}" combined_cxx_flags)
  string(STRIP "${combined_cxx_flags} ${CMAKE_CXX_FLAGS_${config_uppercase}}" combined_cxx_flags)
  string(STRIP "${combined_cxx_flags} ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION}" combined_cxx_flags)
  if(CMAKE_POSITION_INDEPENDENT_CODE)
    string(JOIN " " combined_cxx_flags ${combined_cxx_flags} ${CMAKE_CXX_COMPILE_OPTIONS_PIC})
  endif()
  get_target_interface(core_cxx_flags "${config}" core_interface COMPILE_OPTIONS)
  string(STRIP "${combined_cxx_flags} ${core_cxx_flags}" combined_cxx_flags)
  string(STRIP "${combined_cxx_flags} ${APPEND_CPPFLAGS}" combined_cxx_flags)
  string(STRIP "${combined_cxx_flags} ${APPEND_CXXFLAGS}" combined_cxx_flags)
  indent_message("C++ compiler flags ...................." "${combined_cxx_flags}" ${indent_num})

  string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${config_uppercase}}" combined_linker_flags)
  string(STRIP "${combined_linker_flags} ${CMAKE_EXE_LINKER_FLAGS}" combined_linker_flags)
  get_target_interface(common_link_options "${config}" core_interface LINK_OPTIONS)
  string(STRIP "${combined_linker_flags} ${common_link_options}" combined_linker_flags)
  if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    string(JOIN " " combined_linker_flags ${combined_linker_flags} ${CMAKE_CXX_LINK_OPTIONS_PIE})
  endif()
  string(STRIP "${combined_linker_flags} ${APPEND_LDFLAGS}" combined_linker_flags)
  indent_message("Linker flags .........................." "${combined_linker_flags}" ${indent_num})
endfunction()

function(flags_summary)
  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    list(JOIN CMAKE_CONFIGURATION_TYPES ", " configs)
    message("Available build configurations ........ ${configs}")
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
      set(default_config "Debug")
    else()
      list(GET CMAKE_CONFIGURATION_TYPES 0 default_config)
    endif()
    message("Default build configuration ........... ${default_config}")
    foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
      message("")
      message("'${config}' build configuration:")
      print_flags_per_config("${config}" 2)
    endforeach()
  else()
    message("CMAKE_BUILD_TYPE ...................... ${CMAKE_BUILD_TYPE}")
    print_flags_per_config("${CMAKE_BUILD_TYPE}" 0)
  endif()
  message("")
  message([=[
NOTE: The summary above may not exactly match the final applied build flags
      if any additional CMAKE_* or environment variables have been modified.
      To see the exact flags applied, build with the --verbose option.
]=])
endfunction()
