# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

cmake_path(GET JSON_SOURCE_PATH STEM json_source_basename)

file(READ ${JSON_SOURCE_PATH} hex_content HEX)
string(STRIP "${hex_content}" hex_content)

set(header_content
"#include <util/strencodings.h>

#include <string_view>
#include <cstddef>

using namespace util::hex_literals;

namespace json_tests {
inline auto detail_${json_source_basename}_bytes = \"${hex_content}\"_hex;

inline std::string_view ${json_source_basename}{reinterpret_cast<const char*>(detail_${json_source_basename}_bytes.data()), detail_${json_source_basename}_bytes.size()};
}
")

file(WRITE ${HEADER_PATH} "${header_content}")
