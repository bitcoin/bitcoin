// Copyright (c) 2016-2022 The Bitcoin Core developers
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
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    {
        // Document what we're actually measuring
        CBlock block;
        stream >> TX_WITH_WITNESS(block);

        assert(block.hashMerkleRoot == uint256{"64a50c649fc816baaa2effda230c39cacf1504e4e616a2863685b72aaa7dce05"});
        assert(block.vtx.size() == 1557);
        assert(TallyScriptTypes(block) == (std::map<std::string, uint64_t>{
            {"OP_RETURN", 3},
            {"OTHER", 4887},
            {"P2PKH", 2841},
            {"P2SH", 737},
        }));
    }

    ArgsManager bench_args;
    const auto chainParams = CreateChainParams(bench_args, ChainType::MAIN);

    bench.unit("block").run([&] {
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
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
