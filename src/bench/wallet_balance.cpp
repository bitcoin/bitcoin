// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/wallet_bench_util.h>
#include <consensus/consensus.h>
#include <interfaces/chain.h>
#include <kernel/chainparams.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/time.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace wallet {

static void WalletBalance(benchmark::Bench& bench, const bool set_dirty, const bool add_mine)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    const auto& ADDRESS_WATCHONLY = ADDRESS_BCRT1_UNSPENDABLE;

    // Set clock to genesis block, so the descriptors/keys creation time don't interfere with the blocks scanning process.
    // The reason is 'generatetoaddress', which creates a chain with deterministic timestamps in the past.
    FakeNodeClock clock{test_setup->m_node.chainman->GetParams().GenesisBlock().Time()};
    CWallet wallet{test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase()};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetupDescriptorScriptPubKeyMans();
    }
    auto handler = test_setup->m_node.chain->handleNotifications({&wallet, [](CWallet*) {}});

    const std::optional<std::string> address_mine{add_mine ? std::optional<std::string>{getnewaddress(wallet)} : std::nullopt};

    for (int i = 0; i < 100; ++i) {
        generatetoaddress(test_setup->m_node, address_mine.value_or(ADDRESS_WATCHONLY));
        generatetoaddress(test_setup->m_node, ADDRESS_WATCHONLY);
    }
    // Calls SyncWithValidationInterfaceQueue
    wallet.chain().waitForNotificationsIfTipChanged(uint256::ZERO);

    auto bal = GetBalance(wallet); // Cache

    bench.setup([&] {
            if (set_dirty) wallet.MarkDirty();
        })
        .run([&] {
            bal = GetBalance(wallet);
            ankerl::nanobench::doNotOptimizeAway(bal);
            assert(add_mine == (bal.m_mine_trusted > 0));
        });
}

static void WalletBalanceDirty(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/true, /*add_mine=*/true); }
static void WalletBalanceClean(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/false, /*add_mine=*/true); }
static void WalletBalanceWatch(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/false, /*add_mine=*/false); }

BENCHMARK(WalletBalanceDirty);
BENCHMARK(WalletBalanceClean);
BENCHMARK(WalletBalanceWatch);

static void WalletBalanceManySpent(benchmark::Bench& bench)
{
    constexpr int NUM_BLOCKS{500};
    constexpr int OUTPUTS_PER_BLOCK{100};
    constexpr CAmount OUTPUT_VALUE{50 * COIN / OUTPUTS_PER_BLOCK};
    constexpr size_t TOTAL_WALLET_OUTPUTS{NUM_BLOCKS * OUTPUTS_PER_BLOCK};
    constexpr size_t KEEP_UNSPENT{50};
    constexpr size_t BATCH_SIZE{100};
    constexpr CAmount FEE{1000};
    constexpr size_t TXS_PER_BLOCK{10};
    constexpr int FILLER_OUTPUTS{1};
    constexpr CAmount FILLER_VALUE{50 * COIN};

    // Benchmark GetBalance with many spent TXOs and few unspent TXOs.
    // This scenario benefits from optimizations that separate definitely-spent
    // outputs from potentially-spendable ones (such as m_unusable_txos).
    //
    // Target: many spent TXOs, few unspent TXOs.
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();
    const auto& params = test_setup->m_node.chainman->GetParams();

    NodeClockContext clock_ctx{params.GenesisBlock().Time()};
    CWallet wallet{test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase()};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetupDescriptorScriptPubKeyMans();
    }
    auto handler = test_setup->m_node.chain->handleNotifications({&wallet, [](CWallet*) {}});

    const auto dest = getNewDestination(wallet, OutputType::BECH32);
    const CScript wallet_script = GetScriptForDestination(dest);
    // Burn script for outputs we don't want in the wallet
    const CScript burn_script{CScript() << OP_TRUE};

    // Phase 1: Create the configured wallet-owned output history.
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        GenerateFakeBlock(params, test_setup->m_node, wallet, wallet_script, OUTPUTS_PER_BLOCK);
    }

    // Phase 2: Mature all wallet-owned coinbase outputs.
    for (int i = 0; i < COINBASE_MATURITY; ++i) {
        GenerateFakeBlock(params, test_setup->m_node, wallet, burn_script, FILLER_OUTPUTS);
    }

    // Phase 3: Spend most wallet outputs in batches.
    std::vector<COutPoint> outputs_to_spend;
    outputs_to_spend.reserve(TOTAL_WALLET_OUTPUTS);
    {
        LOCK(wallet.cs_wallet);
        for (const auto& [_, coins] : AvailableCoins(wallet).coins) {
            for (const auto& coin : coins) {
                outputs_to_spend.push_back(coin.outpoint);
            }
        }
    }

    // Keep the configured small remainder unspent, and spend the rest.
    assert(outputs_to_spend.size() == TOTAL_WALLET_OUTPUTS);
    const size_t num_to_spend{outputs_to_spend.size() - KEEP_UNSPENT};

    // Create spending transactions in configured-size batches.
    std::vector<CTransactionRef> spending_txs;

    for (size_t i{0}; i < num_to_spend; i += BATCH_SIZE) {
        CMutableTransaction spend_tx;
        const size_t batch_end{std::min(i + BATCH_SIZE, num_to_spend)};

        CAmount total_value{0};
        for (size_t j{i}; j < batch_end; ++j) {
            spend_tx.vin.emplace_back(outputs_to_spend[j]);
            total_value += OUTPUT_VALUE;
        }

        // Send to burn script so consolidation outputs don't add to wallet TXOs
        spend_tx.vout.emplace_back(total_value - FEE, burn_script);

        spending_txs.push_back(MakeTransactionRef(std::move(spend_tx)));
    }

    // Phase 4: Confirm all spending transactions in fake blocks.
    // Coinbase outputs go to burn script to avoid adding wallet TXOs.
    for (size_t i{0}; i < spending_txs.size(); i += TXS_PER_BLOCK) {
        const size_t batch_end{std::min(i + TXS_PER_BLOCK, spending_txs.size())};
        std::vector<CTransactionRef> block_txs{spending_txs.begin() + i, spending_txs.begin() + batch_end};
        GenerateFakeBlock(params, test_setup->m_node, wallet, burn_script, FILLER_OUTPUTS, FILLER_VALUE, block_txs);
    }

    // Balance alone does not prove that the intended wallet output state was built.
    size_t owned_outputs{0};
    size_t spent_outputs{0};
    {
        LOCK(wallet.cs_wallet);
        for (const auto& [outpoint, _] : wallet.GetTXOs()) {
            ++owned_outputs;
            spent_outputs += wallet.IsSpent(outpoint);
        }
    }
    if (owned_outputs == TOTAL_WALLET_OUTPUTS) {
        assert(spent_outputs == TOTAL_WALLET_OUTPUTS - KEEP_UNSPENT);
    } else {
        assert(owned_outputs == KEEP_UNSPENT);
        assert(spent_outputs == 0);
    }

    CAmount expected_balance{0};
    for (size_t i{0}; i < KEEP_UNSPENT; ++i) {
        expected_balance += OUTPUT_VALUE;
    }

    bench.warmup(1).minEpochIterations(128).run([&] {
        const auto bal{GetBalance(wallet)};
        ankerl::nanobench::doNotOptimizeAway(bal);
        assert(bal.m_mine_trusted == expected_balance);
        assert(bal.m_mine_immature == 0);
    });
}

BENCHMARK(WalletBalanceManySpent);
} // namespace wallet
