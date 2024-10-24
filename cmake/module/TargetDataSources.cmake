# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(target_data_sources target)
  set(options "")
  set(one_value_args RAW_NAMESPACE)
  set(multi_value_keywords JSON_FILES RAW_FILES)
  cmake_parse_arguments(
    PARSE_ARGV 1
    ARG
    "${options}" "${one_value_args}" "${multi_value_keywords}"
  )

  foreach(json_file IN LISTS ARG_JSON_FILES)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${json_file}.h
      COMMAND ${CMAKE_COMMAND} -DJSON_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${json_file} -DHEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/${json_file}.h -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${json_file} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${json_file}
      VERBATIM
    )
    target_sources(${target} PRIVATE ${json_file})
  endforeach()

  foreach(raw_file IN LISTS ARG_RAW_FILES)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${raw_file}.h
      COMMAND ${CMAKE_COMMAND} -DRAW_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${raw_file} -DHEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/${raw_file}.h -DRAW_NAMESPACE=${ARG_RAW_NAMESPACE} -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${raw_file} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${raw_file}
      VERBATIM
    )
    target_sources(${target} PRIVATE ${raw_file})
  endforeach()
endfunction()
