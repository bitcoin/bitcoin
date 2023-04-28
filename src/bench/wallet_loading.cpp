// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <wallet/test/util.h>
#include <util/translation.h>
#include <validationinterface.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>

#include <optional>

using wallet::CWallet;
using wallet::DatabaseFormat;
using wallet::DatabaseOptions;
using wallet::TxStateInactive;
using wallet::WALLET_FLAG_DESCRIPTORS;
using wallet::WalletContext;
using wallet::WalletDatabase;

static std::shared_ptr<CWallet> BenchLoadWallet(std::unique_ptr<WalletDatabase> database, WalletContext& context, DatabaseOptions& options)
{
    bilingual_str error;
    std::vector<bilingual_str> warnings;
    auto wallet = CWallet::Create(context, "", std::move(database), options.create_flags, error, warnings);
    NotifyWalletLoaded(context, wallet);
    if (context.chain) {
        wallet->postInitProcess();
    }
    return wallet;
}

static void BenchUnloadWallet(std::shared_ptr<CWallet>&& wallet)
{
    SyncWithValidationInterfaceQueue();
    wallet->m_chain_notifications_handler.reset();
    UnloadWallet(std::move(wallet));
}

static void AddTx(CWallet& wallet)
{
    CMutableTransaction mtx;
    mtx.vout.push_back({COIN, GetScriptForDestination(*Assert(wallet.GetNewDestination(OutputType::BECH32, "")))});
    mtx.vin.push_back(CTxIn());

    wallet.AddToWallet(MakeTransactionRef(mtx), TxStateInactive{});
}

static void WalletLoading(benchmark::Bench& bench, bool legacy_wallet)
{
    const auto test_setup = MakeNoLogFileContext<TestingSetup>();
    test_setup->m_args.ForceSetArg("-unsafesqlitesync", "1");

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();

    // Setup the wallet
    // Loading the wallet will also create it
    DatabaseOptions options;
    if (legacy_wallet) {
        options.require_format = DatabaseFormat::BERKELEY;
    } else {
        options.create_flags = WALLET_FLAG_DESCRIPTORS;
        options.require_format = DatabaseFormat::SQLITE;
    }
    auto database = CreateMockWalletDatabase(options);
    auto wallet = BenchLoadWallet(std::move(database), context, options);

    // Generate a bunch of transactions and addresses to put into the wallet
    for (int i = 0; i < 1000; ++i) {
        AddTx(*wallet);
    }

    database = DuplicateMockDatabase(wallet->GetDatabase(), options);

    // reload the wallet for the actual benchmark
    BenchUnloadWallet(std::move(wallet));

    bench.epochs(5).run([&] {
        wallet = BenchLoadWallet(std::move(database), context, options);

        // Cleanup
        database = DuplicateMockDatabase(wallet->GetDatabase(), options);
        BenchUnloadWallet(std::move(wallet));
    });
}

#ifdef USE_BDB
static void WalletLoadingLegacy(benchmark::Bench& bench) { WalletLoading(bench, /*legacy_wallet=*/true); }
BENCHMARK(WalletLoadingLegacy, benchmark::PriorityLevel::HIGH);
#endif

#ifdef USE_SQLITE
static void WalletLoadingDescriptors(benchmark::Bench& bench) { WalletLoading(bench, /*legacy_wallet=*/false); }
BENCHMARK(WalletLoadingDescriptors, benchmark::PriorityLevel::HIGH);
#endif
