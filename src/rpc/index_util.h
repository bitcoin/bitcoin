// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_INDEX_UTIL_H
#define BITCOIN_RPC_INDEX_UTIL_H

#include <cstdint>
#include <vector>

#include <amount.h>

class CTxMemPool;
class uint160;
class uint256;
struct CAddressIndexKey;
struct CAddressUnspentKey;
struct CAddressUnspentValue;
struct CSpentIndexKey;
struct CSpentIndexValue;

enum class AddressType : uint8_t;

bool GetAddressIndex(uint160 addressHash, AddressType type,
                     std::vector<std::pair<CAddressIndexKey, CAmount>>& addressIndex,
                     int32_t start = 0, int32_t end = 0);
bool GetAddressUnspentIndex(uint160 addressHash, AddressType type,
                            std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue>>& unspentOutputs);
bool GetSpentIndex(const CTxMemPool& mempool, CSpentIndexKey& key, CSpentIndexValue& value);
bool GetTimestampIndex(const uint32_t high, const uint32_t low, std::vector<uint256>& hashes);

#endif // BITCOIN_RPC_CLIENT_H
