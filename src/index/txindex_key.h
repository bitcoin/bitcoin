// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_KEY_H
#define BITCOIN_INDEX_TXINDEX_KEY_H

#include <primitives/transaction_identifier.h>
#include <uint256.h>

#include <cstdint>
#include <string>
#include <utility>

namespace txindex {
/*
 * Database layout:
 *
 *   ["best_block_v2"]                        -> current sync locator
 *   ['t', txid]                              -> legacy CDiskTxPos
 *   ['B']                                    -> legacy sync locator
 */

inline const std::string DB_BEST_BLOCK_V2{"best_block_v2"};
//! Prefix of a legacy (pre-hashing) txindex row.
constexpr uint8_t DB_TXINDEX{'t'};

//! Key of a legacy (pre-hashing) txindex row: the full txid under the 't' prefix.
inline std::pair<uint8_t, uint256> LegacyTxKey(const Txid& txid)
{
    return {DB_TXINDEX, txid.ToUint256()};
}

} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
