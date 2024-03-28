// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <outputtype.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {

TestChain100Setup* g_setup;

/**
 * Singleton wallet class ensures that only one
 * instance of CWallet is created and reused as required.
 * This increases Fuzzing efficiency by reducing the expense
 * of creating a new `Descriptor Wallet` each time and deletes
 * the pointer safely using the destructor.
 */
class WalletSingleton
{
public:
    static WalletSingleton& GetInstance()
    {
        static WalletSingleton instance;
        return instance;
    }

    CWallet& GetWallet()
    {
        if (!wallet) {
            InitializeWallet();
        }
        return *wallet;
    }
    CoinsResult& GetCoins()
    {
        if (!wallet) {
            InitializeWallet();
        }
        return available_coins;
    }

private:
    WalletSingleton()
    {
        InitializeWallet();
    }

    ~WalletSingleton()
    {
        wallet.reset();
        available_coins.coins.clear();
    }

    WalletSingleton(const WalletSingleton&) = delete;
    WalletSingleton& operator=(const WalletSingleton&) = delete;

    void InitializeWallet()
    {
        auto& node{g_setup->m_node};
        wallet = CreateSyncedWallet(*node.chain, WITH_LOCK(Assert(node.chainman)->GetMutex(), return node.chainman->ActiveChain()), g_setup->coinbaseKey);

        // This is placed here because Available Coins will never be updated while fuzzing.
        LOCK(wallet->cs_wallet);
        available_coins = AvailableCoins(*wallet);
    }

    std::unique_ptr<CWallet> wallet;
    CoinsResult available_coins;
};

CCoinControl GetNewCoinControl(FuzzedDataProvider& fuzzed_data_provider, CWallet& wallet, std::vector<COutput>& coins)
{
    CCoinControl new_coin_control;
    if (fuzzed_data_provider.ConsumeBool()) {
        const CTxDestination tx_destination{ConsumeTxDestination(fuzzed_data_provider)};
        new_coin_control.destChange = tx_destination;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        new_coin_control.m_change_type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
    }
    new_coin_control.m_include_unsafe_inputs = fuzzed_data_provider.ConsumeBool();
    new_coin_control.m_allow_other_inputs = fuzzed_data_provider.ConsumeBool();
    if (fuzzed_data_provider.ConsumeBool()) {
        new_coin_control.m_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    }
    new_coin_control.m_signal_bip125_rbf = fuzzed_data_provider.ConsumeBool();
    new_coin_control.m_avoid_partial_spends = fuzzed_data_provider.ConsumeBool();

    // Taking a random sub-sequence from available coins.
    for (const COutput& coin : coins) {
        if (fuzzed_data_provider.ConsumeBool()) {
            new_coin_control.Select(coin.outpoint);
        }
    }

    // Occasionally, some random `CCoutPoint` are also selected.
    if (fuzzed_data_provider.ConsumeBool()) {
        for (int i = 0; i < 10; ++i) {
            std::optional<COutPoint> optional_out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
            if (!optional_out_point) {
                continue;
            }
            new_coin_control.Select(*optional_out_point);
        }
    }
    return new_coin_control;
}

void initialize_spend()
{
    static auto testing_setup = MakeNoLogFileContext<TestChain100Setup>();
    g_setup = testing_setup.get();

    // Add 50 spendable UTXO, 50 BTC each, to the wallet (total balance 5000 BTC)
    // Because none of the UTXO will ever be used (marked as spent), mining this many should be sufficient.
    for (int i = 0; i < 50; ++i) {
        g_setup->CreateAndProcessBlock({}, GetScriptForRawPubKey(g_setup->coinbaseKey.GetPubKey()));
    }
}

