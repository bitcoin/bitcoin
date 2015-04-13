// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/derivingkeystore.h"

namespace CoreWallet {
    
bool CDerivingKeyStore::AddMeta(const CKeyID &key, const CKeyID &keyParent, uint32_t nIndex, const ChainCode &chaincode, int nDepth)
{
    LOCK(cs_KeyStore);
    if (!keyParent.IsNull() || !chaincode.IsNull()) {
        // This is a deterministic key.
        mapDerivation[key].first = keyParent;
        mapDerivation[key].second = nIndex;
    }
    if (!chaincode.IsNull()) {
        mapChainCode[key].first = chaincode;
        mapChainCode[key].second = nDepth;
    }
    return true;
}

bool CDerivingKeyStore::GetExtKey(const CKeyID &keyid, CExtKey& key) const
{
    CKey keyJust;
    ChainCodeMap::const_iterator itCC = mapChainCode.find(keyid);
    if (itCC != mapChainCode.end() && CBasicKeyStore::GetKey(keyid, keyJust)) {
        key.key = keyJust;
        key.chaincode = itCC->second.first;
        key.nDepth = itCC->second.second;
        DerivationMap::const_iterator itDer = mapDerivation.find(keyid);
        memcpy(key.vchFingerprint, &itDer->second.first, 4);
        key.nChild = itDer->second.second;
        return true;
    }
    
    CExtKey keyParent;
    DerivationMap::const_iterator itDer = mapDerivation.find(keyid);
    if (itDer != mapDerivation.end() && GetExtKey(itDer->second.first, keyParent)) {
        return keyParent.Derive(key, itDer->second.second);
    }
    
    return false;
}

bool CDerivingKeyStore::GetExtPubKey(const CKeyID &keyid, CExtPubKey& key) const
{
    CPubKey keyJust;
    ChainCodeMap::const_iterator itCC = mapChainCode.find(keyid);
    if (itCC != mapChainCode.end() && CBasicKeyStore::GetPubKey(keyid, keyJust)) {
        key.pubkey = keyJust;
        key.chaincode = itCC->second.first;
        key.nDepth = itCC->second.second;
        DerivationMap::const_iterator itDer = mapDerivation.find(keyid);
        memcpy(key.vchFingerprint, &itDer->second.first, 4);
        key.nChild = itDer->second.second;
        return true;
    }
    
    CExtPubKey keyParent;
    DerivationMap::const_iterator itDer = mapDerivation.find(keyid);
    if (itDer != mapDerivation.end() && GetExtPubKey(itDer->second.first, keyParent)) {
        return keyParent.Derive(key, itDer->second.second);
    }
    
    return false;
}

bool CDerivingKeyStore::HaveKey(const CKeyID &key) const
{
    return (CBasicKeyStore::HaveKey(key) || mapDerivation.count(key) != 0);
}

bool CDerivingKeyStore::GetKey(const CKeyID &key, CKey &keyOut) const
{
    if (CBasicKeyStore::GetKey(key, keyOut))
        return true;
    CExtKey extkey;
    if (GetExtKey(key, extkey)) {
        keyOut = extkey.key;
        return true;
    }
    return false;
}

bool CDerivingKeyStore::GetPubKey(const CKeyID &key, CPubKey &keyOut) const
{
    if (CBasicKeyStore::GetPubKey(key, keyOut))
        return true;
    CExtPubKey extkey;
    if (GetExtPubKey(key, extkey)) {
        keyOut = extkey.pubkey;
        return true;
    }
    return false;
}

void CDerivingKeyStore::GetKeys(std::set<CKeyID> &setKeys) const
{
    CBasicKeyStore::GetKeys(setKeys);
    for (DerivationMap::const_iterator it = mapDerivation.begin(); it != mapDerivation.end(); it++)
        setKeys.insert(it->first);
}
}; //end namespace