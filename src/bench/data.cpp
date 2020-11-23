// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/block413567.raw.h>
const std::vector<uint8_t> block413567{block413567_raw, block413567_raw + sizeof(block413567_raw) / sizeof(block413567_raw[0])};

} // namespace data
} // namespace benchmark
