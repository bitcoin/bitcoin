# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

file(READ ${JSON_SOURCE_PATH} hex_content HEX)
string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" bytes "${hex_content}")

set(header_content "#include <string_view>\n")
string(APPEND header_content "namespace json_tests{\n")
get_filename_component(json_source_basename ${JSON_SOURCE_PATH} NAME_WE)
string(APPEND header_content "inline constexpr char detail_${json_source_basename}_bytes[]{\n")

set(i 0)
foreach(byte ${bytes})
  math(EXPR i "${i} + 1")
  if(i EQUAL 8)
    set(i 0)
    string(APPEND header_content "0x${byte},\n")
  else()
    string(APPEND header_content "0x${byte}, ")
  endif()
endforeach()

string(APPEND header_content "\n};\n")
string(APPEND header_content "inline constexpr std::string_view ${json_source_basename}{std::begin(detail_${json_source_basename}_bytes), std::end(detail_${json_source_basename}_bytes)};")
string(APPEND header_content "\n}")

file(WRITE ${HEADER_PATH} "${header_content}")
