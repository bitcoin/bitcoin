// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSINDEX_H
#define BITCOIN_ADDRESSINDEX_H

#include <uint256.h>
#include <amount.h>

#include <chrono>
#include <tuple>

namespace AddressType {
enum AddressType {
    P2PK = 1,
    P2PKH = 1,
    P2SH = 2,

    UNKNOWN = 0
};
}; /* namespace AddressType */

struct CMempoolAddressDelta
{
    std::chrono::seconds time;
    CAmount amount;
    uint256 prevhash;
    unsigned int prevout;

    CMempoolAddressDelta(std::chrono::seconds t, CAmount a, uint256 hash, unsigned int out) {
        time = t;
        amount = a;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddressDelta(std::chrono::seconds t, CAmount a) {
        time = t;
        amount = a;
        prevhash.SetNull();
        prevout = 0;
    }
};

struct CMempoolAddressDeltaKey
{
    int type;
    uint160 addressBytes;
    uint256 txhash;
    unsigned int index;
    bool is_spent;

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash, uint256 hash, unsigned int i, bool s) {
        type = addressType;
        addressBytes = addressHash;
        txhash = hash;
        index = i;
        is_spent = s;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash) {
        type = addressType;
        addressBytes = addressHash;
        txhash.SetNull();
        index = 0;
        is_spent = false;
    }
};

struct CMempoolAddressDeltaKeyCompare
{
    bool operator()(const CMempoolAddressDeltaKey& a, const CMempoolAddressDeltaKey& b) const {
        auto to_tuple = [](const CMempoolAddressDeltaKey& obj) {
            return std::tie(obj.type, obj.addressBytes, obj.txhash, obj.index, obj.is_spent);
        };
        return to_tuple(a) < to_tuple(b);
    }
};

#endif // BITCOIN_ADDRESSINDEX_H
