// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLET_DERIVINGKEYSTORE_H
#define BITCOIN_COREWALLET_DERIVINGKEYSTORE_H

#include "keystore.h"

namespace CoreWallet {
typedef std::map<CKeyID, std::pair<ChainCode, int> > ChainCodeMap;
typedef std::map<CKeyID, std::pair<CKeyID, uint32_t> > DerivationMap;
    
class CDerivingKeyStore : public CBasicKeyStore {
protected:
    DerivationMap mapDerivation;
    ChainCodeMap mapChainCode;

public:
    virtual bool GetExtKey(const CKeyID &keyid, CExtKey& key) const;
    virtual bool GetExtPubKey(const CKeyID &keyid, CExtPubKey& key) const;

    virtual bool AddMeta(const CKeyID &key, const CKeyID &keyParent, uint32_t nIndex, const ChainCode &chaincode = ChainCode(), int nDepth = 0);

    virtual bool HaveKey(const CKeyID &key) const;
    virtual bool GetKey(const CKeyID &key, CKey &keyOut) const;
    virtual bool GetPubKey(const CKeyID &key, CPubKey &keyOut) const;
    virtual void GetKeys(std::set<CKeyID> &setKeys) const;
};
}
#endif // BITCOIN_COREWALLET_DERIVINGKEYSTORE_H