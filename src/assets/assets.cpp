// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <regex>
#include <script/script.h>
#include <version.h>
#include <streams.h>
#include <primitives/transaction.h>
#include <iostream>
#include <script/standard.h>
#include <util.h>
#include <chainparams.h>
#include <base58.h>
#include <validation.h>
#include <txmempool.h>
#include <tinyformat.h>
#include <wallet/wallet.h>
#include "assets.h"
#include "assetdb.h"

#define RVN_R 114
#define RVN_V 118
#define RVN_N 110
#define RVN_Q 113
#define RVN_T 116


static const std::regex ASSET_NAME_REGEX("[A-Za-z_]{3,}");

// Does static checking (length, characters, etc.)
bool IsAssetNameValid(const std::string& name)
{
    return std::regex_match(name, ASSET_NAME_REGEX);
}

bool IsAssetNameSizeValid(const std::string& name)
{
    return name.size() >= 3 && name.size() <= 31;
}

bool CNewAsset::IsNull() const
{
    return strName == "";
}

bool CAssets::GetAssetsOutPoints(const std::string& strName, std::set<COutPoint>& outpoints)
{
    if (mapMyUnspentAssets.count(strName)) {
        outpoints = mapMyUnspentAssets.at(strName);
        return true;
    }

    return false;
}

bool CNewAsset::IsValid(std::string& strError, bool fCheckMempool)
{
    strError = "";

    // Check our current passets to see if the asset has been created yet
    if (passets->setAssets.count(*this))
        strError = std::string("Invalid parameter: asset_name '") + strName + std::string("' has already been used");

    if (fCheckMempool) {
        for (const CTxMemPoolEntry &entry : mempool.mapTx) {
            CTransaction tx = entry.GetTx();
            if (tx.IsNewAsset()) {
                CNewAsset asset;
                std::string address;
                AssetFromTransaction(tx, asset, address);
                if (asset.strName == strName) {
                    strError = "Asset with this name is already in the mempool";
                    return false;
                }
            }
        }
    }

    if (!IsAssetNameValid(std::string(strName)))
        strError = "Invalid parameter: asset_name must be only consist of valid characters. See help for more details.";

    if (!IsAssetNameSizeValid(strName))
        strError  = "Invalid parameter: asset_name must have a size between 3 to 31";

    if (nAmount <= 0)
        strError  = "Invalid parameter: asset amount can't be equal to or less than zero.";

    if (nNameLength < 1 || nNameLength > 9)
        strError  = "Invalid parameter: name_length must be between 1-9";

    if (units < 0 || units > 8)
        strError  = "Invalid parameter: units must be between 0-8.";

    if (nReissuable != 0 && nReissuable != 1)
        strError  = "Invalid parameter: reissuable must be 0 or 1";

    if (nHasIPFS != 0 && nHasIPFS != 1)
        strError  = "Invalid parameter: has_ipfs must be 0 or 1.";

    if (nHasIPFS && strIPFSHash.size() != 40)
        strError  = "Invalid parameter: ipfs_hash must be 40 bytes.";

    return strError == "";
}

std::string CNewAsset::ToString()
{
    std::stringstream ss;
    ss << "Printing an asset" << "\n";
    ss << "name : " << strName << "\n";
    ss << "amount : " << nAmount << "\n";
    ss << "name_length : " << std::to_string(nNameLength) << "\n";
    ss << "units : " << std::to_string(units) << "\n";
    ss << "reissuable : " << std::to_string(nReissuable) << "\n";
    ss << "has_ipfs : " << std::to_string(nHasIPFS) << "\n";

    if (nHasIPFS)
        ss << "ipfs_hash : " << strIPFSHash;

    return ss.str();
}

// 1, 10, 100 ... COIN
// (0.00000001, 0.0000001, ... 1)
bool IsAssetUnitsValid(const CAmount& units)
{
    for (int i = 1; i <= COIN; i *= 10) {
        if (units == i) return true;
    }
    return false;
}

