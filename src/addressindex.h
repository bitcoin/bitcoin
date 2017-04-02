// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSINDEX_H
#define BITCOIN_ADDRESSINDEX_H

#include "uint256.h"
#include "amount.h"
#include "script/script.h"

struct CAddressUnspentKey {
    unsigned int type;
    uint160 hashBytes;
    uint256 txhash;
    size_t index;

    size_t GetSerializeSize() const {
        return 57;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        txhash.Serialize(s);
        ser_writedata32(s, index);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
    }

    CAddressUnspentKey(unsigned int addressType, uint160 addressHash, uint256 txid, size_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    CAmount satoshis;
    CScript script;
    int blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(satoshis);
        READWRITE(*(CScriptBase*)(&script));
        READWRITE(blockHeight);
    }

    CAddressUnspentValue(CAmount sats, CScript scriptPubKey, int height) {
        satoshis = sats;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
        script.clear();
        blockHeight = 0;
    }

    bool IsNull() const {
        return (satoshis == -1);
    }
};

struct CAddressIndexKey {
    unsigned int type;
    uint160 hashBytes;
    int blockHeight;
    unsigned int txindex;
    uint256 txhash;
    size_t index;
    bool spending;

    size_t GetSerializeSize() const {
        return 66;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        // Heights are stored big-endian for key sorting in LevelDB
        ser_writedata32be(s, blockHeight);
        ser_writedata32be(s, txindex);
        txhash.Serialize(s);
        ser_writedata32(s, index);
        char f = spending;
        ser_writedata8(s, f);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        blockHeight = ser_readdata32be(s);
        txindex = ser_readdata32be(s);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
        char f = ser_readdata8(s);
        spending = f;
    }

    CAddressIndexKey(unsigned int addressType, uint160 addressHash, int height, int blockindex,
                     uint256 txid, size_t indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddressIndexKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
    }

};

struct CAddressIndexIteratorKey {
    unsigned int type;
    uint160 hashBytes;

    size_t GetSerializeSize() const {
        return 21;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
    }

    CAddressIndexIteratorKey(unsigned int addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
    }

    CAddressIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
    }
};

struct CAddressIndexIteratorHeightKey {
    unsigned int type;
    uint160 hashBytes;
    int blockHeight;

    size_t GetSerializeSize() const {
        return 25;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        ser_writedata32be(s, blockHeight);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        blockHeight = ser_readdata32be(s);
    }

    CAddressIndexIteratorHeightKey(unsigned int addressType, uint160 addressHash, int height) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        blockHeight = 0;
    }
};

struct CMempoolAddressDelta
{
    int64_t time;
    CAmount amount;
    uint256 prevhash;
    unsigned int prevout;

    CMempoolAddressDelta(int64_t t, CAmount a, uint256 hash, unsigned int out) {
        time = t;
        amount = a;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddressDelta(int64_t t, CAmount a) {
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
    int spending;

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash, uint256 hash, unsigned int i, int s) {
        type = addressType;
        addressBytes = addressHash;
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash) {
        type = addressType;
        addressBytes = addressHash;
        txhash.SetNull();
        index = 0;
        spending = 0;
    }
};

struct CMempoolAddressDeltaKeyCompare
{
    bool operator()(const CMempoolAddressDeltaKey& a, const CMempoolAddressDeltaKey& b) const {
        if (a.type == b.type) {
            if (a.addressBytes == b.addressBytes) {
                if (a.txhash == b.txhash) {
                    if (a.index == b.index) {
                        return a.spending < b.spending;
                    } else {
                        return a.index < b.index;
                    }
                } else {
                    return a.txhash < b.txhash;
                }
            } else {
                return a.addressBytes < b.addressBytes;
            }
        } else {
            return a.type < b.type;
        }
    }
};

#endif // BITCOIN_ADDRESSINDEX_H
