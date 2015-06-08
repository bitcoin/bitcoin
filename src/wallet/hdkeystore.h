// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_HDKEYSTORE_H
#define BITCOIN_WALLET_HDKEYSTORE_H

#include "keystore.h"
#include "wallet/crypter.h"
#include "serialize.h"
#include "pubkey.h"

typedef uint256 HDChainID;

class CHDChain
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    HDChainID chainHash; //hash() of the masterpubkey
    std::string chainPath; //something like "m'/44'/0'/0'/c"
    CExtPubKey externalPubKey;
    CExtPubKey internalPubKey;

    CHDChain()
    {
        SetNull();
    }
    CHDChain(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;

        READWRITE(nCreateTime);
        READWRITE(chainHash);
        READWRITE(chainPath);
        READWRITE(externalPubKey);
        READWRITE(internalPubKey);
    }

    void SetNull()
    {
        nVersion = CHDChain::CURRENT_VERSION;
        nCreateTime = 0;
        chainHash.SetNull();
    }
};

class CHDKeyStore : public CCryptoKeyStore
{
protected:
    std::map<HDChainID, std::vector<unsigned char> > mapHDCryptedMasterSeeds;
    std::map<HDChainID, CKeyingMaterial > mapHDMasterSeeds;

public:
    virtual bool AddMasterSeed(const HDChainID& pubkeyhash, const CKeyingMaterial& masterSeed);
    virtual bool GetMasterSeed(const HDChainID& hash, CKeyingMaterial& seedOut) const;
};
#endif // BITCOIN_WALLET_HDKEYSTORE_H