bool IssueNewAsset(const std::string& name, const CAmount& nAmount, CNewAsset& asset)
{
    asset.SetNull();

    asset.nNameLength = int8_t(name.size() / 8 + 1);
    asset.strName = name;
    asset.nAmount = nAmount;

    std::string strError = "";
    if (!asset.IsValid(strError))
        return error("%s : Asset not valid: %s", __func__, strError);

    return true;
}

bool CAssets::AddNewAsset(const CNewAsset& asset, const std::string& address, const COutPoint& out)
{
    LOCK(cs_main);

    if (!passets->setAssets.insert(asset).second)
        return error("%s: Tried adding new asset, but it already exsisted in the set of assets: %s", __func__, asset.strName);

    // Insert the asset into the assets address map
    if (passets->mapAssetsAddresses.count(asset.strName)) {
        if (passets->mapAssetsAddresses[asset.strName].count(address))
            return error("%s : Tried adding a new asset and saving its quantity, but it already exsisted in the map of assets addresses: %s", __func__, asset.strName);

        passets->mapAssetsAddresses[asset.strName].insert(address);
    } else {
        std::set<std::string> setAddresses;
        setAddresses.insert(address);
        passets->mapAssetsAddresses.insert(std::make_pair(asset.strName, setAddresses));
    }

    // Insert the asset into the assests address amount map
    passets->mapAssetsAddressAmount[std::make_pair(asset.strName, address)] = asset.nAmount;

    if (!passetsdb->WriteAssetData(asset))
        return error("%s: Failed adding new asset to database: %s", __func__, asset.strName);

    if (!passetsdb->WriteAssetAddressQuantity(asset.strName, address, asset.nAmount))
        return error("%s Failed adding new asset quantity database: %s", __func__, asset.strName);


    LogPrintf("%s: Added a new asset: %s at address: %s with amount: %d\n", __func__, asset.strName, address, asset.nAmount);

    return true;
}

bool CAssets::AddTransferAsset(const CAssetTransfer& transferAsset, const std::string& address, const COutPoint& out, const CTxOut& txOut)
{
    // TODO when we are ready. Make sure to only database after we are done validating everything
    if (vpwallets[0]->IsMine(txOut) == ISMINE_SPENDABLE) {
        if (!AddToMyUpspentOutPoints(transferAsset.strName, out))
            return error("%s: Failed to add an asset I own to my Unspent Asset Database. Asset Name: %s, OutPoint: %s",
                         __func__, transferAsset.strName, out.ToString());

        // We only care about balances pertaining to addresses that are mine
        if (!AddToAssetBalance(transferAsset.strName, address, transferAsset.nAmount))
            return error("%s: Failed to Update Asset Balance. Asset Name: %s, OutPoint: %s",
                         __func__, transferAsset.strName, out.ToString());
    }

    return true;
}

bool CAssets::AddToAssetBalance(const std::string& strName, const std::string& address, const CAmount& nAmount)
{
    auto pair = std::make_pair(strName, address);
    // Add to map address -> amount map
    if (mapAssetsAddressAmount.count(pair)) {
        mapAssetsAddressAmount.at(pair) += nAmount;
    } else {
        mapAssetsAddressAmount.insert(std::make_pair(pair, nAmount));
    }

    // Add to map of addresses
    if (!mapAssetsAddresses.count(strName)) {
        mapAssetsAddresses.insert(std::make_pair(strName, std::set<std::string>()));
    }
    mapAssetsAddresses.at(strName).insert(address);

    if (!passetsdb->WriteAssetAddressQuantity(strName, address, mapAssetsAddressAmount.at(pair)))
        return error("%s : Failed to write to asset address quantity database, Asset Name: %s, Address: %s, Amount: %d", __func__, strName, address, mapAssetsAddressAmount.at(pair));

    return true;
}

