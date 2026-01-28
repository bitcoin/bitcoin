// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/block_generator.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <util/chaintype.h>
#include <validation.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    auto stream{benchmark::GetBlockData()};
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
        assert(stream.Rewind());
    });
}

static void DeserializeAndCheckBlockTest(benchmark::Bench& bench)
{
    const auto& chain_params{CChainParams::RegTest(CChainParams::RegTestOptions{})};
    auto stream{benchmark::GetBlockData(*chain_params)};
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction


    bench.unit("block").run([&] {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> TX_WITH_WITNESS(block);
        assert(stream.Rewind());

        BlockValidationState validationState;
        const bool checked{CheckBlock(block, validationState, chain_params->GetConsensus())};
        assert(checked);
    });
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(DeserializeAndCheckBlockTest);
