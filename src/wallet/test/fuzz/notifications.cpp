// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/amount.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/result.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace wallet {
namespace {
const TestingSetup* g_setup;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

void ImportDescriptors(CWallet& wallet, const std::string& seed_insecure)
{
    const std::vector<std::string> DESCS{
        "pkh(%s/%s/*)",
        "sh(wpkh(%s/%s/*))",
        "tr(%s/%s/*)",
        "wpkh(%s/%s/*)",
    };

    for (const std::string& desc_fmt : DESCS) {
        for (bool internal : {true, false}) {
            const auto descriptor{(strprintf)(desc_fmt, "[5aa9973a/66h/4h/2h]" + seed_insecure, int{internal})};

            FlatSigningProvider keys;
            std::string error;
            auto parsed_desc = Parse(descriptor, keys, error, /*require_checksum=*/false);
            assert(parsed_desc);
            assert(error.empty());
            assert(parsed_desc->IsRange());
            assert(parsed_desc->IsSingleType());
            assert(!keys.keys.empty());
            WalletDescriptor w_desc{std::move(parsed_desc), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0};
            assert(!wallet.GetDescriptorScriptPubKeyMan(w_desc));
            LOCK(wallet.cs_wallet);
            auto spk_manager{wallet.AddWalletDescriptor(w_desc, keys, /*label=*/"", internal)};
            assert(spk_manager);
            wallet.AddActiveScriptPubKeyMan(spk_manager->GetID(), *Assert(w_desc.descriptor->GetOutputType()), internal);
        }
    }
}

/**
 * Wraps a descriptor wallet for fuzzing.
 */
struct FuzzedWallet {
    std::shared_ptr<CWallet> wallet;
    FuzzedWallet(const std::string& name, const std::string& seed_insecure)
    {
        auto& chain{*Assert(g_setup->m_node.chain)};
        wallet = std::make_shared<CWallet>(&chain, name, CreateMockableWalletDatabase());
        {
            LOCK(wallet->cs_wallet);
            wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            auto height{*Assert(chain.getHeight())};
            wallet->SetLastBlockProcessed(height, chain.getBlockHash(height));
        }
        wallet->m_keypool_size = 1; // Avoid timeout in TopUp()
        assert(wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
        ImportDescriptors(*wallet, seed_insecure);
    }
    CTxDestination GetDestination(FuzzedDataProvider& fuzzed_data_provider)
    {
        auto type{fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)};
        if (fuzzed_data_provider.ConsumeBool()) {
            return *Assert(wallet->GetNewDestination(type, ""));
        } else {
            return *Assert(wallet->GetNewChangeDestination(type));
        }
    }
    CScript GetScriptPubKey(FuzzedDataProvider& fuzzed_data_provider) { return GetScriptForDestination(GetDestination(fuzzed_data_provider)); }
    void FundTx(FuzzedDataProvider& fuzzed_data_provider, CMutableTransaction tx)
    {
        // The fee of "tx" is 0, so this is the total input and output amount
        const CAmount total_amt{
            std::accumulate(tx.vout.begin(), tx.vout.end(), CAmount{}, [](CAmount t, const CTxOut& out) { return t + out.nValue; })};
        const uint32_t tx_size(GetVirtualTransactionSize(CTransaction{tx}));
        std::set<int> subtract_fee_from_outputs;
        if (fuzzed_data_provider.ConsumeBool()) {
            for (size_t i{}; i < tx.vout.size(); ++i) {
                if (fuzzed_data_provider.ConsumeBool()) {
                    subtract_fee_from_outputs.insert(i);
                }
            }
        }
        std::vector<CRecipient> recipients;
        for (size_t idx = 0; idx < tx.vout.size(); idx++) {
            const CTxOut& tx_out = tx.vout[idx];
            CTxDestination dest;
            ExtractDestination(tx_out.scriptPubKey, dest);
            CRecipient recipient = {dest, tx_out.nValue, subtract_fee_from_outputs.count(idx) == 1};
            recipients.push_back(recipient);
        }
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = fuzzed_data_provider.ConsumeBool();
        CallOneOf(
            fuzzed_data_provider, [&] { coin_control.destChange = GetDestination(fuzzed_data_provider); },
            [&] { coin_control.m_change_type.emplace(fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)); },
            [&] { /* no op (leave uninitialized) */ });
        coin_control.fAllowWatchOnly = fuzzed_data_provider.ConsumeBool();
        coin_control.m_include_unsafe_inputs = fuzzed_data_provider.ConsumeBool();
        {
            auto& r{coin_control.m_signal_bip125_rbf};
            CallOneOf(
                fuzzed_data_provider, [&] { r = true; }, [&] { r = false; }, [&] { r = std::nullopt; });
        }
        coin_control.m_feerate = CFeeRate{
            // A fee of this range should cover all cases
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, 2 * total_amt),
            tx_size,
        };
        if (fuzzed_data_provider.ConsumeBool()) {
            *coin_control.m_feerate += GetMinimumFeeRate(*wallet, coin_control, nullptr);
        }
        coin_control.fOverrideFeeRate = fuzzed_data_provider.ConsumeBool();
        // Add solving data (m_external_provider and SelectExternal)?

        int change_position{fuzzed_data_provider.ConsumeIntegralInRange<int>(-1, tx.vout.size() - 1)};
        bilingual_str error;
        // Clear tx.vout since it is not meant to be used now that we are passing outputs directly.
        // This sets us up for a future PR to completely remove tx from the function signature in favor of passing inputs directly
        tx.vout.clear();
        (void)FundTransaction(*wallet, tx, recipients, change_position, /*lockUnspents=*/false, coin_control);
    }
};

FUZZ_TARGET(wallet_notifications, .init = initialize_setup)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    // The total amount, to be distributed to the wallets a and b in txs
    // without fee. Thus, the balance of the wallets should always equal the
    // total amount.
    const auto total_amount{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY / 100000)};
    FuzzedWallet a{
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };
    FuzzedWallet b{
        "fuzzed_wallet_b",
        "tprv8ZgxMBicQKsPfCunYTF18sEmEyjz8TfhGnZ3BoVAhkqLv7PLkQgmoG2Ecsp4JuqciWnkopuEwShit7st743fdmB9cMD4tznUkcs33vK51K9",
    };

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
        coins.emplace(total_amount, COutPoint{Txid::FromUint256(uint256::ONE), 1});
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
                    LIMITED_WHILE(in > 0 && fuzzed_data_provider.ConsumeBool(), 10)
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
                    // Check that funding the tx doesn't crash the wallet
                    a.FundTx(fuzzed_data_provider, tx);
                    b.FundTx(fuzzed_data_provider, tx);
                }
                // Mine block
                const uint256& hash = block.GetHash();
                interfaces::BlockInfo info{hash};
                info.prev_hash = &block.hashPrevBlock;
                info.height = chain.size();
                info.data = &block;
                // Ensure that no blocks are skipped by the wallet by setting the chain's accumulated
                // time to the maximum value. This ensures that the wallet's birth time is always
                // earlier than this maximum time.
                info.chain_time_max = std::numeric_limits<unsigned int>::max();
                a.wallet->blockConnected(ChainstateRole::NORMAL, info);
                b.wallet->blockConnected(ChainstateRole::NORMAL, info);
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
                const uint256& hash = block.GetHash();
                interfaces::BlockInfo info{hash};
                info.prev_hash = &block.hashPrevBlock;
                info.height = chain.size() - 1;
                info.data = &block;
                a.wallet->blockDisconnected(info);
                b.wallet->blockDisconnected(info);
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
} // namespace wallet
