// Copyright (c) 2016 BitPay Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_INDEX_UTIL_H
#define BITCOIN_RPC_INDEX_UTIL_H

#include <consensus/amount.h>
#include <addressindex.h>
#include <spentindex.h>
#include <sync.h>
#include <threadsafety.h>

#include <cstdint>
#include <vector>

class CBlockTreeDB;
class CTxMemPool;
class uint160;
class uint256;

enum class AddressType : uint8_t;

extern RecursiveMutex cs_main;

//! throws JSONRPCError if address index is unavailable
bool GetAddressIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                     std::vector<CAddressIndexEntry>& addressIndex,
                     const int32_t start = 0, const int32_t end = 0)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
//! throws JSONRPCError if address index is unavailable
bool GetAddressUnspentIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                            std::vector<CAddressUnspentIndexEntry>& unspentOutputs, const bool height_sort = false)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
//! throws JSONRPCError if address index is unavailable
bool GetMempoolAddressDeltaIndex(const CTxMemPool& mempool,
                                 const std::vector<CMempoolAddressDeltaKey>& addressDeltaIndex,
                                 std::vector<CMempoolAddressDeltaEntry>& addressDeltaEntries,
                                 const bool timestamp_sort = false);

//! throws JSONRPCError if spent index is unavailable
bool GetSpentIndex(CBlockTreeDB& block_tree_db, const CTxMemPool& mempool, const CSpentIndexKey& key,
                   CSpentIndexValue& value)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! throws JSONRPCError if timestamp index is unavailable
bool GetTimestampIndex(CBlockTreeDB& block_tree_db, const uint32_t high, const uint32_t low,
                       std::vector<uint256>& hashes)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif // BITCOIN_RPC_INDEX_UTIL_H
