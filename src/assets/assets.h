// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef RAVENCOIN_ASSET_PROTOCOL_H
#define RAVENCOIN_ASSET_PROTOCOL_H

#include <amount.h>

#include <string>
#include <set>
#include <map>
#include <tinyformat.h>
#include "serialize.h"
#include "primitives/transaction.h"
#include "assettypes.h"

#define RVN_R 114
#define RVN_V 118
#define RVN_N 110
#define RVN_Q 113
#define RVN_T 116
#define RVN_O 111

#define OWNER "!"
#define OWNER_LENGTH 1
#define MIN_ASSET_LENGTH 3
#define MAX_ASSET_LENGTH 31

class CScript;
class CDataStream;
class CTransaction;
class CTxOut;
class Coin;

class CAssets {
public:
    std::set<CNewAsset, AssetComparator> setAssets;
    std::map<std::string, std::set<COutPoint> > mapMyUnspentAssets; // Asset Name -> COutPoint
    std::map<std::string, std::set<std::string> > mapAssetsAddresses; // Asset Name -> set <Addresses>
    std::map<std::pair<std::string, std::string>, CAmount> mapAssetsAddressAmount; // pair < Asset Name , Address > -> Quantity of tokens in the address

    CAssets(const CAssets& assets) {
        this->mapMyUnspentAssets = assets.mapMyUnspentAssets;
        this->mapAssetsAddressAmount = assets.mapAssetsAddressAmount;
        this->mapAssetsAddresses = assets.mapAssetsAddresses;
        this->setAssets = assets.setAssets;
    }

    CAssets& operator=(const CAssets& other) {
        mapMyUnspentAssets = other.mapMyUnspentAssets;
        mapAssetsAddressAmount = other.mapAssetsAddressAmount;
        mapAssetsAddresses = other.mapAssetsAddresses;
        setAssets = other.setAssets;
        return *this;
    }

    CAssets() {
        SetNull();
    }

    void SetNull() {
        setAssets.clear();
        mapMyUnspentAssets.clear();
        mapAssetsAddresses.clear();
        mapAssetsAddressAmount.clear();
    }
};

class CAssetsCache : public CAssets
{
private:
    bool AddBackSpentAsset(const Coin& coin, const std::string& assetName, const std::string& address, const CAmount& nAmount, const COutPoint& out);
    void AddToAssetBalance(const std::string& strName, const std::string& address, const CAmount& nAmount);
    bool UndoTransfer(const CAssetTransfer& transfer, const std::string& address, const COutPoint& outToRemove);
public :
    //! These are memory only containers that show dirty entries that will be databased when flushed
    std::vector<CAssetCacheUndoAssetAmount> vUndoAssetAmount;
    std::vector<CAssetCacheSpendAsset> vSpentAssets;
    std::set<std::string> setChangeOwnedOutPoints;

    // New Assets Caches
    std::set<CAssetCacheNewAsset> setNewAssetsToRemove;
    std::set<CAssetCacheNewAsset> setNewAssetsToAdd;

    // Ownership Assets Caches
    std::set<CAssetCacheNewOwner> setNewOwnerAssetsToAdd;
    std::set<CAssetCacheNewOwner> setNewOwnerAssetsToRemove;

    // Transfer Assets Caches
    std::set<CAssetCacheNewTransfer> setNewTransferAssetsToAdd;
    std::set<CAssetCacheNewTransfer> setNewTransferAssetsToRemove;

    //! During init, the wallet isn't enabled, there for if we disconnect any blocks
    //! We need to store possible unspends of assets that could be ours and check back when the wallet is enabled.
    std::set<CAssetCachePossibleMine> setPossiblyMineAdd;
    std::set<CAssetCachePossibleMine> setPossiblyMineRemove;

    CAssetsCache()
    {
        SetNull();
    }

    CAssetsCache(const CAssetsCache& cache)
    {
        mapMyUnspentAssets = cache.mapMyUnspentAssets;
        mapAssetsAddressAmount = cache.mapAssetsAddressAmount;
        mapAssetsAddresses = cache.mapAssetsAddresses;
        setAssets = cache.setAssets;

        // Copy dirty cache also
        vSpentAssets = cache.vSpentAssets;
        vUndoAssetAmount = cache.vUndoAssetAmount;
        setChangeOwnedOutPoints = cache.setChangeOwnedOutPoints;

        // New Assets
        setNewAssetsToAdd = cache.setNewAssetsToAdd;
        setNewAssetsToRemove = cache.setNewAssetsToRemove;

        // New Transfers
        setNewTransferAssetsToAdd = cache.setNewTransferAssetsToAdd;
        setNewTransferAssetsToRemove = cache.setNewTransferAssetsToRemove;

        // New Owners
        setNewOwnerAssetsToAdd = cache.setNewOwnerAssetsToAdd;
        setNewOwnerAssetsToRemove = cache.setNewOwnerAssetsToRemove;

        // Copy sets of possibilymine
        setPossiblyMineAdd = cache.setPossiblyMineAdd;
        setPossiblyMineRemove = cache.setPossiblyMineRemove;
    }

