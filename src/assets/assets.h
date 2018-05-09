// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef RAVENCOIN_ASSET_PROTOCOL_H
#define RAVENCOIN_ASSET_PROTOCOL_H

#include <amount.h>

#include <string>
#include <set>
#include <map>
#include "serialize.h"

class CScript;
class CDataStream;
class CTransaction;
class COutPoint;
class CTxOut;
class Coin;

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

    bool IsValid(std::string& strError, bool fCheckMempool = false);

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

class AssetComparator
{
public:
    bool operator()(const CNewAsset& s1, const CNewAsset& s2) const
    {
        return s1.strName < s2.strName;
    }
};

class CAssets {
private:
    bool AddToAssetBalance(const std::string& strName, const std::string& address, const CAmount& nAmount);

public:
    std::set<CNewAsset, AssetComparator> setAssets;
    std::map<std::string, std::set<COutPoint> > mapMyUnspentAssets; // Asset Name -> COutPoint
    std::map<std::string, std::set<COutPoint> > mapMySpentAssets;
    std::map<std::string, std::set<std::string> > mapAssetsAddresses; // Asset Name -> set <Addresses>
    std::map<std::pair<std::string, std::string>, CAmount> mapAssetsAddressAmount; // pair < Asset Name , Address > -> Quantity of tokens in the address

    std::set<COutPoint> setUnspent;


    CAssets() {
        SetNull();
    }

    void SetNull() {
        setAssets.clear();
        mapMyUnspentAssets.clear();
        mapMySpentAssets.clear();
        mapAssetsAddresses.clear();
        mapAssetsAddressAmount.clear();
        setUnspent.clear();
    }

    bool AddNewAsset(const CNewAsset& asset, const std::string& address, const COutPoint& out);
    bool AddTransferAsset(const CAssetTransfer& transferAsset, const std::string& address, const COutPoint& out, const CTxOut& txOut);
    bool AddToMyUpspentOutPoints(const std::string& strName, const COutPoint& out);

    bool ContainsAsset(const CNewAsset& asset);
    bool RemoveAssetAndOutPoints(const CNewAsset& asset, const std::string& address);

    bool GetAssetsOutPoints(const std::string& strName, std::set<COutPoint>& outpoints);

    void TrySpendCoin(const COutPoint& out, const Coin& coin);
};

bool IsAssetNameValid(const std::string& name);

bool IsAssetNameSizeValid(const std::string& name);

bool IsAssetUnitsValid(const CAmount& units);

bool IssueNewAsset(const std::string& name, const CAmount& nAmount, CNewAsset& asset);

bool AssetFromTransaction(const CTransaction& tx, CNewAsset& asset, std::string& strAddress);

bool TransferAssetFromScript(const CScript& scriptPubKey, CAssetTransfer& assetTransfer, std::string& strAddress);

bool AssetFromScript(const CScript& scriptPubKey, CNewAsset& assetTransfer, std::string& strAddress);

bool CheckIssueBurnTx(const CTxOut& txOut);

bool CheckIssueDataTx(const CTxOut& txOut);

bool IsScriptNewAsset(const CScript& scriptPubKey);
bool IsScriptTransferAsset(const CScript& scriptPubKey);

#endif //RAVENCOIN_ASSET_PROTOCOL_H
