// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COINS_H
#define BITCOIN_TEST_UTIL_COINS_H

#include <primitives/transaction.h>

class CCoinsViewCache;
class FastRandomContext;
class Coin;

/**
 * Create a Coin with DynamicMemoryUsage of 80 bytes and add it to the given view.
 * @param[in,out] coins_view  The coins view cache to add the new coin to.
 * @returns the COutPoint of the created coin.
 */
COutPoint AddTestCoin(FastRandomContext& rng, CCoinsViewCache& coins_view);

/**
 * Test adding a coin to a CCoinsViewCache.
 * 
 * @param coins_view The coins view cache to add the coin to
 * @param outpoint The outpoint of the coin to add
 * @param amount The value of the coin
 * @param height The block height when the coin was created
 * @param is_coinbase Whether the coin is from a coinbase transaction
 * @param possible_overwrite Whether overwriting an existing coin is allowed
 * @return true if the test succeeded (coin was added correctly or exception occurred appropriately)
 */
bool TestAddCoin(CCoinsViewCache& coins_view, const COutPoint& outpoint, CAmount amount, int height, bool is_coinbase, bool possible_overwrite);

/**
 * Test spending a coin from a CCoinsViewCache.
 * 
 * @param coins_view The coins view cache to spend the coin from
 * @param outpoint The outpoint of the coin to spend
 * @param moved_coin Optional pointer to store the spent coin in (can be nullptr)
 * @return true if the test succeeded (coin was spent correctly or not found as expected)
 */
bool TestSpendCoin(CCoinsViewCache& coins_view, const COutPoint& outpoint, Coin* moved_coin = nullptr);

#endif // BITCOIN_TEST_UTIL_COINS_H
