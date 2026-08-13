// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COINS_H
#define BITCOIN_TEST_UTIL_COINS_H

#include <coins.h>
#include <crypto/hex_base.h>
#include <primitives/transaction.h>
#include <tinyformat.h>

#include <ostream>

class CCoinsViewCache;
class FastRandomContext;

/**
 * Create a Coin with DynamicMemoryUsage of 80 bytes and add it to the given view.
 * @param[in,out] coins_view  The coins view cache to add the new coin to.
 * @returns the COutPoint of the created coin.
 */
COutPoint AddTestCoin(FastRandomContext& rng, CCoinsViewCache& coins_view);

//! Strict equality, including fields of spent coins
inline bool operator==(const Coin& a, const Coin& b)
{
    return a.fCoinBase == b.fCoinBase && a.nHeight == b.nHeight && a.out == b.out;
}

//! Printed when a coin comparison fails
inline std::ostream& operator<<(std::ostream& os, const Coin& coin)
{
    return os << strprintf("Coin(spent=%d, coinbase=%d, height=%d, value=%d, scriptPubKey=%s)",
                           coin.IsSpent(), coin.fCoinBase, coin.nHeight, coin.out.nValue, HexStr(coin.out.scriptPubKey));
}

#endif // BITCOIN_TEST_UTIL_COINS_H
