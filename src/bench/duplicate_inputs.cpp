// Copyright (c) 2011-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>


static void DuplicateInputs(benchmark::Bench& bench)
{
    TestingSetup test_setup{
        CBaseChainParams::REGTEST,
        /* extra_args */ {
            "-nodebuglogfile",
            "-nodebug",
        },
    };

    const CScript SCRIPT_PUB{CScript(OP_TRUE)};

    const CChainParams& chainparams = Params();

    CBlock block{};
    CMutableTransaction coinbaseTx{};
    CMutableTransaction naughtyTx{};

    LOCK(cs_main);
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    assert(pindexPrev != nullptr);
    block.nBits = GetNextWorkRequired(pindexPrev, &block, chainparams.GetConsensus());
    block.nNonce = 0;
    auto nHeight = pindexPrev->nHeight + 1;

    // Make a coinbase TX
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = SCRIPT_PUB;
    coinbaseTx.vout[0].nValue = GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;


    naughtyTx.vout.resize(1);
    naughtyTx.vout[0].nValue = 0;
    naughtyTx.vout[0].scriptPubKey = SCRIPT_PUB;

    uint64_t n_inputs = (((MAX_BLOCK_SERIALIZED_SIZE / WITNESS_SCALE_FACTOR) - (CTransaction(coinbaseTx).GetTotalSize() + CTransaction(naughtyTx).GetTotalSize())) / 41) - 100;
    for (uint64_t x = 0; x < (n_inputs - 1); ++x) {
        naughtyTx.vin.emplace_back(GetRandHash(), 0, CScript(), 0);
    }
    naughtyTx.vin.emplace_back(naughtyTx.vin.back());

    block.vtx.push_back(MakeTransactionRef(std::move(coinbaseTx)));
    block.vtx.push_back(MakeTransactionRef(std::move(naughtyTx)));

    block.hashMerkleRoot = BlockMerkleRoot(block);

    bench.run([&] {
        BlockValidationState cvstate{};
        assert(!CheckBlock(block, cvstate, chainparams.GetConsensus(), false, false));
        assert(cvstate.GetRejectReason() == "bad-txns-inputs-duplicate");
    });
}

BENCHMARK(DuplicateInputs);
