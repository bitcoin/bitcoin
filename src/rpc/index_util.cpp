// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/index_util.h>

#include <txmempool.h>
#include <uint256.h>
#include <validation.h>

bool GetAddressIndex(const uint160& addressHash, const AddressType type,
                     std::vector<std::pair<CAddressIndexKey, CAmount>>& addressIndex,
                     const int32_t start, const int32_t end)
{
    if (!fAddressIndex)
        return error("Address index not enabled");

    if (!pblocktree->ReadAddressIndex(addressHash, type, addressIndex, start, end))
        return error("Unable to get txids for address");

    return true;
}

bool GetAddressUnspentIndex(const uint160& addressHash, const AddressType type,
                            std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>& unspentOutputs)
{
    if (!fAddressIndex)
        return error("Address index not enabled");

    if (!pblocktree->ReadAddressUnspentIndex(addressHash, type, unspentOutputs))
        return error("Unable to get txids for address");

    return true;
}

bool GetSpentIndex(const CTxMemPool& mempool, const CSpentIndexKey& key, CSpentIndexValue& value)
{
    if (!fSpentIndex)
        return error("Spent index not enabled");

    if (mempool.getSpentIndex(key, value))
        return true;

    if (!pblocktree->ReadSpentIndex(key, value))
        return error("Unable to get spend information");

    return true;
}

bool GetTimestampIndex(const uint32_t high, const uint32_t low, std::vector<uint256>& hashes)
{
    if (!fTimestampIndex)
        return error("Timestamp index not enabled");

    if (!pblocktree->ReadTimestampIndex(high, low, hashes))
        return error("Unable to get hashes for timestamps");

    return true;
}
