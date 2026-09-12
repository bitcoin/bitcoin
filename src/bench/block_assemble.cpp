// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/consensus.h>
#include <node/mining_types.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validation.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ranges>
#include <vector>

using node::BlockCreateOptions;

static void AssembleBlock(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    CScriptWitness witness;
    witness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    BlockCreateOptions options{
        .coinbase_output_script = P2WSH_OP_TRUE,
    };

    // Collect fan-out transactions that split the coinbases of our mined blocks
    constexpr size_t NUM_BLOCKS{200};
    constexpr uint32_t FAN_OUT{100};
    std::array<CTransactionRef, NUM_BLOCKS - COINBASE_MATURITY + 1> txs;
    for (size_t b{0}; b < NUM_BLOCKS; ++b) {
        CMutableTransaction tx;
        tx.vin.emplace_back(MineBlock(test_setup->m_node, options));
        tx.vin.back().scriptWitness = witness;
        tx.vout.assign(FAN_OUT, CTxOut{100'000, P2WSH_OP_TRUE});
        if (NUM_BLOCKS - b >= COINBASE_MATURITY)
            txs.at(b) = MakeTransactionRef(tx);
    }
    {
        LOCK(::cs_main);

        for (const auto& txr : txs) {
            const MempoolAcceptResult res = test_setup->m_node.chainman->ProcessTransaction(txr);
            assert(res.m_result_type == MempoolAcceptResult::ResultType::VALID);
        }
    }

    // Mine a block confirming the fan-out transactions, so the loose transactions
    // below spend coins that already exist in the UTXO set
    MineBlock(test_setup->m_node, options);

    // Collect loose transactions spending the fan-out outputs to fill the assembled block
    {
        LOCK(::cs_main);

        for (const auto& txr : txs) {
            for (const uint32_t o : std::views::iota(uint32_t{0}, FAN_OUT)) {
                CMutableTransaction tx;
                tx.vin.emplace_back(COutPoint{txr->GetHash(), o});
                tx.vin.back().scriptWitness = witness;
                tx.vout.emplace_back(1337, P2WSH_OP_TRUE);
                const MempoolAcceptResult res = test_setup->m_node.chainman->ProcessTransaction(MakeTransactionRef(tx));
                assert(res.m_result_type == MempoolAcceptResult::ResultType::VALID);
            }
        }
    }

    bench.run([&] {
        PrepareBlock(test_setup->m_node, options);
    });
}
static void BlockAssemblerAddPackageTxns(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    auto testing_setup{MakeNoLogFileContext<TestChain100Setup>()};
    testing_setup->PopulateMempool(det_rand, /*num_transactions=*/1000, /*submit=*/true);

    bench.run([&] {
        PrepareBlock(testing_setup->m_node, {
            .coinbase_output_script = P2WSH_OP_TRUE,
            .test_block_validity = false
        });
    });
}

BENCHMARK(AssembleBlock);
BENCHMARK(BlockAssemblerAddPackageTxns);