void CAssets::TrySpendCoin(const COutPoint& out, const Coin& coin)
{
    std::pair<std::string, COutPoint> pairToRemove = std::make_pair("", COutPoint());
    for (auto setOuts : mapMyUnspentAssets) {
        // If we own one of the assets, we need to update our databases and memory
        if (setOuts.second.count(out)) {

            // Placeholder strings that will get set if you successfully get the transfer or asset from the script
            std::string address = "";
            std::string assetName = "";

            // Get the New Asset or Transfer Asset from the scriptPubKey
            if (coin.out.scriptPubKey.IsNewAsset()) {
                CNewAsset asset;
                if (AssetFromScript(coin.out.scriptPubKey, asset, address))
                    assetName = asset.strName;
            } else if (coin.out.scriptPubKey.IsTransferAsset()) {
                CAssetTransfer transfer;
                if (TransferAssetFromScript(coin.out.scriptPubKey, transfer, address))
                    assetName = transfer.strName;
            }

            // If we got the address and the assetName, proceed to remove it from the database, and in memory objects
            if (address != "" && assetName != "") {
                if (!passetsdb->EraseAssetAddressQuantity(assetName, address))
                    LogPrintf(
                            "%s : ERROR Failed erasing address quantity from database, Asset Name: %s, Address : %s\n",
                            __func__, assetName, address);

                setOuts.second.erase(out);
                if (!passetsdb->EraseMyOutPoints(assetName) || !passetsdb->WriteMyAssetsData(assetName, setOuts.second))
                    LogPrintf("%s : ERROR Failed databasing asset spend OutPoint: %s\n", __func__, out.ToString());
                
                mapAssetsAddressAmount.erase(make_pair(assetName, address));
                pairToRemove = std::make_pair(assetName, out);
                mapAssetsAddresses.at(assetName).erase(address);
            } else {
                LogPrintf("%s : ERROR Failed get the asset from the OutPoint: %s\n", __func__, out.ToString());
            }
            break;
        }
    }

    if (pairToRemove.first != "" && !pairToRemove.second.IsNull() && mapMyUnspentAssets.count(pairToRemove.first)) {
        mapMyUnspentAssets.at(pairToRemove.first).erase(pairToRemove.second);
    }

}

bool CAssets::AddToMyUpspentOutPoints(const std::string& strName, const COutPoint& out, bool fMemoryOnly)
{
    LOCK(cs_main);

    if (!mapMyUnspentAssets.count(strName)) {
        std::set<COutPoint> setOuts;
        setOuts.insert(out);
        mapMyUnspentAssets.insert(std::make_pair(strName, setOuts));

    } else {
        if (!mapMyUnspentAssets[strName].insert(out).second)
            return error("%s: Tried adding an asset to my map of upspent assets, but it already exsisted in the set of assets: %s, COutPoint: %s", __func__, strName, out.ToString());
    }

    // TODO We are writing every COutPoint everytime we add an out to our list. Figure out a better way to do this
    if (!fMemoryOnly && !passetsdb->WriteMyAssetsData(strName, mapMyUnspentAssets[strName]))
        return error("%s: Failed adding an asset I own to database: %s, COutPoint: %s", __func__, strName,
                     out.ToString());

    LogPrintf("%s: Added an asset that I own: %s, COutPoint: %s\n", __func__, strName, out.ToString());

    return true;
}

bool CAssets::ContainsAsset(const CNewAsset& asset)
{
    return setAssets.find(asset) != setAssets.end();
}

