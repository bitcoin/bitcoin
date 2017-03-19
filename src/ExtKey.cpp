// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ExtKey.h"
#include "crypto/hmac_sha512.h"

bool CExtKey::Derive(CExtKey &out, unsigned int _nChild) const {
    out.m_nDepth = m_nDepth + 1;
    CKeyID id = m_key.GetPubKey().GetID();
    memcpy(&out.m_vchFingerprint[0], &id, 4);
    out.m_nChild = _nChild;
    return m_key.Derive(out.m_key, out.m_chaincode, _nChild, m_chaincode);
}

void CExtKey::SetMaster(const unsigned char *seed, unsigned int nSeedLen) {
    static const unsigned char hashkey[] = { 'B','i','t','c','o','i','n',' ','s','e','e','d' };
    std::vector<unsigned char, secure_allocator<unsigned char>> vout(64);
    CHMAC_SHA512(hashkey, sizeof(hashkey)).Write(seed, nSeedLen).Finalize(vout.data());
    m_key.SetBinary(&vout[0], &vout[32], true);
    memcpy(m_chaincode.begin(), &vout[32], 32);
    m_nDepth = 0;
    m_nChild = 0;
    memset(m_vchFingerprint, 0, sizeof(m_vchFingerprint));
}

CExtPubKey CExtKey::Neuter() const {
    CExtPubKey ret;
    ret.nDepth = m_nDepth;
    memcpy(&ret.vchFingerprint[0], &m_vchFingerprint[0], 4);
    ret.nChild = m_nChild;
    ret.pubkey = m_key.GetPubKey();
    ret.chaincode = m_chaincode;
    return ret;
}

void CExtKey::Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const {
    code[0] = m_nDepth;
    memcpy(code + 1, m_vchFingerprint, 4);
    code[5] = (m_nChild >> 24) & 0xFF; code[6] = (m_nChild >> 16) & 0xFF;
    code[7] = (m_nChild >> 8) & 0xFF; code[8] = (m_nChild >> 0) & 0xFF;
    memcpy(code + 9, m_chaincode.begin(), 32);
    code[41] = 0;
    assert(m_key.size() == 32);
    memcpy(code + 42, m_key.begin(), 32);
}

void CExtKey::Decode(const unsigned char code[BIP32_EXTKEY_SIZE]) {
    m_nDepth = code[0];
    memcpy(m_vchFingerprint, code + 1, 4);
    m_nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(m_chaincode.begin(), code + 9, 32);
    m_key.SetBinary(code + 42, code + BIP32_EXTKEY_SIZE, true);
}

#include "BitcoinExtKeyBase.h"
// CChainParams::EXT_SECRET_KEY
base58string CExtKey::GetBase58stringWithNetworkExtSecretKeyPrefix() const
{
    CBitcoinExtKey b58key;
    b58key.SetKey(*this);
    return base58string(b58key._ToString());
}
