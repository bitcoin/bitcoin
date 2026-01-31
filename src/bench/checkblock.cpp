// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/chaintype.h>
#include <validation.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    DataStream stream;
    bench.unit("block").epochIterations(1)
        .setup([&] { stream = DataStream{benchmark::data::block413567}; })
        .run([&] { CBlock block; stream >> TX_WITH_WITNESS(block); });
}

static void CheckBlockTest(benchmark::Bench& bench)
{
    ArgsManager bench_args;
    const auto chainParams = CreateChainParams(bench_args, ChainType::MAIN);

    CBlock block;
    bench.unit("block").epochIterations(1)
        .setup([&] {
            block = CBlock{};
            DataStream stream{benchmark::data::block413567};
            stream >> TX_WITH_WITNESS(block);
        })
        .run([&] {
            BlockValidationState validationState;
            bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
            assert(checked);
        });
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(CheckBlockTest);
