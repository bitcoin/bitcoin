// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <kernel/chain.h>
#include <kernel/types.h>
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

    // Add watch-only addresses
    std::vector<std::pair<CScript, CTxDestination>> scripts_watch_only;
    for (int w = 0; w < NUM_WATCH_ONLY_ADDR; ++w) {
        CKey key = GenerateRandomKey();
        const PKHash dest{key.GetPubKey()};
        scripts_watch_only.emplace_back(GetScriptForDestination(dest), dest);
    }

    // Generate transactions and local addresses
    std::vector<CKey> keys(500);
    std::ranges::generate(keys, []{ return GenerateRandomKey(); });

    std::unique_ptr<CWallet> wallet;
    size_t i = 0;
    bench.epochs(/*numEpochs=*/1) // run the migration exactly once
        .setup([&] {
            // Setup legacy wallet
            wallet = std::make_unique<CWallet>(test_setup->m_node.chain.get(), std::string(i++, 'A'), CreateMockableWalletDatabase());
            LegacyDataSPKM* legacy_spkm = wallet->GetOrCreateLegacyDataSPKM();
            WalletBatch batch{wallet->GetDatabase()};

            LOCK(wallet->cs_wallet);

            // Write a best block record as migration expects one to exist
            CBlockLocator loc;
            batch.WriteBestBlock(loc);

            // Add watch-only addresses
            for (size_t w = 0; w < scripts_watch_only.size(); ++w) {
                const auto& [script, dest] = scripts_watch_only.at(w);
                assert(legacy_spkm->LoadWatchOnly(script));
                assert(wallet->SetAddressBook(dest, strprintf("watch_%d", w), /*purpose=*/std::nullopt));
                batch.WriteWatchOnly(script, CKeyMetadata());
            }

            // Generate transactions and local addresses
            for (size_t j = 0; j < keys.size(); ++j) {
                const CKey& key = keys.at(j);
                // Load key, scripts and create address book record
                CPubKey pubkey = key.GetPubKey();
                Assert(legacy_spkm->LoadKey(key, pubkey));
                CTxDestination dest{PKHash(pubkey)};
                Assert(wallet->SetAddressBook(dest, strprintf("legacy_%d", j), /*purpose=*/std::nullopt));

                CMutableTransaction mtx;
                mtx.vout.emplace_back(COIN, GetScriptForDestination(dest));
                mtx.vout.emplace_back(COIN, scripts_watch_only.at(j % NUM_WATCH_ONLY_ADDR).first);
                mtx.vin.resize(2);
                wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{}, /*update_wtx=*/nullptr, /*rescanning_old_block=*/true);
                batch.WriteKey(pubkey, key.GetPrivKey(), CKeyMetadata());
            }
        })
        .run([&] {
            auto res{MigrateLegacyToDescriptor(std::move(wallet), /*passphrase=*/"", *loader->context())};
            assert(res);
            assert(res->wallet);
            assert(res->watchonly_wallet);
        });
}

BENCHMARK(WalletMigration);

} // namespace wallet
