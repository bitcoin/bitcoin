# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

macro(set_add_custom_command_options)
  set(DEPENDS_EXPLICIT_OPT "")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
    set(DEPENDS_EXPLICIT_OPT DEPENDS_EXPLICIT_ONLY)
  endif()
  set(CODEGEN_OPT "")
  if(POLICY CMP0171)
    cmake_policy(GET CMP0171 _cmp0171_status)
    if(_cmp0171_status STREQUAL "NEW")
      set(CODEGEN_OPT CODEGEN)
    endif()
    unset(_cmp0171_status)
  endif()
endmacro()

# Specifies JSON data files to be processed into corresponding
# header files for inclusion when building a target.
function(target_json_data_sources target)
  set_add_custom_command_options()
  foreach(json_file IN LISTS ARGN)
    set(header ${CMAKE_CURRENT_BINARY_DIR}/${json_file}.h)
    add_custom_command(
      OUTPUT ${header}
      COMMAND ${CMAKE_COMMAND} -DJSON_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${json_file} -DHEADER_PATH=${header} -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${json_file} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
      VERBATIM
      ${CODEGEN_OPT}
      ${DEPENDS_EXPLICIT_OPT}
    )
    target_sources(${target} PRIVATE ${header})
  endforeach()
endfunction()

# Specifies raw binary data files to be processed into corresponding
# header files for inclusion when building a target.
function(target_raw_data_sources target)
  cmake_parse_arguments(PARSE_ARGV 1 _ "" "NAMESPACE" "")
  set_add_custom_command_options()
  foreach(raw_file IN LISTS __UNPARSED_ARGUMENTS)
    set(header ${CMAKE_CURRENT_BINARY_DIR}/${raw_file}.h)
    add_custom_command(
      OUTPUT ${header}
      COMMAND ${CMAKE_COMMAND} -DRAW_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${raw_file} -DHEADER_PATH=${header} -DRAW_NAMESPACE=${__NAMESPACE} -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${raw_file} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
      VERBATIM
      ${CODEGEN_OPT}
      ${DEPENDS_EXPLICIT_OPT}
    )
    target_sources(${target} PRIVATE ${header})
  endforeach()
endfunction()
