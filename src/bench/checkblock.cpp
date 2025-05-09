// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block_784588.raw.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
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
    DataStream stream(benchmark::data::block_784588);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
        bool rewound = stream.Rewind(benchmark::data::block_784588.size());
        assert(rewound);
    });
}


static std::map<std::string, uint64_t> TallyScriptTypes(const CBlock& block)
{
    auto classify{[&](const CScript& script) {
        switch (std::vector<std::vector<uint8_t>> s; Solver(script, s)) {
        case TxoutType::ANCHOR: return "P2A";
        case TxoutType::MULTISIG: return "P2MS";
        case TxoutType::NULL_DATA: return "OP_RETURN";
        case TxoutType::PUBKEY: return "P2PK";
        case TxoutType::PUBKEYHASH: return "P2PKH";
        case TxoutType::SCRIPTHASH: return "P2SH";
        case TxoutType::WITNESS_V1_TAPROOT: return "P2TR";
        case TxoutType::WITNESS_V0_KEYHASH: return "P2WPKH";
        case TxoutType::WITNESS_V0_SCRIPTHASH: return "P2WSH";
        case TxoutType::NONSTANDARD:     // fall-through
        case TxoutType::WITNESS_UNKNOWN: // fall-through
        default: return "OTHER";
        }
    }};

    std::map<std::string, uint64_t> tally;
    for (const auto& tx : block.vtx) {
        for (const auto& txin : tx->vin) ++tally[classify(txin.scriptSig)];
        for (const auto& txout : tx->vout) ++tally[classify(txout.scriptPubKey)];
    }
    return tally;
}

static void DeserializeAndCheckBlockTest(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block_784588);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    {
        // Document what we're actually measuring
        CBlock block;
        stream >> TX_WITH_WITNESS(block);

        assert(block.hashMerkleRoot == uint256{"471dd9fc392bb29f35cfa91b2b249294c908c39775b6d8823cb5e158776fb51f"});
        assert(block.vtx.size() == 1984);
        assert(TallyScriptTypes(block) == (std::map<std::string, uint64_t>{
            {"OP_RETURN", 21},
            {"OTHER", 5541},
            {"P2MS", 36},
            {"P2PKH", 800},
            {"P2SH", 1016},
            {"P2TR", 1242},
            {"P2WPKH", 1669},
            {"P2WSH", 182},
        }));
    }

    ArgsManager bench_args;
    const auto chainParams = CreateChainParams(bench_args, ChainType::MAIN);

    bench.unit("block").run([&] {
        bool rewound = stream.Rewind(benchmark::data::block_784588.size());
        assert(rewound);

        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> TX_WITH_WITNESS(block);

        BlockValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
        assert(checked);
    });
}

BENCHMARK(DeserializeBlockTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeAndCheckBlockTest, benchmark::PriorityLevel::HIGH);