bool CAssets::RemoveAssetAndOutPoints(const CNewAsset& asset, const std::string& address, const bool fMemoryOnly)
{
    if (setAssets.count(asset)) {
        setAssets.erase(asset);
        if (!fMemoryOnly && !passetsdb->EraseAssetData(asset.strName))
            return error("%s : Failed Erasing Asset Data from database. Asset Name: %s", __func__, asset.strName);
    }

    if (mapMyUnspentAssets.count(asset.strName)) {
        if (!fMemoryOnly && !passetsdb->EraseMyOutPoints(asset.strName))
            return error("%s : Failed Erasing My Asset Data OutPoints from database. Asset Name: %s", __func__, asset.strName);
        mapMyUnspentAssets.erase(asset.strName);
    }

    if (mapAssetsAddresses.count(asset.strName))
        mapAssetsAddresses[asset.strName].erase(address);

    auto it = mapAssetsAddressAmount.find(std::make_pair(asset.strName, address));
    if (it != mapAssetsAddressAmount.end()) {
        mapAssetsAddressAmount.erase(it);

        if (!fMemoryOnly && !passetsdb->EraseAssetAddressQuantity(asset.strName, address))
            return error("%s Failed Erasing Assest quantity from database. Asset Name: %s", __func__, asset.strName);
    }

    return true;
}

CNewAsset::CNewAsset(const std::string& strName, const CAmount& nAmount, const int& nNameLength, const int& units, const int& nReissuable, const int& nHasIPFS, const std::string& strIPFSHash)
{
    this->SetNull();
    this->strName = strName;
    this->nAmount = nAmount;
    this->nNameLength = int8_t(nNameLength);
    this->units = int8_t(units);
    this->nReissuable = int8_t(nReissuable);
    this->nHasIPFS = int8_t(nHasIPFS);
    this->strIPFSHash = strIPFSHash;
}

/**
 * Constructs a CScript that carries the asset name and quantity and adds to to the end of the given script
 * @param dest - The destination that the asset will belong to
 * @param script - This script needs to be a pay to address script
 */
void CNewAsset::ConstructTransaction(CScript& script) const
{
    CDataStream ssAsset(SER_NETWORK, PROTOCOL_VERSION);
    ssAsset << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(RVN_R); // r
    vchMessage.push_back(RVN_V); // v
    vchMessage.push_back(RVN_N); // n
    vchMessage.push_back(RVN_Q); // q

    vchMessage.insert(vchMessage.end(), ssAsset.begin(), ssAsset.end());
    script << OP_15 << vchMessage << OP_DROP;
}

bool AssetFromTransaction(const CTransaction& tx, CNewAsset& asset, std::string& strAddress)
{
    // Check to see if the transaction is an new asset issue tx
    if (!tx.IsNewAsset())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 1].scriptPubKey;

    return AssetFromScript(scriptPubKey, asset, strAddress);
}

bool TransferAssetFromScript(const CScript& scriptPubKey, CAssetTransfer& assetTransfer, std::string& strAddress)
{
    if (!IsScriptTransferAsset(scriptPubKey))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchTransferAsset;
    vchTransferAsset.insert(vchTransferAsset.end(), scriptPubKey.begin() + 31, scriptPubKey.end());
    CDataStream ssAsset(vchTransferAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssAsset >> assetTransfer;
    } catch(std::exception& e) {
        std::cout << "Failed to get the transfer asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool AssetFromScript(const CScript& scriptPubKey, CNewAsset& assetTransfer, std::string& strAddress)
{
    if (!IsScriptNewAsset(scriptPubKey))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchTransferAsset;
    vchTransferAsset.insert(vchTransferAsset.end(), scriptPubKey.begin() + 31, scriptPubKey.end());
    CDataStream ssAsset(vchTransferAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssAsset >> assetTransfer;
    } catch(std::exception& e) {
        std::cout << "Failed to get the asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool CTransaction::IsNewAsset() const
{
    // Issuing a new Asset must contain at least 3 CTxOut ( 500 Raven Burn, Any Number of other Outputs ..., Issue Asset Tx, Asset Metadata)
    if (vout.size() < 2)
        return false;

    // Check for the assets data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is places in a random position in the CWalletTx
    for (auto out : vout)
        if (CheckIssueBurnTx(out))
            return true;

    return false;
}

bool CheckIssueBurnTx(const CTxOut& txOut)
{
    // Check the first transaction is the 500 Burn amount to the burn address
    if (txOut.nValue != Params().IssueAssetBurnAmount())
        return false;

    // Extract the destination
    CTxDestination destination;
    if (!ExtractDestination(txOut.scriptPubKey, destination))
        return false;

    // Verify destination is valid
    if (!IsValidDestination(destination))
        return false;

    // Check destination address is the burn address
    if (EncodeDestination(destination) != Params().IssueAssetBurnAddress())
        return false;

    return true;
}

bool CheckIssueDataTx(const CTxOut& txOut)
{
    // Verify 'rvnq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptNewAsset(scriptPubKey);
}

bool IsScriptNewAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 39) {
        if (scriptPubKey[25] == OP_15 && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_Q) {
            return true;
        }
    }

    return false;
}

bool IsScriptTransferAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 30) {
        if (scriptPubKey[25] == OP_15 && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_T) {
            return true;
        }
    }

    return false;
}

