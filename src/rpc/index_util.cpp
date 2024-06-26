// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/index_util.h>

#include <txmempool.h>
#include <uint256.h>
#include <validation.h>

bool GetAddressIndex(uint160 addressHash, AddressType type,
                     std::vector<std::pair<CAddressIndexKey, CAmount>>& addressIndex, int32_t start, int32_t end)
{
    if (!fAddressIndex)
        return error("address index not enabled");

    if (!pblocktree->ReadAddressIndex(addressHash, type, addressIndex, start, end))
        return error("unable to get txids for address");

    return true;
}

bool GetAddressUnspentIndex(uint160 addressHash, AddressType type,
                            std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>& unspentOutputs)
{
    if (!fAddressIndex)
        return error("address index not enabled");

    if (!pblocktree->ReadAddressUnspentIndex(addressHash, type, unspentOutputs))
        return error("unable to get txids for address");

    return true;
}

bool GetSpentIndex(CTxMemPool& mempool, CSpentIndexKey& key, CSpentIndexValue& value)
{
    if (!fSpentIndex)
        return false;

    if (mempool.getSpentIndex(key, value))
        return true;

    if (!pblocktree->ReadSpentIndex(key, value))
        return false;

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
