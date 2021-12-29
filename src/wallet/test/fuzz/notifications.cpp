// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/translation.h>
#include <wallet/context.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
const TestingSetup* g_setup;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

/**
 * Wraps a descriptor wallet for fuzzing. The constructor writes the sqlite db
 * to disk, the destructor deletes it.
 */
struct FuzzedWallet {
    ArgsManager args;
    WalletContext context;
    std::shared_ptr<CWallet> wallet;
    FuzzedWallet(const std::string& name)
    {
        context.args = &args;
        context.chain = g_setup->m_node.chain.get();

        DatabaseOptions options;
        options.require_create = true;
        options.create_flags = WALLET_FLAG_DESCRIPTORS;
        const std::optional<bool> load_on_start;
        gArgs.ForceSetArg("-keypool", "0"); // Avoid timeout in TopUp()

        DatabaseStatus status;
        bilingual_str error;
        std::vector<bilingual_str> warnings;
        wallet = CreateWallet(context, name, load_on_start, options, status, error, warnings);
        assert(wallet);
        assert(error.empty());
        assert(warnings.empty());
        assert(wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    }
    ~FuzzedWallet()
    {
        const auto name{wallet->GetName()};
        std::vector<bilingual_str> warnings;
        std::optional<bool> load_on_start;
        assert(RemoveWallet(context, wallet, load_on_start, warnings));
        assert(warnings.empty());
        UnloadWallet(std::move(wallet));
        fs::remove_all(GetWalletDir() / name);
    }
    CScript GetScriptPubKey(FuzzedDataProvider& fuzzed_data_provider)
    {
        auto type{fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)};
        CTxDestination dest;
        bilingual_str error;
        if (fuzzed_data_provider.ConsumeBool()) {
            assert(wallet->GetNewDestination(type, "", dest, error));
        } else {
            assert(wallet->GetNewChangeDestination(type, dest, error));
        }
        assert(error.empty());
        return GetScriptForDestination(dest);
    }
};

FUZZ_TARGET_INIT(wallet_notifications, initialize_setup)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    // The total amount, to be distributed to the wallets a and b in txs
    // without fee. Thus, the balance of the wallets should always equal the
    // total amount.
    const auto total_amount{ConsumeMoney(fuzzed_data_provider)};
    FuzzedWallet a{"fuzzed_wallet_a"};
    FuzzedWallet b{"fuzzed_wallet_b"};

    // Keep track of all coins in this test.
    // Each tuple in the chain represents the coins and the block created with
    // those coins. Once the block is mined, the next tuple will have an empty
    // block and the freshly mined coins.
    using Coins = std::set<std::tuple<CAmount, COutPoint>>;
    std::vector<std::tuple<Coins, CBlock>> chain;
    {
        // Add the initial entry
        chain.emplace_back();
        auto& [coins, block]{chain.back()};
        coins.emplace(total_amount, COutPoint{uint256::ONE, 1});
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 200)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                auto& [coins_orig, block]{chain.back()};
                // Copy the coins for this block and consume all of them
                Coins coins = coins_orig;
                while (!coins.empty()) {
                    // Create a new tx
                    CMutableTransaction tx{};
                    // Add some coins as inputs to it
                    auto num_inputs{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, coins.size())};
                    CAmount in{0};
                    while (num_inputs-- > 0) {
                        const auto& [coin_amt, coin_outpoint]{*coins.begin()};
                        in += coin_amt;
                        tx.vin.emplace_back(coin_outpoint);
                        coins.erase(coins.begin());
                    }
                    // Create some outputs spending all inputs, without fee
                    LIMITED_WHILE(in > 0 && fuzzed_data_provider.ConsumeBool(), 100)
                    {
                        const auto out_value{ConsumeMoney(fuzzed_data_provider, in)};
                        in -= out_value;
                        auto& wallet{fuzzed_data_provider.ConsumeBool() ? a : b};
                        tx.vout.emplace_back(out_value, wallet.GetScriptPubKey(fuzzed_data_provider));
                    }
                    // Spend the remaining input value, if any
                    auto& wallet{fuzzed_data_provider.ConsumeBool() ? a : b};
                    tx.vout.emplace_back(in, wallet.GetScriptPubKey(fuzzed_data_provider));
                    // Add tx to block
                    block.vtx.emplace_back(MakeTransactionRef(tx));
                }
                // Mine block
                a.wallet->blockConnected(block, chain.size());
                b.wallet->blockConnected(block, chain.size());
                // Store the coins for the next block
                Coins coins_new;
                for (const auto& tx : block.vtx) {
                    uint32_t i{0};
                    for (const auto& out : tx->vout) {
                        coins_new.emplace(out.nValue, COutPoint{tx->GetHash(), i++});
                    }
                }
                chain.emplace_back(coins_new, CBlock{});
            },
            [&] {
                if (chain.size() <= 1) return; // The first entry can't be removed
                auto& [coins, block]{chain.back()};
                if (block.vtx.empty()) return; // Can only disconnect if the block was submitted first
                // Disconnect block
                a.wallet->blockDisconnected(block, chain.size() - 1);
                b.wallet->blockDisconnected(block, chain.size() - 1);
                chain.pop_back();
            });
        auto& [coins, first_block]{chain.front()};
        if (!first_block.vtx.empty()) {
            // Only check balance when at least one block was submitted
            const auto bal_a{GetBalance(*a.wallet).m_mine_trusted};
            const auto bal_b{GetBalance(*b.wallet).m_mine_trusted};
            assert(total_amount == bal_a + bal_b);
        }
    }
}
} // namespace