CAssetTransfer::CAssetTransfer(const std::string& strAssetName, const CAmount& nAmount)
{
    this->strName = strAssetName;
    this->nAmount = nAmount;
}

void CAssetTransfer::ConstructTransaction(CScript& script) const
{
    CDataStream ssTransfer(SER_NETWORK, PROTOCOL_VERSION);
    ssTransfer << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(RVN_R); // r
    vchMessage.push_back(RVN_V); // v
    vchMessage.push_back(RVN_N); // n
    vchMessage.push_back(RVN_T); // t

    vchMessage.insert(vchMessage.end(), ssTransfer.begin(), ssTransfer.end());
    script << OP_15 << vchMessage << OP_DROP;
}

bool CAssetsCache::UndoAssetCoin(const Coin& coin, const COutPoint& out)
{
    std::string strAddress = "";
    std::string assetName = "";
    CAmount nAmount = 0;

    // Get the asset tx from the script
    if (IsScriptNewAsset(coin.out.scriptPubKey)) {
        CNewAsset asset;
        if (!AssetFromScript(coin.out.scriptPubKey, asset, strAddress))
            return error("%s : Failed to get asset from script while trying to undo asset. OutPoint : %s", __func__,
                         out.ToString());
        assetName = asset.strName;
        nAmount = asset.nAmount;
    } else if (IsScriptTransferAsset(coin.out.scriptPubKey)) {
        CAssetTransfer transfer;
        if (!TransferAssetFromScript(coin.out.scriptPubKey, transfer, strAddress))
            return error("%s : Failed to get transfer asset from script while trying to undo asset. OutPoint : %s", __func__,
                         out.ToString());

        assetName = transfer.strName;
        nAmount = transfer.nAmount;
    }

    if (assetName == "" || strAddress == "" || nAmount == 0)
        return error("%s : AssetName, Address or nAmount is invalid., Asset Name: %s, Address: %s, Amount: %d", __func__, assetName, strAddress, nAmount);

    if (!AddBackSpentAsset(coin, assetName, strAddress, nAmount, out))
        return (error("%s : Failed to add back the spent asset. OutPoint : %s", __func__, out.ToString()));

    return true;
}

//! Changes Memory Only
bool CAssetsCache::AddBackSpentAsset(const Coin& coin, const std::string& assetName, const std::string& address, const CAmount& nAmount, const COutPoint& out)
{
    // Add back the asset to its previous address
    if (!mapAssetsAddresses.count(assetName))
        mapAssetsAddresses.insert(std::make_pair(assetName, std::set<std::string>()));

    mapAssetsAddresses.at(assetName).insert(address);

    // Update the assets address balance
    auto pair = std::make_pair(assetName, address);
    if (!mapAssetsAddressAmount.count(pair))
        mapAssetsAddressAmount.insert(std::make_pair(pair, 0));

    mapAssetsAddressAmount.at(pair) += nAmount;

    // Add the undoAmount to the vector so we know what changes are dirty and what needs to be saved to database
    CUndoAssetAmount undoAmount(assetName, address, nAmount);
    vUndoAssetAmount.push_back(undoAmount);

    // If the CTxOut is mine add it to the list of unspent outpoints
    if (vpwallets[0]->IsMine(coin.out) == ISMINE_SPENDABLE) {
        if (!AddToMyUpspentOutPoints(assetName, out, true)) // Boolean true means only change the in memory data. We will want to save at the same time that RVN coin saves its cache
            return error("%s: Failed to add an asset I own to my Unspent Asset Database. asset %s",
                         __func__, assetName);

        // Add the outpoint to the set so we know what we need to database
        setChangeOwnedOutPoints.insert(assetName);
    }

    return true;
}

