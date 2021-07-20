// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <coins.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <key.h>
#include <node/coinstats.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {
const TestingSetup* g_setup;
const Coin EMPTY_COIN{};

bool operator==(const Coin& a, const Coin& b)
{
    if (a.IsSpent() && b.IsSpent()) return true;
    return a.fCoinBase == b.fCoinBase && a.nHeight == b.nHeight && a.out == b.out;
}
} // namespace

void initialize_coins_view()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(coins_view, initialize_coins_view)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CCoinsView backend_coins_view;
    CCoinsViewCache coins_view_cache{&backend_coins_view};
    COutPoint random_out_point;
    Coin random_coin;
    CMutableTransaction random_mutable_transaction;
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (random_coin.IsSpent()) {
                    return;
                }
                Coin coin = random_coin;
                bool expected_code_path = false;
                const bool possible_overwrite = fuzzed_data_provider.ConsumeBool();
                try {
                    coins_view_cache.AddCoin(random_out_point, std::move(coin), possible_overwrite);
                    expected_code_path = true;
                } catch (const std::logic_error& e) {
                    if (e.what() == std::string{"Attempted to overwrite an unspent coin (when possible_overwrite is false)"}) {
                        assert(!possible_overwrite);
                        expected_code_path = true;
                    }
                }
                assert(expected_code_path);
            },
            [&] {
                (void)coins_view_cache.Flush();
            },
            [&] {
                coins_view_cache.SetBestBlock(ConsumeUInt256(fuzzed_data_provider));
            },
            [&] {
                Coin move_to;
                (void)coins_view_cache.SpendCoin(random_out_point, fuzzed_data_provider.ConsumeBool() ? &move_to : nullptr);
            },
            [&] {
                coins_view_cache.Uncache(random_out_point);
            },
            [&] {
                if (fuzzed_data_provider.ConsumeBool()) {
                    backend_coins_view = CCoinsView{};
                }
                coins_view_cache.SetBackend(backend_coins_view);
            },
            [&] {
                const std::optional<COutPoint> opt_out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!opt_out_point) {
                    return;
                }
                random_out_point = *opt_out_point;
            },
            [&] {
                const std::optional<Coin> opt_coin = ConsumeDeserializable<Coin>(fuzzed_data_provider);
                if (!opt_coin) {
                    return;
                }
                random_coin = *opt_coin;
            },
            [&] {
                const std::optional<CMutableTransaction> opt_mutable_transaction = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
                if (!opt_mutable_transaction) {
                    return;
                }
                random_mutable_transaction = *opt_mutable_transaction;
            },
            [&] {
                CCoinsMap coins_map;
                while (fuzzed_data_provider.ConsumeBool()) {
                    CCoinsCacheEntry coins_cache_entry;
                    coins_cache_entry.flags = fuzzed_data_provider.ConsumeIntegral<unsigned char>();
                    if (fuzzed_data_provider.ConsumeBool()) {
                        coins_cache_entry.coin = random_coin;
                    } else {
                        const std::optional<Coin> opt_coin = ConsumeDeserializable<Coin>(fuzzed_data_provider);
                        if (!opt_coin) {
                            return;
                        }
                        coins_cache_entry.coin = *opt_coin;
                    }
                    coins_map.emplace(random_out_point, std::move(coins_cache_entry));
                }
                bool expected_code_path = false;
                try {
                    coins_view_cache.BatchWrite(coins_map, fuzzed_data_provider.ConsumeBool() ? ConsumeUInt256(fuzzed_data_provider) : coins_view_cache.GetBestBlock());
                    expected_code_path = true;
                } catch (const std::logic_error& e) {
                    if (e.what() == std::string{"FRESH flag misapplied to coin that exists in parent cache"}) {
                        expected_code_path = true;
                    }
                }
                assert(expected_code_path);
            });
    }

    {
        const Coin& coin_using_access_coin = coins_view_cache.AccessCoin(random_out_point);
        const bool exists_using_access_coin = !(coin_using_access_coin == EMPTY_COIN);
        const bool exists_using_have_coin = coins_view_cache.HaveCoin(random_out_point);
        const bool exists_using_have_coin_in_cache = coins_view_cache.HaveCoinInCache(random_out_point);
        Coin coin_using_get_coin;
        const bool exists_using_get_coin = coins_view_cache.GetCoin(random_out_point, coin_using_get_coin);
        if (exists_using_get_coin) {
            assert(coin_using_get_coin == coin_using_access_coin);
        }
        assert((exists_using_access_coin && exists_using_have_coin_in_cache && exists_using_have_coin && exists_using_get_coin) ||
               (!exists_using_access_coin && !exists_using_have_coin_in_cache && !exists_using_have_coin && !exists_using_get_coin));
        const bool exists_using_have_coin_in_backend = backend_coins_view.HaveCoin(random_out_point);
        if (exists_using_have_coin_in_backend) {
            assert(exists_using_have_coin);
        }
        Coin coin_using_backend_get_coin;
        if (backend_coins_view.GetCoin(random_out_point, coin_using_backend_get_coin)) {
            assert(exists_using_have_coin_in_backend);
            assert(coin_using_get_coin == coin_using_backend_get_coin);
        } else {
            assert(!exists_using_have_coin_in_backend);
        }
    }

    {
        bool expected_code_path = false;
        try {
            (void)coins_view_cache.Cursor();
        } catch (const std::logic_error&) {
            expected_code_path = true;
        }
        assert(expected_code_path);
        (void)coins_view_cache.DynamicMemoryUsage();
        (void)coins_view_cache.EstimateSize();
        (void)coins_view_cache.GetBestBlock();
        (void)coins_view_cache.GetCacheSize();
        (void)coins_view_cache.GetHeadBlocks();
        (void)coins_view_cache.HaveInputs(CTransaction{random_mutable_transaction});
    }

    {
        std::unique_ptr<CCoinsViewCursor> coins_view_cursor = backend_coins_view.Cursor();
        assert(!coins_view_cursor);
        (void)backend_coins_view.EstimateSize();
        (void)backend_coins_view.GetBestBlock();
        (void)backend_coins_view.GetHeadBlocks();
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const CTransaction transaction{random_mutable_transaction};
                bool is_spent = false;
                for (const CTxOut& tx_out : transaction.vout) {
                    if (Coin{tx_out, 0, transaction.IsCoinBase()}.IsSpent()) {
                        is_spent = true;
                    }
                }
                if (is_spent) {
                    // Avoid:
                    // coins.cpp:69: void CCoinsViewCache::AddCoin(const COutPoint &, Coin &&, bool): Assertion `!coin.IsSpent()' failed.
                    return;
                }
                bool expected_code_path = false;
                const int height = fuzzed_data_provider.ConsumeIntegral<int>();
                const bool possible_overwrite = fuzzed_data_provider.ConsumeBool();
                try {
                    AddCoins(coins_view_cache, transaction, height, possible_overwrite);
                    expected_code_path = true;
                } catch (const std::logic_error& e) {
                    if (e.what() == std::string{"Attempted to overwrite an unspent coin (when possible_overwrite is false)"}) {
                        assert(!possible_overwrite);
                        expected_code_path = true;
                    }
                }
                assert(expected_code_path);
            },
            [&] {
                (void)AreInputsStandard(CTransaction{random_mutable_transaction}, coins_view_cache, false);
                (void)AreInputsStandard(CTransaction{random_mutable_transaction}, coins_view_cache, true);
            },
            [&] {
                TxValidationState state;
                CAmount tx_fee_out;
                const CTransaction transaction{random_mutable_transaction};
                if (ContainsSpentInput(transaction, coins_view_cache)) {
                    // Avoid:
                    // consensus/tx_verify.cpp:171: bool Consensus::CheckTxInputs(const CTransaction &, TxValidationState &, const CCoinsViewCache &, int, CAmount &): Assertion `!coin.IsSpent()' failed.
                    return;
                }
                TxValidationState dummy;
                if (!CheckTransaction(transaction, dummy)) {
                    // It is not allowed to call CheckTxInputs if CheckTransaction failed
                    return;
                }
                if (Consensus::CheckTxInputs(transaction, state, coins_view_cache, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, std::numeric_limits<int>::max()), tx_fee_out)) {
                    assert(MoneyRange(tx_fee_out));
                }
            },
            [&] {
                const CTransaction transaction{random_mutable_transaction};
                if (ContainsSpentInput(transaction, coins_view_cache)) {
                    // Avoid:
                    // consensus/tx_verify.cpp:130: unsigned int GetP2SHSigOpCount(const CTransaction &, const CCoinsViewCache &): Assertion `!coin.IsSpent()' failed.
                    return;
                }
                (void)GetP2SHSigOpCount(transaction, coins_view_cache);
            },
            [&] {
                const CTransaction transaction{random_mutable_transaction};
                if (ContainsSpentInput(transaction, coins_view_cache)) {
                    // Avoid:
                    // consensus/tx_verify.cpp:130: unsigned int GetP2SHSigOpCount(const CTransaction &, const CCoinsViewCache &): Assertion `!coin.IsSpent()' failed.
                    return;
                }
                const auto flags{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                if (!transaction.vin.empty() && (flags & SCRIPT_VERIFY_WITNESS) != 0 && (flags & SCRIPT_VERIFY_P2SH) == 0) {
                    // Avoid:
                    // script/interpreter.cpp:1705: size_t CountWitnessSigOps(const CScript &, const CScript &, const CScriptWitness *, unsigned int): Assertion `(flags & SCRIPT_VERIFY_P2SH) != 0' failed.
                    return;
                }
                (void)GetTransactionSigOpCost(transaction, coins_view_cache, flags);
            },
            [&] {
                CCoinsStats stats{CoinStatsHashType::HASH_SERIALIZED};
                bool expected_code_path = false;
                try {
                    (void)GetUTXOStats(&coins_view_cache, WITH_LOCK(::cs_main, return std::ref(g_setup->m_node.chainman->m_blockman)), stats);
                } catch (const std::logic_error&) {
                    expected_code_path = true;
                }
                assert(expected_code_path);
            },
            [&] {
                (void)IsWitnessStandard(CTransaction{random_mutable_transaction}, coins_view_cache);
            });
    }
}
