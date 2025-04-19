// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <outputtype.h>
#include <random.h>
#include <support/allocators/secure.h>
#include <test/util/setup_common.h>
#include <wallet/context.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <cassert>

namespace wallet {
static void WalletEncrypt(benchmark::Bench& bench, bool legacy_wallet, bool measure_overhead)
{
    auto test_setup = MakeNoLogFileContext<TestingSetup>();
    FastRandomContext random;

    auto password{random.randbytes(20)};
    SecureString secure_pass{password.begin(), password.end()};

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();
    uint64_t create_flags{0};
    if(!legacy_wallet) {
        create_flags = WALLET_FLAG_DESCRIPTORS;
    }

    auto database{CreateMockableWalletDatabase()};
    auto wallet{TestLoadWallet(std::move(database), context, create_flags)};

    int key_count{0};

    if(!legacy_wallet) {
        // Add destinations
        for(auto type : OUTPUT_TYPES) {
            for(int i = 0; i < 10'000; i++) {
                CMutableTransaction mtx;
                mtx.vout.emplace_back(COIN, GetScriptForDestination(*Assert(wallet->GetNewDestination(type, ""))));
                mtx.vin.emplace_back();
                wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{});
                key_count++;
            }
        }
    }
    else {
        LegacyDataSPKM* legacy_spkm = wallet->GetOrCreateLegacyDataSPKM();
        /* legacy spkm */
        for(size_t i = 0; i < 10'000 * OUTPUT_TYPES.size(); i++) {
            CKey key = GenerateRandomKey();
            CPubKey pubkey = key.GetPubKey();
            // Load key, scripts and create address book record
            Assert(legacy_spkm->LoadKey(key, pubkey));
            CTxDestination dest{PKHash(pubkey)};
            Assert(wallet->SetAddressBook(dest, strprintf("legacy_%d", i), /*purpose=*/std::nullopt));

            CMutableTransaction mtx;
            mtx.vout.emplace_back(COIN, GetScriptForDestination(dest));
            mtx.vin.emplace_back();
            wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{});
            key_count++;
        }
    }

    database = DuplicateMockDatabase(wallet->GetDatabase());

    // reload the wallet for the actual benchmark
    TestUnloadWallet(std::move(wallet));

    bench.batch(key_count).unit("key").run([&] {
        wallet = TestLoadWallet(std::move(database), context, create_flags);

        // Save a copy of the db before encrypting
        database = DuplicateMockDatabase(wallet->GetDatabase());

        // Skip actually encrypting wallet on the overhead measuring run, so we
        // can subtract the overhead from the results.
        if(!measure_overhead) {
            wallet->EncryptWallet(secure_pass, 25000);
        }

        // cleanup
        TestUnloadWallet(std::move(wallet));
    });
}

static void WalletEncryptDescriptors(benchmark::Bench& bench) { WalletEncrypt(bench, /*legacy_wallet=*/false, /*measure_overhead=*/false); }
static void WalletEncryptLegacy(benchmark::Bench& bench) { WalletEncrypt(bench, /*legacy_wallet=*/true, /*measure_overhead=*/false); }

BENCHMARK(WalletEncryptDescriptors, benchmark::PriorityLevel::HIGH);
BENCHMARK(WalletEncryptLegacy, benchmark::PriorityLevel::HIGH);

static void WalletEncryptDescriptorsBenchOverhead(benchmark::Bench& bench) { WalletEncrypt(bench, /*legacy_wallet=*/false, /*measure_overhead=*/true); }
static void WalletEncryptLegacyBenchOverhead(benchmark::Bench& bench) { WalletEncrypt(bench, /*legacy_wallet=*/true, /*measure_overhead=*/true); }

BENCHMARK(WalletEncryptDescriptorsBenchOverhead, benchmark::PriorityLevel::LOW);
BENCHMARK(WalletEncryptLegacyBenchOverhead, benchmark::PriorityLevel::LOW);

} // namespace wallet
