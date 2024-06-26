// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_INDEX_UTIL_H
#define BITCOIN_RPC_INDEX_UTIL_H

#include <cstdint>
#include <vector>

#include <amount.h>
#include <addressindex.h>
#include <spentindex.h>

class CTxMemPool;
class uint160;
class uint256;

enum class AddressType : uint8_t;

bool GetAddressIndex(const uint160& addressHash, const AddressType type,
                     std::vector<CAddressIndexEntry>& addressIndex,
                     const int32_t start = 0, const int32_t end = 0);
bool GetAddressUnspentIndex(const uint160& addressHash, const AddressType type,
                            std::vector<CAddressUnspentIndexEntry>& unspentOutputs);
bool GetMempoolAddressDeltaIndex(const CTxMemPool& mempool,
                                 const std::vector<CMempoolAddressDeltaKey>& addressDeltaIndex,
                                 std::vector<CMempoolAddressDeltaEntry>& addressDeltaEntries);
bool GetSpentIndex(const CTxMemPool& mempool, const CSpentIndexKey& key, CSpentIndexValue& value);
bool GetTimestampIndex(const uint32_t high, const uint32_t low, std::vector<uint256>& hashes);

#endif // BITCOIN_RPC_CLIENT_H
