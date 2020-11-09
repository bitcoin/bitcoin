// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <optional.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <test/util/wallet.h>
#include <validationinterface.h>
#include <wallet/wallet.h>

static void WalletBalance(benchmark::Bench& bench, const bool set_dirty, const bool add_watchonly, const bool add_mine)
{
    TestingSetup test_setup{
        CBaseChainParams::REGTEST,
        /* extra_args */ {
            "-nodebuglogfile",
            "-nodebug",
        },
    };

    const auto& ADDRESS_WATCHONLY = ADDRESS_BCRT1_UNSPENDABLE;

    NodeContext node;
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain(node);
    CWallet wallet{chain.get(), "", CreateMockWalletDatabase()};
    {
        wallet.SetupLegacyScriptPubKeyMan();
        bool first_run;
        if (wallet.LoadWallet(first_run) != DBErrors::LOAD_OK) assert(false);
    }
    auto handler = chain->handleNotifications({&wallet, [](CWallet*) {}});

    const Optional<std::string> address_mine{add_mine ? Optional<std::string>{getnewaddress(wallet)} : nullopt};
    if (add_watchonly) importaddress(wallet, ADDRESS_WATCHONLY);

    for (int i = 0; i < 100; ++i) {
        generatetoaddress(test_setup.m_node, address_mine.get_value_or(ADDRESS_WATCHONLY));
        generatetoaddress(test_setup.m_node, ADDRESS_WATCHONLY);
    }
    SyncWithValidationInterfaceQueue();

    auto bal = wallet.GetBalance(); // Cache

    bench.run([&] {
        if (set_dirty) wallet.MarkDirty();
        bal = wallet.GetBalance();
        if (add_mine) assert(bal.m_mine_trusted > 0);
        if (add_watchonly) assert(bal.m_watchonly_trusted > 0);
    });
}

static void WalletBalanceDirty(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ true, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceClean(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ true); }
static void WalletBalanceMine(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ false, /* add_mine */ true); }
static void WalletBalanceWatch(benchmark::Bench& bench) { WalletBalance(bench, /* set_dirty */ false, /* add_watchonly */ true, /* add_mine */ false); }

BENCHMARK(WalletBalanceDirty);
BENCHMARK(WalletBalanceClean);
BENCHMARK(WalletBalanceMine);
BENCHMARK(WalletBalanceWatch);
