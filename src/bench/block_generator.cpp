// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/block_generator.h>
#include <bench/data/block413567.raw.h>

#include <span.h>
#include <streams.h>

namespace benchmark {
DataStream GenerateBlockData(const CChainParams&, const uint256&)
{
    return DataStream{benchmark::data::block413567};
}

CBlock GenerateBlock(const CChainParams&, const uint256&)
{
    CBlock block;
    SpanReader{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    return block;
}
} // namespace benchmark
