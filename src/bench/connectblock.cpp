// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/bench.h>
#include <interfaces/chain.h>
#include <kernel/cs_main.h>
#include <script/interpreter.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cassert>
#include <vector>

/*
 * Creates a test block containing transactions with the following properties:
 * - Each transaction has the same number of inputs and outputs
 * - All Taproot inputs use simple key path spends (no script path spends)
 * - All signatures use SIGHASH_ALL (default sighash)
 * - Each transaction spends all outputs from the previous transaction
 */
CBlock CreateTestBlock(
    TestChain100Setup& test_setup,
    const std::vector<CKey>& keys,
    const std::vector<CTxOut>& outputs,
    int num_txs = 1000)
{
    Chainstate& chainstate{test_setup.m_node.chainman->ActiveChainstate()};

    const WitnessV1Taproot coinbase_taproot{XOnlyPubKey(test_setup.coinbaseKey.GetPubKey())};

    // Create the outputs that will be spent in the first transaction of the test block
    // Doing this in a separate block excludes the validation of its inputs from the benchmark
    auto& coinbase_to_spend{test_setup.m_coinbase_txns[0]};
    const auto [first_tx, _]{test_setup.CreateValidTransaction(
        {coinbase_to_spend},
        {COutPoint(coinbase_to_spend->GetHash(), 0)},
        chainstate.m_chain.Height() + 1, keys, outputs, {}, {})};
    const CScript coinbase_spk{GetScriptForDestination(coinbase_taproot)};
    test_setup.CreateAndProcessBlock({first_tx}, coinbase_spk, &chainstate);

    std::vector<CMutableTransaction> txs;
    txs.reserve(num_txs);
    CTransactionRef tx_to_spend{MakeTransactionRef(first_tx)};
    for (int i{0}; i < num_txs; i++) {
        std::vector<COutPoint> inputs;
        inputs.reserve(outputs.size());

        for (size_t j{0}; j < outputs.size(); j++) {
            inputs.emplace_back(tx_to_spend->GetHash(), j);
        }

        const auto [tx, _]{test_setup.CreateValidTransaction(
            {tx_to_spend}, inputs, chainstate.m_chain.Height() + 1, keys, outputs, {}, {})};
        txs.emplace_back(tx);
        tx_to_spend = MakeTransactionRef(tx);
    }

    // Coinbase output can use any output type as it is not spent and will not change the benchmark
    return test_setup.CreateBlock(txs, coinbase_spk, chainstate);
}

/*
 * Creates key pairs and corresponding outputs for the benchmark transactions.
 * - For Schnorr signatures: Creates simple key path spendable outputs
 * - For Ecdsa signatures: Creates P2WPKH (native SegWit v0) outputs
 * - All outputs have value of 1 BTC
 */
std::pair<std::vector<CKey>, std::vector<CTxOut>> CreateKeysAndOutputs(const CKey& coinbaseKey, size_t num_schnorr, size_t num_ecdsa)
{
    std::vector<CKey> keys{coinbaseKey};
    keys.reserve(num_schnorr + num_ecdsa + 1);

    std::vector<CTxOut> outputs;
    outputs.reserve(num_schnorr + num_ecdsa);

    for (size_t i{0}; i < num_ecdsa; ++i) {
        keys.emplace_back(GenerateRandomKey());
        outputs.emplace_back(COIN, GetScriptForDestination(WitnessV0KeyHash{keys.back().GetPubKey()}));
    }

    for (size_t i{0}; i < num_schnorr; ++i) {
        keys.emplace_back(GenerateRandomKey());
        outputs.emplace_back(COIN, GetScriptForDestination(WitnessV1Taproot{XOnlyPubKey(keys.back().GetPubKey())}));
    }

    return {keys, outputs};
}

void BenchmarkConnectBlock(benchmark::Bench& bench, std::vector<CKey>& keys, std::vector<CTxOut>& outputs, TestChain100Setup& test_setup)
{
    const auto& test_block{CreateTestBlock(test_setup, keys, outputs)};
    bench.unit("block").run([&] {
        LOCK(cs_main);
        auto& chainman{test_setup.m_node.chainman};
        auto& chainstate{chainman->ActiveChainstate()};
        BlockValidationState test_block_state;
        auto* pindex{chainman->m_blockman.AddToBlockIndex(test_block, chainman->m_best_header)}; // Doing this here doesn't impact the benchmark
        CCoinsViewCache viewNew{&chainstate.CoinsTip()};

        assert(chainstate.ConnectBlock(test_block, test_block_state, pindex, viewNew));
    });
}

static void ConnectBlockAllSchnorr(benchmark::Bench& bench)
{
    const auto test_setup{MakeNoLogFileContext<TestChain100Setup>()};
    auto [keys, outputs]{CreateKeysAndOutputs(test_setup->coinbaseKey, /*num_schnorr=*/5, /*num_ecdsa=*/0)};
    BenchmarkConnectBlock(bench, keys, outputs, *test_setup);
}

static void ConnectBlockMixedEcdsaSchnorr(benchmark::Bench& bench)
{
    const auto test_setup{MakeNoLogFileContext<TestChain100Setup>()};
    // Blocks in range 848000 to 868000 have a roughly 20 to 80 ratio of schnorr to ecdsa inputs
    auto [keys, outputs]{CreateKeysAndOutputs(test_setup->coinbaseKey, /*num_schnorr=*/1, /*num_ecdsa=*/4)};
    BenchmarkConnectBlock(bench, keys, outputs, *test_setup);
}

static void ConnectBlockAllEcdsa(benchmark::Bench& bench)
{
    const auto test_setup{MakeNoLogFileContext<TestChain100Setup>()};
    auto [keys, outputs]{CreateKeysAndOutputs(test_setup->coinbaseKey, /*num_schnorr=*/0, /*num_ecdsa=*/5)};
    BenchmarkConnectBlock(bench, keys, outputs, *test_setup);
}

BENCHMARK(ConnectBlockAllSchnorr, benchmark::PriorityLevel::HIGH);
BENCHMARK(ConnectBlockMixedEcdsaSchnorr, benchmark::PriorityLevel::HIGH);
BENCHMARK(ConnectBlockAllEcdsa, benchmark::PriorityLevel::HIGH);