    CAssetsCache& operator=(const CAssetsCache& cache)
    {
        mapMyUnspentAssets = cache.mapMyUnspentAssets;
        mapAssetsAddressAmount = cache.mapAssetsAddressAmount;
        mapAssetsAddresses = cache.mapAssetsAddresses;
        setAssets = cache.setAssets;

        // Copy dirty cache also
        vSpentAssets = cache.vSpentAssets;
        setNewTransferAssetsToAdd = cache.setNewTransferAssetsToAdd;
        vUndoAssetAmount = cache.vUndoAssetAmount;
        setNewAssetsToRemove = cache.setNewAssetsToRemove;
        setNewAssetsToAdd = cache.setNewAssetsToAdd;
        setNewTransferAssetsToRemove = cache.setNewTransferAssetsToRemove;
        setChangeOwnedOutPoints = cache.setChangeOwnedOutPoints;
        setNewOwnerAssetsToAdd = cache.setNewOwnerAssetsToAdd;
        setNewOwnerAssetsToRemove = cache.setNewOwnerAssetsToRemove;

        // Copy sets of possibilymine
        setPossiblyMineAdd = cache.setPossiblyMineAdd;
        setPossiblyMineRemove = cache.setPossiblyMineRemove;

        return *this;
    }

    // Cache only undo functions
    bool RemoveNewAsset(const CNewAsset& asset, const std::string address);
    bool RemoveTransfer(const CAssetTransfer& transfer, const std::string& address, const COutPoint& out);
    bool RemoveOwnerAsset(const std::string& assetsName, const std::string address);
    bool UndoAssetCoin(const Coin& coin, const COutPoint& out);

    // Cache only add asset functions
    bool AddNewAsset(const CNewAsset& asset, const std::string address);
    bool AddTransferAsset(const CAssetTransfer& transferAsset, const std::string& address, const COutPoint& out, const CTxOut& txOut);
    bool AddOwnerAsset(const std::string& assetsName, const std::string address);
    bool AddToMyUpspentOutPoints(const std::string& strName, const COutPoint& out);

    // Cache only validation functions
    bool TrySpendCoin(const COutPoint& out, const CTxOut& coin);

    // Help functions
    bool GetAssetsOutPoints(const std::string& strName, std::set<COutPoint>& outpoints);
    bool ContainsAsset(const CNewAsset& asset);
    bool AddPossibleOutPoint(const CAssetCachePossibleMine& possibleMine);

    //! Calculate the size of the CAssets (in bytes)
    size_t DynamicMemoryUsage() const;

    //! Get the size of the none databased cache
    size_t GetCacheSize() const;

    //! Flush a cache to a different cache (usually passets), save to database if fToDataBase is true
    bool Flush(bool fSoftCopy = false, bool fToDataBase = false);

    void Copy(const CAssetsCache& cache);

    void ClearDirtyCache() {

        vUndoAssetAmount.clear();
        vSpentAssets.clear();

        setNewAssetsToRemove.clear();
        setNewAssetsToAdd.clear();

        setNewTransferAssetsToAdd.clear();
        setNewTransferAssetsToRemove.clear();

        setNewOwnerAssetsToAdd.clear();
        setNewOwnerAssetsToRemove.clear();

        setChangeOwnedOutPoints.clear();

        // Copy sets of possibilymine
        setPossiblyMineAdd.clear();
        setPossiblyMineRemove.clear();
    }

   std::string CacheToString() const {

      return strprintf("vNewAssetsToRemove size : %d, vNewAssetsToAdd size : %d, vNewTransfer size : %d, vSpentAssets : %d, setChangeOwnedOutPoints size : %d\n",
                       setNewAssetsToRemove.size(), setNewAssetsToAdd.size(), setNewTransferAssetsToAdd.size(), vSpentAssets.size(), setChangeOwnedOutPoints.size());
    }
};

bool IsAssetNameValid(const std::string& name);

bool IsAssetNameSizeValid(const std::string& name);

bool IsAssetUnitsValid(const CAmount& units);

bool AssetFromTransaction(const CTransaction& tx, CNewAsset& asset, std::string& strAddress);
bool OwnerFromTransaction(const CTransaction& tx, std::string& ownerName, std::string& strAddress);

bool TransferAssetFromScript(const CScript& scriptPubKey, CAssetTransfer& assetTransfer, std::string& strAddress);
bool AssetFromScript(const CScript& scriptPubKey, CNewAsset& assetTransfer, std::string& strAddress);
bool OwnerAssetFromScript(const CScript& scriptPubKey, std::string& assetName, std::string& strAddress);

bool CheckIssueBurnTx(const CTxOut& txOut);
bool CheckIssueDataTx(const CTxOut& txOut);
bool CheckOwnerDataTx(const CTxOut& txOut);

bool IsScriptNewAsset(const CScript& scriptPubKey);
bool IsScriptOwnerAsset(const CScript& scriptPubKey);
bool IsScriptTransferAsset(const CScript& scriptPubKey);

bool IsNewOwnerTxValid(const CTransaction& tx, const std::string& assetName, const std::string& address, std::string& errorMsg);

void UpdatePossibleAssets();
#endif //RAVENCOIN_ASSET_PROTOCOL_H
