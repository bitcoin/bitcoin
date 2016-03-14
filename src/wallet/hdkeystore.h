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

class CKeyMetadata
{
public:
    static const int CURRENT_VERSION=3;
    static const int VERSION_SUPPORT_FLAGS=2;
    static const int VERSION_SUPPORT_HD=3;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown
    HDChainID chainID;
    std::string keypath;

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = CKeyMetadata::VERSION_SUPPORT_HD;
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        if (nVersion >= VERSION_SUPPORT_HD)
        {
            READWRITE(keypath);
            if (keypath.size() > 0)
                READWRITE(chainID);
        }
    }

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
        keypath.clear();
        chainID.SetNull();
    }
};

/** class for representing a hd chain of keys. */
class CHDChain
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    bool usePubCKD;
    int64_t nCreateTime; // 0 means unknown

    HDChainID chainID; //hash of the masterpubkey
    std::string keypathTemplate; //example "m'/44'/0'/0'/c"

    CHDChain()
    {
        SetNull();
    }

    CHDChain(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    bool IsValid()
    {
        return (keypathTemplate.size() > 0);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;

        READWRITE(nCreateTime);
        READWRITE(chainID);
        READWRITE(keypathTemplate);
    }

    void SetNull()
    {
        nVersion = CHDChain::CURRENT_VERSION;
        nCreateTime = 0;
        chainID.SetNull();
        keypathTemplate.clear();
    }
};

class CHDKeyStore : public CCryptoKeyStore
{
protected:
    std::map<HDChainID, CKeyingMaterial > mapHDMasterSeeds; //master seeds are stored outside of the CHDChain object (for independent encryption)
    std::map<HDChainID, std::vector<unsigned char> > mapHDCryptedMasterSeeds;
    std::map<HDChainID, CHDChain> mapChains; //all available chains

    //!private key derivition of a ext priv key
    bool PrivateKeyDerivation(const std::string chainPath, const HDChainID& chainID, CExtKey& extKeyOut) const;

    //!derive key from a CHDPubKey object
    bool DeriveKey(const HDChainID chainID, const std::string keypath, CKey& keyOut) const;

public:
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;

    //!add a master seed with a given pubkeyhash (memory only)
    bool AddMasterSeed(const HDChainID& chainID, const CKeyingMaterial& masterSeed);

    //!add a crypted master seed with a given pubkeyhash (memory only)
    virtual bool AddCryptedMasterSeed(const HDChainID& chainID, const std::vector<unsigned char>& vchCryptedSecret);

    //!encrypt existing uncrypted seeds and remove unencrypted data
    virtual bool EncryptSeeds(CKeyingMaterial& vMasterKeyIn);

    //!export the master seed from a given chain id (hash of the master pub key)
    virtual bool GetMasterSeed(const HDChainID& chainID, CKeyingMaterial& seedOut) const;

    //!get the encrypted master seed of a giveb chain id
    virtual bool GetCryptedMasterSeed(const HDChainID& chainID, std::vector<unsigned char>& vchCryptedSecret) const;

    //!writes all available chain ids to a vector
    virtual void GetAvailableChainIDs(std::vector<HDChainID>& chainIDs);

    //!add a new chain to the keystore (memory only)
    bool AddHDChain(const CHDChain& chain);

    //!writes a chain defined by given chainId to chainOut, returns false if not found
    bool GetChain(const HDChainID chainID, CHDChain& chainOut) const;

    //!Derives a key at index in the given hd chain (defined by chainId) 
    bool DeriveKeyAtIndex(const HDChainID chainID, CKey& keyOut, std::string& keypathOut, unsigned int nIndex, bool internal) const;
    /**
     * Get next available index for a child key in chain defined by given chain id
     * @return next available index
     * @warning This will "fill gaps". If you have m/0/0, m/0/1, m/0/2, m/0/100 it will return 3 (m/0/3)
     */
    unsigned int GetNextChildIndex(const HDChainID& chainID, bool internal);
};
#endif // BITCOIN_WALLET_HDKEYSTORE_H
