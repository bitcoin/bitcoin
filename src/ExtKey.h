// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "key.h"
class base58string;

struct CExtKey {
    unsigned char m_nDepth;
    unsigned char m_vchFingerprint[4];
    unsigned int m_nChild;
    ChainCode m_chaincode;
    CKey m_key;

    friend bool operator==(const CExtKey& a, const CExtKey& b)
    {
        return a.m_nDepth == b.m_nDepth &&
            memcmp(&a.m_vchFingerprint[0], &b.m_vchFingerprint[0], sizeof(m_vchFingerprint)) == 0 &&
            a.m_nChild == b.m_nChild &&
            a.m_chaincode == b.m_chaincode &&
            a.m_key == b.m_key;
    }

    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    bool Derive(CExtKey& out, unsigned int nChild) const;
    CExtPubKey Neuter() const;
    void SetMaster(const unsigned char* seed, unsigned int nSeedLen);
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        unsigned int len = BIP32_EXTKEY_SIZE;
        ::WriteCompactSize(s, len);
        unsigned char code[BIP32_EXTKEY_SIZE];
        Encode(code);
        s.write((const char *)&code[0], len);
    }
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        unsigned int len = (unsigned int)::ReadCompactSize(s);
        unsigned char code[BIP32_EXTKEY_SIZE];
        s.read((char *)&code[0], len);
        Decode(code);
    }

    base58string GetBase58stringWithNetworkExtSecretKeyPrefix() const;
};
