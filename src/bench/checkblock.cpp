// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data.h>

#include <chainparams.h>
#include <validation.h>
#include <streams.h>
#include <consensus/validation.h>

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::State& state)
{
    CDataStream stream(benchmark::data::block413567, SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    while (state.KeepRunning()) {
        CBlock block;
        stream >> block;
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);
    }
}

static void DeserializeAndCheckBlockTest(benchmark::State& state)
{
    CDataStream stream(benchmark::data::block413567, SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);

    while (state.KeepRunning()) {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> block;
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);

        BlockValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
        assert(checked);
    }
}

BENCHMARK(DeserializeBlockTest, 130);
BENCHMARK(DeserializeAndCheckBlockTest, 160);
