// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ExtPubKey.h"
#include "PubKeyUtil.h"
#include <secp256k1.h>
#include <secp256k1_recovery.h>

/* Global secp256k1_context object used for verification. (defined in pubkey.cpp) */
extern secp256k1_context* secp256k1_context_verify;

void CExtPubKey::Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const {
    code[0] = m_nDepth;
    memcpy(code + 1, m_vchFingerprint, 4);
    code[5] = (m_nChild >> 24) & 0xFF; code[6] = (m_nChild >> 16) & 0xFF;
    code[7] = (m_nChild >> 8) & 0xFF; code[8] = (m_nChild >> 0) & 0xFF;
    memcpy(code + 9, m_chaincode.begin(), 32);
    assert(m_pubkey.size() == 33);
    memcpy(code + 41, m_pubkey.begin(), 33);
}

void CExtPubKey::Decode(const unsigned char code[BIP32_EXTKEY_SIZE]) {
    m_nDepth = code[0];
    memcpy(m_vchFingerprint, code + 1, 4);
    m_nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(m_chaincode.begin(), code + 9, 32);
    m_pubkey.Set(code + 41, code + BIP32_EXTKEY_SIZE);
}

bool CExtPubKey::Derive(CExtPubKey &out, unsigned int _nChild) const {
    out.m_nDepth = m_nDepth + 1;
    CKeyID id = m_pubkey.GetID();
    memcpy(&out.m_vchFingerprint[0], &id, 4);
    out.m_nChild = _nChild;
    return m_pubkey.Derive(out.m_pubkey, out.m_chaincode, _nChild, m_chaincode);
}

/* static */ bool CPubKey::CheckLowS(const std::vector<unsigned char>& vchSig) {
    secp256k1_ecdsa_signature sig;
    if (!CPubKeyUtil::ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig, &vchSig[0], vchSig.size())) {
        return false;
    }
    return (!secp256k1_ecdsa_signature_normalize(secp256k1_context_verify, NULL, &sig));
}

#include "BitcoinExtKeyBase.h"

// CChainParams::EXT_PUBLIC_KEY
base58string CExtPubKey::GetBase58stringWithNetworkExtPublicKeyPrefix() const
{
    return base58string(CBitcoinExtPubKey(*this)._ToString());
}
