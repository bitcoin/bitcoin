# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(generate_header_from_json json_source_relpath)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${json_source_relpath}.h
    COMMAND ${CMAKE_COMMAND} -DJSON_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${json_source_relpath} -DHEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/${json_source_relpath}.h -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${json_source_relpath} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromJson.cmake
    VERBATIM
  )
endfunction()

function(generate_header_from_raw raw_source_relpath raw_namespace)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${raw_source_relpath}.h
    COMMAND ${CMAKE_COMMAND} -DRAW_SOURCE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${raw_source_relpath} -DHEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/${raw_source_relpath}.h -DRAW_NAMESPACE=${raw_namespace} -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${raw_source_relpath} ${PROJECT_SOURCE_DIR}/cmake/script/GenerateHeaderFromRaw.cmake
    VERBATIM
  )
endfunction()
