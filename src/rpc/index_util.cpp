// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/index_util.h>

#include <txmempool.h>
#include <uint256.h>
#include <validation.h>

bool GetAddressIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                     std::vector<CAddressIndexEntry>& addressIndex,
                     const int32_t start, const int32_t end)
{
    AssertLockHeld(::cs_main);

    if (!fAddressIndex)
        return error("Address index not enabled");

    if (!block_tree_db.ReadAddressIndex(addressHash, type, addressIndex, start, end))
        return error("Unable to get txids for address");

    return true;
}

bool GetAddressUnspentIndex(CBlockTreeDB& block_tree_db, const uint160& addressHash, const AddressType type,
                            std::vector<CAddressUnspentIndexEntry>& unspentOutputs, const bool height_sort)
{
    AssertLockHeld(::cs_main);

    if (!fAddressIndex)
        return error("Address index not enabled");

    if (!block_tree_db.ReadAddressUnspentIndex(addressHash, type, unspentOutputs))
        return error("Unable to get txids for address");

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
    if (!fAddressIndex)
        return error("Address index not enabled");

    if (!mempool.getAddressIndex(addressDeltaIndex, addressDeltaEntries))
        return error("Unable to get address delta information");

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

    if (!fSpentIndex)
        return error("Spent index not enabled");

    if (mempool.getSpentIndex(key, value))
        return true;

    if (!block_tree_db.ReadSpentIndex(key, value))
        return error("Unable to get spend information");

    return true;
}

bool GetTimestampIndex(CBlockTreeDB& block_tree_db, const uint32_t high, const uint32_t low,
                       std::vector<uint256>& hashes)
{
    AssertLockHeld(::cs_main);

    if (!fTimestampIndex)
        return error("Timestamp index not enabled");

    if (!block_tree_db.ReadTimestampIndex(high, low, hashes))
        return error("Unable to get hashes for timestamps");

    return true;
}
