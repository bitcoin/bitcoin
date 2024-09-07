# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

file(READ ${RAW_SOURCE_PATH} hex_content HEX)
string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" bytes "${hex_content}")

set(header_content "#include <cstddef>\n")
string(APPEND header_content "#include <span>\n")
string(APPEND header_content "namespace ${RAW_NAMESPACE} {\n")
get_filename_component(raw_source_basename ${RAW_SOURCE_PATH} NAME_WE)
string(APPEND header_content "inline constexpr std::byte detail_${raw_source_basename}_raw[]{\n")

set(i 0)
foreach(byte ${bytes})
  math(EXPR i "${i} + 1")
  if(i EQUAL 8)
    set(i 0)
    string(APPEND header_content "std::byte{0x${byte}},\n")
  else()
    string(APPEND header_content "std::byte{0x${byte}}, ")
  endif()
endforeach()

string(APPEND header_content "\n};\n")
string(APPEND header_content "inline constexpr std::span ${raw_source_basename}{detail_${raw_source_basename}_raw};\n")
string(APPEND header_content "}")

file(WRITE ${HEADER_PATH} "${header_content}")
