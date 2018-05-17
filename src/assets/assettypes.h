//
// Created by Jeremy Anderson on 5/15/18.
//

#ifndef RAVENCOIN_NEWASSET_H
#define RAVENCOIN_NEWASSET_H

#include <amount.h>
#include <string>
#include "serialize.h"

class CAssetsCache;

class CNewAsset {
public:
    int8_t nNameLength;
    std::string strName;
    CAmount nAmount;
    int8_t units;
    int8_t nReissuable;
    int8_t nHasIPFS;
    std::string strIPFSHash;

    CNewAsset()
    {
        SetNull();
    }

    CNewAsset(const std::string& strName, const CAmount& nAmount, const int& nNameLength, const int& units, const int& nReissuable, const int& nHasIPFS, const std::string& strIPFSHash);

    void SetNull()
    {
        nNameLength = int8_t(1);
        strName= "";
        nAmount = 0;
        units = int8_t(1);
        nReissuable = int8_t(0);
        nHasIPFS = int8_t(0);
        strIPFSHash = "";
    }

    bool IsNull() const;

    bool IsValid(std::string& strError, CAssetsCache& assetCache, bool fCheckMempool = false, bool fCheckDuplicateInputs = true);

    std::string ToString();

    void ConstructTransaction(CScript& script) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nNameLength);
        READWRITE(strName);
        READWRITE(nAmount);
        READWRITE(units);
        READWRITE(nReissuable);
        READWRITE(nHasIPFS);
        READWRITE(strIPFSHash);
    }
};

class AssetComparator
{
public:
    bool operator()(const CNewAsset& s1, const CNewAsset& s2) const
    {
        return s1.strName < s2.strName;
    }
};

class CAssetTransfer {
public:
    std::string strName;
    CAmount nAmount;


    CAssetTransfer()
    {
        SetNull();
    }

    void SetNull()
    {
        nAmount = 0;
        strName = "";
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(strName);
        READWRITE(nAmount);
    }

    CAssetTransfer(const std::string& strAssetName, const CAmount& nAmount);
    void ConstructTransaction(CScript& script) const;
};



/** THESE ARE ONLY TO BE USED WHEN ADDING THINGS TO THE CACHE DURING CONNECT AND DISCONNECT BLOCK */
struct CAssetCacheUndoAssetTransfer
{
    CAssetTransfer transfer;
    std::string address;
    COutPoint out;

    CAssetCacheUndoAssetTransfer(const CAssetTransfer& transfer, const std::string& address, const COutPoint& out)
    {
        this->transfer = transfer;
        this->address = address;
        this->out = out;
    }
};

struct CAssetCacheUndoAssetAmount
{
    std::string assetName;
    std::string address;
    CAmount nAmount;

    CAssetCacheUndoAssetAmount(const std::string& assetName, const std::string& address, const CAmount& nAmount)
    {
        this->assetName = assetName;
        this->address = address;
        this->nAmount = nAmount;
    }
};

struct CAssetCacheNewTransfer
{
    std::string assetName;
    std::string address;

    CAssetCacheNewTransfer(const std::string& assetName, const std::string& address)
    {
        this->assetName = assetName;
        this->address = address;
    }
};

struct CAssetCacheSpendAsset
{
    std::string assetName;
    std::string address;

    CAssetCacheSpendAsset(const std::string& assetName, const std::string& address)
    {
        this->assetName = assetName;
        this->address = address;
    }
};

struct CAssetCachePossibleMine
{
    std::string assetName;
    COutPoint out;
    CTxOut txOut;

    CAssetCachePossibleMine(const std::string& assetName, const COutPoint& out, const CTxOut txOut)
    {
        this->assetName = assetName;
        this->out = out;
        this->txOut = txOut;
    }

    bool operator<(const CAssetCachePossibleMine &other) const
    {
        return out < other.out;
    }
};

#endif //RAVENCOIN_NEWASSET_H
