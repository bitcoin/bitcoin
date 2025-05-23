// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <kernel/chain.h>
#include <node/context.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <optional>

namespace wallet{

static void WalletMigration(benchmark::Bench& bench)
{
    const auto test_setup{MakeNoLogFileContext<TestingSetup>()};
    const auto loader{MakeWalletLoader(*test_setup->m_node.chain, test_setup->m_args)};

    // Number of imported watch only addresses
    int NUM_WATCH_ONLY_ADDR = 20;

    // Setup legacy wallet
    std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase());
    LegacyDataSPKM* legacy_spkm = wallet->GetOrCreateLegacyDataSPKM();
    WalletBatch batch{wallet->GetDatabase()};

    // Write a best block record as migration expects one to exist
    CBlockLocator loc;
    batch.WriteBestBlock(loc);

    // Add watch-only addresses
    std::vector<CScript> scripts_watch_only;
    for (int w = 0; w < NUM_WATCH_ONLY_ADDR; ++w) {
        CKey key = GenerateRandomKey();
        LOCK(wallet->cs_wallet);
        const PKHash dest{key.GetPubKey()};
        const CScript& script = scripts_watch_only.emplace_back(GetScriptForDestination(dest));
        assert(legacy_spkm->LoadWatchOnly(script));
        assert(wallet->SetAddressBook(dest, strprintf("watch_%d", w), /*purpose=*/std::nullopt));
        batch.WriteWatchOnly(script, CKeyMetadata());
    }

    // Generate transactions and local addresses
    for (int j = 0; j < 500; ++j) {
        CKey key = GenerateRandomKey();
        CPubKey pubkey = key.GetPubKey();
        // Load key, scripts and create address book record
        Assert(legacy_spkm->LoadKey(key, pubkey));
        CTxDestination dest{PKHash(pubkey)};
        Assert(wallet->SetAddressBook(dest, strprintf("legacy_%d", j), /*purpose=*/std::nullopt));

        CMutableTransaction mtx;
        mtx.vout.emplace_back(COIN, GetScriptForDestination(dest));
        mtx.vout.emplace_back(COIN, scripts_watch_only.at(j % NUM_WATCH_ONLY_ADDR));
        mtx.vin.resize(2);
        wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{}, /*update_wtx=*/nullptr, /*rescanning_old_block=*/true);
        batch.WriteKey(pubkey, key.GetPrivKey(), CKeyMetadata());
    }

    bench.epochs(/*numEpochs=*/1).epochIterations(/*numIters=*/1) // run the migration exactly once
         .run([&] {
             auto res{MigrateLegacyToDescriptor(std::move(wallet), /*passphrase=*/"", *loader->context(), /*was_loaded=*/false)};
             assert(res);
             assert(res->wallet);
             assert(res->watchonly_wallet);
         });
}

BENCHMARK(WalletMigration, benchmark::PriorityLevel::LOW);

} // namespace wallet
