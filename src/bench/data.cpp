// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/block813851.raw.h>
const std::vector<uint8_t> block813851{std::begin(raw_bench::block813851_raw), std::end(raw_bench::block813851_raw)};

} // namespace data
} // namespace benchmark
