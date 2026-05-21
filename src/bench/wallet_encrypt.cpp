// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <key_io.h>
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
static void WalletEncrypt(benchmark::Bench& bench, unsigned int key_count)
{
    auto test_setup = MakeNoLogFileContext<TestingSetup>();
    FastRandomContext rand{/*fDeterministic=*/true};

    auto password{rand.randbytes(20)};
    SecureString secure_pass{password.begin(), password.end()};

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();

    uint64_t create_flags = WALLET_FLAG_DESCRIPTORS;

    std::vector<std::pair<WalletDescriptor, FlatSigningProvider>> descs;
    descs.reserve(key_count);
    for (unsigned int i = 0; i < key_count; i++) {
        CKey key = GenerateRandomKey();
        FlatSigningProvider keys;
        std::string error;
        std::vector<std::unique_ptr<Descriptor>> desc = Parse("combo(" + EncodeSecret(key) + ")", keys, error, /*require_checksum=*/false);
        WalletDescriptor w_desc(std::move(desc.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/0, /*next_index=*/0);
        descs.emplace_back(w_desc, keys);
    }

    // Setting a mock time is necessary to force default derive iteration count during
    // wallet encryption.
    SetMockTime(1);

    std::unique_ptr<WalletDatabase> database;
    std::shared_ptr<CWallet> wallet;
    bench.batch(key_count).unit("key").setup([&] {
            if (wallet) {
                TestUnloadWallet(std::move(wallet));
            }

            std::unique_ptr<WalletDatabase> database = CreateMockableWalletDatabase();
            wallet = TestCreateWallet(std::move(database), context, create_flags);

            {
                LOCK(wallet->cs_wallet);
                for (auto& [desc, keys] : descs) {
                    Assert(wallet->AddWalletDescriptor(desc, keys, /*label=*/"", /*internal=*/false));
                }
            }
        })
        .run([&] {
            wallet->EncryptWallet(secure_pass);

            for (const auto& [_, key] : wallet->mapMasterKeys){
                assert(key.nDeriveIterations == CMasterKey::DEFAULT_DERIVE_ITERATIONS);
            }
        });
    TestUnloadWallet(std::move(wallet));
}

constexpr unsigned int KEY_COUNT = 2000;

static void WalletEncryptDescriptors(benchmark::Bench& bench) { WalletEncrypt(bench, /*key_count=*/KEY_COUNT); }
BENCHMARK(WalletEncryptDescriptors);

} // namespace wallet
