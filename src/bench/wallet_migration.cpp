// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <bench/bench.h>
#include <kernel/chain.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <wallet/test/util.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>

#include <optional>

#if defined(USE_SQLITE) // only enable benchmark when sqlite is enabled

namespace wallet{

static void WalletMigration(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestingSetup>();

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();

    // Number of imported watch only addresses
    int NUM_WATCH_ONLY_ADDR = 20;

    // Setup legacy wallet
    std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(test_setup->m_node.chain.get(), "", CreateMockableWalletDatabase());
    wallet->chainStateFlushed(ChainstateRole::NORMAL, CBlockLocator{});
    LegacyDataSPKM* legacy_spkm = wallet->GetOrCreateLegacyDataSPKM();

    // Add watch-only addresses
    std::vector<CScript> scripts_watch_only;
    for (int w = 0; w < NUM_WATCH_ONLY_ADDR; ++w) {
        CKey key = GenerateRandomKey();
        LOCK(wallet->cs_wallet);
        const auto& dest = GetDestinationForKey(key.GetPubKey(), OutputType::LEGACY);
        const CScript& script = scripts_watch_only.emplace_back(GetScriptForDestination(dest));
        assert(legacy_spkm->LoadWatchOnly(script));
        assert(wallet->SetAddressBook(dest, strprintf("watch_%d", w), /*purpose=*/std::nullopt));
    }

    // Generate transactions and local addresses
    for (int j = 0; j < 500; ++j) {
        CKey key = GenerateRandomKey();
        CPubKey pubkey = key.GetPubKey();
        Assert(legacy_spkm->LoadKey(key, pubkey));
        // Load spendable scripts and create address book records
        bool is_bech32 = j % 2;
        CTxDestination dest = is_bech32 ? WitnessV0KeyHash(pubkey) : CTxDestination{PKHash(pubkey)};
        std::string label = is_bech32 ? strprintf("bech32_%d", j) : strprintf("legacy_%d", j);
        Assert(legacy_spkm->LoadCScript(GetScriptForDestination(dest)));
        Assert(wallet->SetAddressBook(dest, label, /*purpose=*/std::nullopt));
        // Extra: replicate legacy wallet 'LearnRelatedScripts' behavior during p2pkh dest generation
        if (!is_bech32) {
            Assert(legacy_spkm->LoadCScript(GetScriptForDestination(WitnessV0KeyHash(pubkey.GetID()))));
        }

        CMutableTransaction mtx;
        mtx.vout.emplace_back(COIN, GetScriptForDestination(dest));
        mtx.vout.emplace_back(COIN, scripts_watch_only.at(j % NUM_WATCH_ONLY_ADDR));
        mtx.vin.resize(2);
        wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{}, /*update_wtx=*/nullptr, /*fFlushOnClose=*/false, /*rescanning_old_block=*/true);
    }

    bench.epochs(/*numEpochs=*/1).run([&context, &wallet] {
        util::Result<MigrationResult> res = MigrateLegacyToDescriptor(std::move(wallet), /*passphrase=*/"", context, /*was_loaded=*/false);
        assert(res);
        assert(res->wallet);
        assert(res->watchonly_wallet);
    });
}

BENCHMARK(WalletMigration, benchmark::PriorityLevel::LOW);

} // namespace wallet

#endif // end USE_SQLITE && USE_BDB
