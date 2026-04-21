// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <validation.h>

#include <cassert>
#include <cstddef>

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    const auto block_data{benchmark::data::block413567};
    bench.unit("block").run([&] {
        CBlock block;
        SpanReader{block_data} >> TX_WITH_WITNESS(block);
        assert(block.vtx.size() == 1557);
    });
}

static void CheckBlockTest(benchmark::Bench& bench)
{
    const auto& chain_params{CChainParams::Main()};
    const auto block_data{benchmark::data::block413567};

    CBlock block;
    bench.unit("block").epochIterations(1)
        .setup([&] {
            block = CBlock{};
            SpanReader{block_data} >> TX_WITH_WITNESS(block);
            assert(block.vtx.size() == 1557);
        })
        .run([&] {
            BlockValidationState validationState;
            const bool checked{CheckBlock(block, validationState, chain_params->GetConsensus())};
            assert(checked);
        });
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(CheckBlockTest);
