// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <outputtype.h>
#include <random.h>
#include <support/allocators/secure.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <wallet/context.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <cassert>

namespace wallet {
static void WalletEncrypt(benchmark::Bench& bench, bool measure_overhead)
{
    auto test_setup = MakeNoLogFileContext<TestingSetup>();
    FastRandomContext random;

    auto password{random.randbytes(20)};
    SecureString secure_pass{password.begin(), password.end()};

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();
    uint64_t create_flags{WALLET_FLAG_DESCRIPTORS};

    auto database{CreateMockableWalletDatabase()};
    auto wallet{TestLoadWallet(std::move(database), context, create_flags)};

    unsigned int key_count{0};

    // Add destinations
    for (auto type : OUTPUT_TYPES) {
        for (int i = 0; i < 5'000; i++) {
            CMutableTransaction mtx;
            mtx.vout.emplace_back(COIN, GetScriptForDestination(*Assert(wallet->GetNewDestination(type, ""))));
            mtx.vin.emplace_back();
            wallet->AddToWallet(MakeTransactionRef(mtx), TxStateInactive{});
            key_count++;
        }
    }

    database = DuplicateMockDatabase(wallet->GetDatabase());

    // reload the wallet for the actual benchmark
    TestUnloadWallet(std::move(wallet));

    // Setting a mock time is necessary to force default derive iteration count during
    // wallet encryption.
    SetMockTime(1);
    bench.batch(key_count).unit("key").run([&] {
        wallet = TestLoadWallet(std::move(database), context, create_flags);

        // Save a copy of the db before encrypting
        database = DuplicateMockDatabase(wallet->GetDatabase());

        // Skip actually encrypting wallet on the overhead measuring run, so we
        // can subtract the overhead from the full benchmark results to get
        // encryption performance.
        if (!measure_overhead) {
            wallet->EncryptWallet(secure_pass);
        }

        for (const auto& [_, key] : wallet->mapMasterKeys){
            assert(key.nDeriveIterations == CMasterKey::DEFAULT_DERIVE_ITERATIONS);
        }

        // cleanup
        TestUnloadWallet(std::move(wallet));
    });
}

static void WalletEncryptDescriptors(benchmark::Bench& bench) { WalletEncrypt(bench, /*measure_overhead=*/false); }

BENCHMARK(WalletEncryptDescriptors, benchmark::PriorityLevel::HIGH);

static void WalletEncryptDescriptorsBenchOverhead(benchmark::Bench& bench) { WalletEncrypt(bench, /*measure_overhead=*/true); }

BENCHMARK(WalletEncryptDescriptorsBenchOverhead, benchmark::PriorityLevel::LOW);

} // namespace wallet
