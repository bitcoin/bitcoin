# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

cmake_path(GET RAW_SOURCE_PATH STEM raw_source_basename)

file(READ ${RAW_SOURCE_PATH} hex_content HEX)
string(STRIP "${hex_content}" hex_content)

set(header_content
"#include <util/strencodings.h>

#include <cstddef>
#include <span>

using namespace util::hex_literals;

namespace ${RAW_NAMESPACE} {

inline constexpr auto ${raw_source_basename} = \"${hex_content}\"_hex;
inline constexpr std::span<const std::byte> ${raw_source_basename}_span{${raw_source_basename}};

} // namespace ${RAW_NAMESPACE}
")

file(WRITE ${HEADER_PATH} "${header_content}")
