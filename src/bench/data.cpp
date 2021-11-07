// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/block413567.raw.h>
const std::vector<uint8_t> block413567{std::begin(block413567_raw), std::end(block413567_raw)};

} // namespace data
} // namespace benchmark
