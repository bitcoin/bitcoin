// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <node/chainstate.h>
#include <node/context.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <wallet/test/util.h>
#include <validationinterface.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>

#include <optional>

namespace wallet {
static void WalletBalance(benchmark::Bench& bench, const bool set_dirty, const bool add_mine)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    const auto& ADDRESS_WATCHONLY = ADDRESS_BCRT1_UNSPENDABLE;

    // Set clock to genesis block, so the descriptors/keys creation time don't interfere with the blocks scanning process.
    // The reason is 'generatetoaddress', which creates a chain with deterministic timestamps in the past.
    SetMockTime(test_setup->m_node.chainman->GetParams().GenesisBlock().nTime);
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

    bench.run([&] {
        if (set_dirty) wallet.MarkDirty();
        bal = GetBalance(wallet);
        if (add_mine) assert(bal.m_mine_trusted > 0);
    });
}

static void WalletBalanceDirty(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/true, /*add_mine=*/true); }
static void WalletBalanceClean(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/false, /*add_mine=*/true); }
static void WalletBalanceMine(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/false, /*add_mine=*/true); }
static void WalletBalanceWatch(benchmark::Bench& bench) { WalletBalance(bench, /*set_dirty=*/false, /*add_mine=*/false); }

BENCHMARK(WalletBalanceDirty, benchmark::PriorityLevel::HIGH);
BENCHMARK(WalletBalanceClean, benchmark::PriorityLevel::HIGH);
BENCHMARK(WalletBalanceMine, benchmark::PriorityLevel::HIGH);
BENCHMARK(WalletBalanceWatch, benchmark::PriorityLevel::HIGH);
} // namespace wallet