//! Changes Memory Only
bool CAssetsCache::UndoTransfer(const CAssetTransfer& transfer, const std::string& address, const COutPoint& outToRemove)
{
    // Make sure we are in a valid state to undo the tranfer of the asset
    auto pair = std::make_pair(transfer.strName, address);
    if (!mapAssetsAddressAmount.count(pair))
        return error("%s : Tried undoing a transfer and the map of address amount didn't have the asset address pair", __func__);

    if (mapAssetsAddressAmount.at(pair) < transfer.nAmount)
        return error("%s : Tried undoing a transfer and the map of address amount had less than the amount we are trying to undo", __func__);

    if (!mapAssetsAddresses.count(transfer.strName))
        return error("%s : Map asset address, didn't contain an entry for this asset we are trying to undo", __func__);

    if (!mapAssetsAddresses.at(transfer.strName).count(address))
        return error("%s : Map of asset address didn't have the address we are trying to undo", __func__);

    // Change the in memory balance of the asset at the address
    mapAssetsAddressAmount[std::make_pair(transfer.strName, address)] -= transfer.nAmount;

    // If the balance is now 0, remove it from the list
    if (mapAssetsAddressAmount.at(pair) == 0) {
        mapAssetsAddresses.at(transfer.strName).erase(address);
    }

    // If this transfer asset was added to my map of unspents remove the COutPoint
    if (mapMyUnspentAssets.count(transfer.strName))
        if (mapMyUnspentAssets.at(transfer.strName).count(outToRemove))
            mapMyUnspentAssets.at(transfer.strName).erase(outToRemove);

    // Update dirty memory so we know what to database
    setChangeOwnedOutPoints.insert(transfer.strName);

    return true;
}

//bool CAssets::Flush(const CAssetsDB* db)
//{
//    bool dirty = false;
//
//
//
//
//}

//! Changes Memory Only
bool CAssetsCache::RemoveNewAsset(const CNewAsset& asset, const std::string address)
{
    if (!setAssets.count(asset)) {
        return error("%s : Tried removeing an asset that isn't in our set of assets %s", __func__, asset.strName);
    }

    setAssets.erase(asset);

    if (mapMyUnspentAssets.count(asset.strName)) {
        mapMyUnspentAssets.erase(asset.strName);
    }

    if (mapAssetsAddresses.count(asset.strName))
        mapAssetsAddresses[asset.strName].erase(address);

    auto it = mapAssetsAddressAmount.find(std::make_pair(asset.strName, address));
    if (it != mapAssetsAddressAmount.end())
        mapAssetsAddressAmount.erase(it);

    vNewAssetsToRemove.push_back(std::make_pair(asset, address));

    return true;
}

//! Changes Memory Only
bool CAssetsCache::AddNewAsset(const CNewAsset& asset, const std::string address)
{
    if (!setAssets.insert(asset).second)
        return error("%s: Tried adding new asset, but it already exsisted in the set of assets: %s", __func__, asset.strName);

    // Insert the asset into the assets address map
    if (mapAssetsAddresses.count(asset.strName)) {
        if (mapAssetsAddresses[asset.strName].count(address))
            return error("%s : Tried adding a new asset and saving its quantity, but it already exsisted in the map of assets addresses: %s", __func__, asset.strName);

        mapAssetsAddresses[asset.strName].insert(address);
    } else {
        std::set<std::string> setAddresses;
        setAddresses.insert(address);
        mapAssetsAddresses.insert(std::make_pair(asset.strName, setAddresses));
    }

    // Insert the asset into the assests address amount map
    mapAssetsAddressAmount[std::make_pair(asset.strName, address)] = asset.nAmount;

    vNewAssetsToAdd.push_back(std::make_pair(asset, address));

    return true;
}

