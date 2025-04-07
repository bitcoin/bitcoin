// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/coins.h>

#include <coins.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/random.h>
#include <uint256.h>

#include <stdint.h>
#include <utility>

COutPoint AddTestCoin(FastRandomContext& rng, CCoinsViewCache& coins_view)
{
    Coin new_coin;
    COutPoint outpoint{Txid::FromUint256(rng.rand256()), /*nIn=*/0};
    new_coin.nHeight = 1;
    new_coin.out.nValue = RandMoney(rng);
    new_coin.out.scriptPubKey.assign(uint32_t{56}, 1);
    coins_view.AddCoin(outpoint, std::move(new_coin), /*possible_overwrite=*/false);

    return outpoint;
};

bool TestAddCoin(CCoinsViewCache& coins_view, const COutPoint& outpoint, CAmount amount, int height, bool is_coinbase, bool possible_overwrite)
{
    Coin new_coin;
    new_coin.out.nValue = amount;
    new_coin.nHeight = height;
    new_coin.fCoinBase = is_coinbase;
    new_coin.out.scriptPubKey.assign(uint32_t{40}, 1); // Simple script

    // Check the initial state
    bool had_coin_before = coins_view.HaveCoin(outpoint);

    try {
        coins_view.AddCoin(outpoint, std::move(new_coin), possible_overwrite);
        
        // If we had the coin before and overwrite was not allowed, this should have thrown an exception
        if (had_coin_before && !possible_overwrite && !coins_view.AccessCoin(outpoint).IsSpent()) {
            return false; // Should not reach here
        }
        
        // Verify the coin was added correctly
        const Coin& added_coin = coins_view.AccessCoin(outpoint);
        return !added_coin.IsSpent() && 
               added_coin.out.nValue == amount &&
               added_coin.nHeight == height &&
               added_coin.fCoinBase == is_coinbase;
    } catch (const std::logic_error&) {
        // Expected exception when trying to overwrite without permission
        return had_coin_before && !possible_overwrite && !coins_view.AccessCoin(outpoint).IsSpent();
    }
}

bool TestSpendCoin(CCoinsViewCache& coins_view, const COutPoint& outpoint, Coin* moved_coin)
{
    // Check initial state
    bool had_coin_before = coins_view.HaveCoin(outpoint);
    if (!had_coin_before) {
        // If we don't have the coin, SpendCoin should return false
        bool result = coins_view.SpendCoin(outpoint, moved_coin);
        return !result;
    }

    // Capture the coin's data before spending if needed for verification
    Coin original_coin;
    if (moved_coin) {
        original_coin = coins_view.AccessCoin(outpoint);
    }

    // Spend the coin
    bool result = coins_view.SpendCoin(outpoint, moved_coin);
    
    // Verify the coin is now spent
    const Coin& spent_coin = coins_view.AccessCoin(outpoint);
    
    // Check if moveout got the expected data
    bool moveout_correct = true;
    if (moved_coin && result) {
        moveout_correct = moved_coin->out.nValue == original_coin.out.nValue &&
                          moved_coin->nHeight == original_coin.nHeight &&
                          moved_coin->fCoinBase == original_coin.fCoinBase;
    }
    
    return result && spent_coin.IsSpent() && moveout_correct;
}
