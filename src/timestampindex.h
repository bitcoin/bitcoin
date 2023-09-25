// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TIMESTAMPINDEX_H
#define BITCOIN_TIMESTAMPINDEX_H

#include <serialize.h>
#include <uint256.h>

struct CTimestampIndexIteratorKey {
public:
    uint32_t m_time;

public:
    CTimestampIndexIteratorKey() {
        SetNull();
    }

    CTimestampIndexIteratorKey(uint32_t time) : m_time{time} {};

    void SetNull() {
        m_time = 0;
    }

public:
    size_t GetSerializeSize(int nType, int nVersion) const {
        return 4;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata32be(s, m_time);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        m_time = ser_readdata32be(s);
    }
};

struct CTimestampIndexKey {
public:
    uint32_t m_block_time;
    uint256 m_block_hash;

public:
    CTimestampIndexKey() {
        SetNull();
    }

    CTimestampIndexKey(uint32_t block_time, uint256 block_hash) :
        m_block_time{block_time}, m_block_hash{block_hash} {};

    void SetNull() {
        m_block_time = 0;
        m_block_hash.SetNull();
    }

public:
    size_t GetSerializeSize(int nType, int nVersion) const {
        return 36;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata32be(s, m_block_time);
        m_block_hash.Serialize(s);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        m_block_time = ser_readdata32be(s);
        m_block_hash.Unserialize(s);
    }
};

#endif // BITCOIN_TIMESTAMPINDEX_H