//! Changes Memory Only
bool CAssetsCache::RemoveTransfer(const CAssetTransfer &transfer, const std::string &address, const COutPoint &out)
{
    if (!UndoTransfer(transfer, address, out))
        return error("%s : Failed to undo the transfer", __func__);

    CUndoAssetTransfer undo(transfer, address, out);

    vUndoTransfer.push_back(undo);

    return true;
}

bool CAssetsCache::Flush()
{
    bool dirty = false;
    std::string message;

    // Add the new assets to database
    for (auto newAsset : vNewAssetsToAdd) {
        if (!passetsdb->WriteAssetData(newAsset.first)) {
            dirty = true;
            message = "_Failed Writing New Asset Data to database";
        }

        if (dirty) {
            return error("%s : %s", __func__, message);
        }

        if (!passetsdb->WriteAssetAddressQuantity(newAsset.first.strName, newAsset.second, newAsset.first.nAmount)) {
            dirty = true;
            message = "_Failed Writing Address Balance to database";
        }

        if (dirty) {
            return error("%s : %s", __func__, message);
        }
    }

    // Remove new assets from database
    for (auto newAsset : vNewAssetsToRemove) {
        if (!passetsdb->EraseAssetData(newAsset.first.strName)) {
            dirty = true;
            message = "_Failed Erasing New Asset Data from database";
        }

        if (dirty) {
            return error("%s : %s", __func__, message);
        }

        if (!passetsdb->EraseAssetAddressQuantity(newAsset.first.strName, newAsset.second)) {
            dirty = true;
            message = "_Failed Erasing Address Balance from database";
        }

        if (dirty) {
            return error("%s : %s", __func__, message);
        }
    }

    // Undo the transfering by updating the balances
    for (auto undoTransfer : vUndoTransfer) {
        auto pair = std::make_pair(undoTransfer.transfer.strName, undoTransfer.address);
        if (!mapAssetsAddressAmount.count(pair)) {
            if (!passetsdb->EraseAssetAddressQuantity(undoTransfer.transfer.strName, undoTransfer.address)) {
                dirty = true;
                message = "_Failed Erasing Address Quantity from database";
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }
        } else {
            if (!passetsdb->WriteAssetAddressQuantity(undoTransfer.transfer.strName, undoTransfer.address, mapAssetsAddressAmount.at(pair))) {
                dirty = true;
                message = "_Failed Writing updated Address Quantity to database when undoing transfers";
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }
        }
    }

    // Unspend the assets if they were spent and update the balances
    for (auto undoSpend : vUndoAssetAmount) {
        auto pair = std::make_pair(undoSpend.assetName, undoSpend.address);
        if (!passetsdb->WriteAssetAddressQuantity(undoSpend.assetName, undoSpend.address, mapAssetsAddressAmount.at(pair))) {
                dirty = true;
                message = "_Failed Writing updated Address Quantity to database when undoing spends";
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }
    }

    // For each set of my outpoints, database them
    for (auto updateOutPoints : setChangeOwnedOutPoints) {
        if (!passetsdb->WriteMyAssetsData(updateOutPoints, mapMyUnspentAssets.at(updateOutPoints))) {
            dirty = true;
            message = "_Failed Writing my set of outpoints to database";
        }

        if (dirty) {
            return error("%s : %s", __func__, message);
        }
    }

    passets->mapMyUnspentAssets = mapMyUnspentAssets;
    passets->mapAssetsAddressAmount = mapAssetsAddressAmount;
    passets->mapAssetsAddresses = mapAssetsAddresses;
    passets->setAssets = setAssets;

    return !dirty;
}
