// Copyright (c) 2016 BitPay Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/index_util.h>

#include <node/blockstorage.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>

static void EnsureAddressIndexAvailable()
{
    if (!fAddressIndex) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Address index is disabled. You should run Dash Core with -addressindex (requires reindex)");
    }
}

bool GetAddressIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                     std::vector<CAddressIndexEntry>& addressIndex,
                     const int32_t start, const int32_t end)
{
    AssertLockHeld(::cs_main);
    EnsureAddressIndexAvailable();

    return block_tree_db.ReadAddressIndex(addressHash, type, addressIndex, start, end);
}

bool GetAddressUnspentIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                            std::vector<CAddressUnspentIndexEntry>& unspentOutputs, const bool height_sort)
{
    AssertLockHeld(::cs_main);
    EnsureAddressIndexAvailable();

    if (!block_tree_db.ReadAddressUnspentIndex(addressHash, type, unspentOutputs))
        return false;

    if (height_sort) {
        std::sort(unspentOutputs.begin(), unspentOutputs.end(),
                  [](const CAddressUnspentIndexEntry &a, const CAddressUnspentIndexEntry &b) {
                        return a.second.m_block_height < b.second.m_block_height;
                  });
    }

    return true;
}

bool GetMempoolAddressDeltaIndex(const CTxMemPool& mempool,
                                 const std::vector<CMempoolAddressDeltaKey>& addressDeltaIndex,
                                 std::vector<CMempoolAddressDeltaEntry>& addressDeltaEntries,
                                 const bool timestamp_sort)
{
    EnsureAddressIndexAvailable();

    if (!mempool.getAddressIndex(addressDeltaIndex, addressDeltaEntries))
        return false;

    if (timestamp_sort) {
        std::sort(addressDeltaEntries.begin(), addressDeltaEntries.end(),
                  [](const CMempoolAddressDeltaEntry &a, const CMempoolAddressDeltaEntry &b) {
                        return a.second.m_time < b.second.m_time;
                  });
    }

    return true;
}

bool GetSpentIndex(CBlockTreeDB& block_tree_db, const CTxMemPool& mempool, const CSpentIndexKey& key,
                   CSpentIndexValue& value)
{
    AssertLockHeld(::cs_main);

    if (!fSpentIndex) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Spent index is disabled. You should run Dash Core with -spentindex (requires reindex)");
    }

    if (mempool.getSpentIndex(key, value))
        return true;

    return block_tree_db.ReadSpentIndex(key, value);
}

bool GetTimestampIndex(CBlockTreeDB& block_tree_db, const uint32_t high, const uint32_t low,
                       std::vector<uint256>& hashes)
{
    AssertLockHeld(::cs_main);

    if (!fTimestampIndex) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "Timestamp index is disabled. You should run Dash Core with -timestampindex (requires reindex)");
    }

    return block_tree_db.ReadTimestampIndex(high, low, hashes);
}
