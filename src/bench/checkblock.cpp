// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/tx_verify.h>
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

// These are the major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockBench(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);
    });
}

static void CheckBlockBench(benchmark::Bench& bench)
{
    CBlock block;
    DataStream(benchmark::data::block413567) >> TX_WITH_WITNESS(block);
    const auto& chainParams = CreateChainParams(ArgsManager{}, ChainType::MAIN);
    bench.unit("block").run([&] {
        block.ResetChecked();
        BlockValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus(), /*fCheckPOW=*/true, /*fCheckMerkleRoot=*/true);
        assert(checked && validationState.IsValid());
    });
}

static void SigOpsBlockBench(benchmark::Bench& bench)
{
    CBlock block;
    DataStream(benchmark::data::block413567) >> TX_WITH_WITNESS(block);

    constexpr auto expected_sigops{2841};
    bench.batch(expected_sigops).unit("sigops").run([&] {
        auto nSigOps{0};
        for (const auto& tx : block.vtx) {
            nSigOps += GetLegacySigOpCount(*tx);
        }
        assert(nSigOps == expected_sigops);
    });
}

BENCHMARK(DeserializeBlockBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(CheckBlockBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(SigOpsBlockBench, benchmark::PriorityLevel::HIGH);
