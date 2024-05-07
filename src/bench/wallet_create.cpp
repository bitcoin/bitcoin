// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <bench/bench.h>
#include <node/context.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <wallet/context.h>
#include <wallet/wallet.h>

namespace wallet {
static void WalletCreate(benchmark::Bench& bench, bool encrypted)
{
    auto test_setup = MakeNoLogFileContext<TestingSetup>();
    FastRandomContext random;

    WalletContext context;
    context.args = &test_setup->m_args;
    context.chain = test_setup->m_node.chain.get();

    DatabaseOptions options;
    options.require_format = DatabaseFormat::SQLITE;
    options.require_create = true;
    options.create_flags = WALLET_FLAG_DESCRIPTORS;

    if (encrypted) {
        options.create_passphrase = random.rand256().ToString();
    }

    DatabaseStatus status;
    bilingual_str error_string;
    std::vector<bilingual_str> warnings;

    fs::path wallet_path = test_setup->m_path_root / strprintf("test_wallet_%d", random.rand32()).c_str();
    bench.run([&] {
        auto wallet = CreateWallet(context, wallet_path.utf8string(), /*load_on_start=*/std::nullopt, options, status, error_string, warnings);
        assert(status == DatabaseStatus::SUCCESS);
        assert(wallet != nullptr);

        // Cleanup
        wallet.reset();
        fs::remove_all(wallet_path);
    });
}

static void WalletCreatePlain(benchmark::Bench& bench) { WalletCreate(bench, /*encrypted=*/false); }
static void WalletCreateEncrypted(benchmark::Bench& bench) { WalletCreate(bench, /*encrypted=*/true); }

#ifndef _MSC_VER
// TODO: Being built with MSVC, the fs::remove_all() call in
// the WalletCreate() fails with the error "The process cannot
// access the file because it is being used by another process."
#ifdef USE_SQLITE
BENCHMARK(WalletCreatePlain, benchmark::PriorityLevel::LOW);
BENCHMARK(WalletCreateEncrypted, benchmark::PriorityLevel::LOW);
#endif
#endif

} // namespace wallet
