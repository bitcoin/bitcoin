# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

file(READ ${JSON_SOURCE_PATH} hex_content HEX)
string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" bytes "${hex_content}")

file(WRITE ${HEADER_PATH} "#include <string>\n")
file(APPEND ${HEADER_PATH} "namespace json_tests{\n")
get_filename_component(json_source_basename ${JSON_SOURCE_PATH} NAME_WE)
file(APPEND ${HEADER_PATH} "static const std::string ${json_source_basename}{\n")

set(i 0)
foreach(byte ${bytes})
  math(EXPR i "${i} + 1")
  math(EXPR remainder "${i} % 8")
  if(remainder EQUAL 0)
    file(APPEND ${HEADER_PATH} "0x${byte},\n")
  else()
    file(APPEND ${HEADER_PATH} "0x${byte}, ")
  endif()
endforeach()

file(APPEND ${HEADER_PATH} "\n};};")
