// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/wallet.h>
#include <validationinterface.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>

#include <optional>

using wallet::CreateMockWalletDatabase;
using wallet::CWallet;
using wallet::DBErrors;
using wallet::WALLET_FLAG_DESCRIPTORS;

static void WalletBalance(benchmark::Bench& bench, const bool set_dirty, const bool add_mine, const uint32_t epoch_iters)
{
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();
    const auto& ADDRESS_WATCHONLY = ADDRESS_B58T_UNSPENDABLE;

    CWallet wallet{test_setup->m_node.chain.get(), test_setup->m_node.coinjoin_loader.get(), "", gArgs, CreateMockWalletDatabase()};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetupDescriptorScriptPubKeyMans();
        if (wallet.LoadWallet() != DBErrors::LOAD_OK) assert(false);
    }
    auto handler = test_setup->m_node.chain->handleNotifications({&wallet, [](CWallet*) {}});

    const std::optional<std::string> address_mine{add_mine ? std::optional<std::string>{getnewaddress(wallet)} : std::nullopt};

    for (int i = 0; i < 100; ++i) {
        generatetoaddress(test_setup->m_node, address_mine.value_or(ADDRESS_WATCHONLY));
        generatetoaddress(test_setup->m_node, ADDRESS_WATCHONLY);
    }
    SyncWithValidationInterfaceQueue();

    auto bal = GetBalance(wallet); // Cache

    bench.minEpochIterations(epoch_iters).run([&] {
        if (set_dirty) wallet.MarkDirty();
        bal = GetBalance(wallet);
        if (add_mine) assert(bal.m_mine_trusted > 0);
    });
}

static void WalletBalanceDirty(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ true, /* add_mine */ true, 2500); }
static void WalletBalanceClean(benchmark::Bench& bench) {WalletBalance(bench, /* set_dirty */ false, /* add_mine */ true, 8000); }
static void WalletBalanceMine(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_mine */ true, 16000); }
static void WalletBalanceWatch(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_mine */ false, 8000); }

BENCHMARK(WalletBalanceDirty);
BENCHMARK(WalletBalanceClean);
BENCHMARK(WalletBalanceMine);
BENCHMARK(WalletBalanceWatch);