FUZZ_TARGET(spend, .init = initialize_spend)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CWallet& wallet = WalletSingleton::GetInstance().GetWallet();
    CoinsResult& available_coins = WalletSingleton::GetInstance().GetCoins();

    // Asserting if the `wallet` has no funds.
    std::vector<COutput> coins = available_coins.All();
    assert(!coins.empty());

    CCoinControl coin_control;
    CMutableTransaction mtx;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 505)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Creates a new `CoinControl`.
                coin_control = GetNewCoinControl(fuzzed_data_provider, wallet, coins);
            },
            [&] {
                // Creates a new Transaction using `CreateTransaction`.
                std::vector<CRecipient> recipients;
                unsigned int recipients_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 100);
                for (unsigned int i = 0; i < recipients_size; ++i) {
                    recipients.push_back({/*dest=*/ConsumeTxDestination(fuzzed_data_provider),
                                          /*nAmount=*/ConsumeMoney(fuzzed_data_provider),
                                          /*fSubtractFeeFromAmount=*/fuzzed_data_provider.ConsumeBool()});
                }
                // Equally likely to choose one of the three values.
                std::vector<int> random_positions = {-1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, recipients_size), fuzzed_data_provider.ConsumeIntegral<int>()};
                int change_pos = PickValue(fuzzed_data_provider, random_positions);

                auto res = CreateTransaction(wallet, recipients, change_pos, coin_control, /*sign=*/fuzzed_data_provider.ConsumeBool());
                if (!res) return;
                mtx = CMutableTransaction(*(res->tx));
            },
            [&] {
                // Occasionally, creating a random Transaction for Fuzzing.
                auto new_mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
                if (!new_mtx) return;
                mtx = *new_mtx;
            },
            [&] {
                LOCK(wallet.cs_wallet);
                if (fuzzed_data_provider.ConsumeBool()) {
                    (void)CalculateMaximumSignedTxSize(CTransaction(mtx), &wallet);
                } else {
                    (void)CalculateMaximumSignedTxSize(CTransaction(mtx), &wallet, &coin_control);
                }
            },
            [&] {
                const CTxOut tx_out{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)};
                CalculateMaximumSignedInputSize(tx_out, &wallet, &coin_control);
            },
            [&] {
                CAmount fee;
                std::vector<int> random_positions = {-1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, mtx.vout.size()), fuzzed_data_provider.ConsumeIntegral<int>()};
                int change_pos = PickValue(fuzzed_data_provider, random_positions);
                bilingual_str error;
                std::set<int> setSubtractFeeFromOutputs;
                for (size_t i = 0; i < mtx.vout.size(); ++i) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        setSubtractFeeFromOutputs.insert(i);
                    }
                }
                FundTransaction(wallet, mtx, fee, change_pos, error, false, setSubtractFeeFromOutputs, coin_control);
            });
    }

    // Plays with the `CoinsResult`
    CoinsResult wallet_coins;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 50)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Re-initializing the wallet_coins.
                wallet_coins = available_coins;
            },
            [&] {
                std::optional<COutPoint> optional_out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!optional_out_point) {
                    return;
                }
                COutput out_put = COutput{/*outpoint=*/*optional_out_point,
                                          /*txout=*/CTxOut{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)},
                                          /*depth=*/fuzzed_data_provider.ConsumeIntegral<int>(),
                                          /*input_bytes=*/fuzzed_data_provider.ConsumeIntegral<int>(),
                                          /*spendable=*/fuzzed_data_provider.ConsumeBool(),
                                          /*solvable=*/fuzzed_data_provider.ConsumeBool(),
                                          /*safe=*/fuzzed_data_provider.ConsumeBool(),
                                          /*time=*/fuzzed_data_provider.ConsumeIntegral<int64_t>(),
                                          /*from_me=*/fuzzed_data_provider.ConsumeBool()};
                OutputType type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
                wallet_coins.Add(type, out_put);
            },
            [&] {
                std::unordered_set<COutPoint, SaltedOutpointHasher> outs_to_remove;
                std::vector<COutput> coins = wallet_coins.All();
                // Taking a random sub-sequence from available coins
                for (const COutput& coin : coins) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        outs_to_remove.emplace(coin.outpoint);
                    }
                }
                wallet_coins.Erase(outs_to_remove);
            },
            [&] {
                FastRandomContext random_context{ConsumeUInt256(fuzzed_data_provider)};
                wallet_coins.Shuffle(random_context);
            },
            [&] {
                wallet_coins.Clear();
            },
            [&] {
                (void)wallet_coins.GetTotalAmount();
            },
            [&] {
                (void)wallet_coins.GetEffectiveTotalAmount();
            });
    }
}
} // namespace
} // namespace wallet
