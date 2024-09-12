# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

file(READ ${RAW_SOURCE_PATH} hex_content HEX)
string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" bytes "${hex_content}")

file(WRITE ${HEADER_PATH} "#include <cstddef>\n")
file(APPEND ${HEADER_PATH} "#include <span>\n")
file(APPEND ${HEADER_PATH} "namespace ${RAW_NAMESPACE} {\n")
get_filename_component(raw_source_basename ${RAW_SOURCE_PATH} NAME_WE)
file(APPEND ${HEADER_PATH} "inline constexpr std::byte detail_${raw_source_basename}_raw[]{\n")

set(i 0)
foreach(byte ${bytes})
  math(EXPR i "${i} + 1")
  math(EXPR remainder "${i} % 8")
  if(remainder EQUAL 0)
    file(APPEND ${HEADER_PATH} "std::byte{0x${byte}},\n")
  else()
    file(APPEND ${HEADER_PATH} "std::byte{0x${byte}}, ")
  endif()
endforeach()

file(APPEND ${HEADER_PATH} "\n};\n")
file(APPEND ${HEADER_PATH} "inline constexpr std::span ${raw_source_basename}{detail_${raw_source_basename}_raw};\n")
file(APPEND ${HEADER_PATH} "}")
