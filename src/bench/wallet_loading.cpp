// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bench/bench.h>
#include <consensus/amount.h>
#include <outputtype.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <wallet/context.h>
#include <wallet/db.h>
#include <wallet/test/util.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace wallet{
static void AddTx(CWallet& wallet)
{
    CMutableTransaction mtx;
    mtx.vout.emplace_back(COIN, GetScriptForDestination(*Assert(wallet.GetNewDestination(OutputType::BECH32, ""))));
    mtx.vin.emplace_back();

    wallet.AddToWallet(MakeTransactionRef(mtx), TxStateInactive{});
}

static void WalletLoadingDescriptors(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestingSetup>();

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();

    // Setup the wallet
    // Loading the wallet will also create it
    uint64_t create_flags = WALLET_FLAG_DESCRIPTORS;
    auto database = CreateMockableWalletDatabase();
    auto wallet = TestLoadWallet(std::move(database), context, create_flags);

    // Generate a bunch of transactions and addresses to put into the wallet
    for (int i = 0; i < 1000; ++i) {
        AddTx(*wallet);
    }

    database = DuplicateMockDatabase(wallet->GetDatabase());

    // reload the wallet for the actual benchmark
    TestUnloadWallet(std::move(wallet));

    bench.epochs(5).run([&] {
        wallet = TestLoadWallet(std::move(database), context, create_flags);

        // Cleanup
        database = DuplicateMockDatabase(wallet->GetDatabase());
        TestUnloadWallet(std::move(wallet));
    });
}

BENCHMARK(WalletLoadingDescriptors, benchmark::PriorityLevel::HIGH);
} // namespace wallet
