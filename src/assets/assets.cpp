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
#include <boost/algorithm/string.hpp>
#include <consensus/validation.h>
#include "assets.h"
#include "assetdb.h"
#include "assettypes.h"

// excluding owner tag ('!')
static const auto MAX_NAME_LENGTH = 30;

// min lengths are expressed by quantifiers
static const std::regex ROOT_NAME_CHARACTERS("^[A-Z0-9._]{3,}$");
static const std::regex SUB_NAME_CHARACTERS("^[A-Z0-9._]+$");
static const std::regex UNIQUE_TAG_CHARACTERS("^[A-Za-z0-9@$%&*()[\\]{}<>\\-_.;?\\\\:]+$");
static const std::regex CHANNEL_TAG_CHARACTERS("^[A-Z0-9._]+$");

static const std::regex DOUBLE_PUNCTUATION("^.*[._]{2,}.*$");
static const std::regex LEADING_PUNCTUATION("^[._].*$");
static const std::regex TRAILING_PUNCTUATION("^.*[._]$");

static const std::string SUB_NAME_DELIMITER = "/";
static const std::string UNIQUE_TAG_DELIMITER = "#";
static const std::string CHANNEL_TAG_DELIMITER = "~";

static const std::regex UNIQUE_INDICATOR("^[^#]+#[^#]+$");
static const std::regex CHANNEL_INDICATOR("^[^~]+~[^~]+$");
static const std::regex OWNER_INDICATOR("^[^!]+!$");

bool IsRootNameValid(const std::string& name)
{
    return std::regex_match(name, ROOT_NAME_CHARACTERS)
        && !std::regex_match(name, DOUBLE_PUNCTUATION)
        && !std::regex_match(name, LEADING_PUNCTUATION)
        && !std::regex_match(name, TRAILING_PUNCTUATION);
}

bool IsSubNameValid(const std::string& name)
{
    return std::regex_match(name, SUB_NAME_CHARACTERS)
        && !std::regex_match(name, DOUBLE_PUNCTUATION)
        && !std::regex_match(name, LEADING_PUNCTUATION)
        && !std::regex_match(name, TRAILING_PUNCTUATION);
}

bool IsUniqueTagValid(const std::string& tag)
{
    return std::regex_match(tag, UNIQUE_TAG_CHARACTERS);
}

bool IsChannelTagValid(const std::string& tag)
{
    return std::regex_match(tag, CHANNEL_TAG_CHARACTERS)
        && !std::regex_match(tag, DOUBLE_PUNCTUATION)
        && !std::regex_match(tag, LEADING_PUNCTUATION)
        && !std::regex_match(tag, TRAILING_PUNCTUATION);
}

bool IsNameValidBeforeTag(const std::string& name)
{
    std::vector<std::string> parts;
    boost::split(parts, name, boost::is_any_of(SUB_NAME_DELIMITER));

    if (!IsRootNameValid(parts.front())) return false;

    if (parts.size() > 1)
    {
        for (unsigned long i = 1; i < parts.size(); i++)
        {
            if (!IsSubNameValid(parts[i])) return false;
        }
    }

    return true;
}

bool IsAssetNameValid(const std::string& name)
{

    if (std::regex_match(name, UNIQUE_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(UNIQUE_TAG_DELIMITER));
        return IsNameValidBeforeTag(parts.front())
            && IsUniqueTagValid(parts.back());
    }
    else if (std::regex_match(name, CHANNEL_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(CHANNEL_TAG_DELIMITER));
        return IsNameValidBeforeTag(parts.front())
            && IsChannelTagValid(parts.back());
    }
    else if (std::regex_match(name, OWNER_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH + 1) return false;
        return IsNameValidBeforeTag(name.substr(0, name.size() - 1));
    }
    else
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        return IsNameValidBeforeTag(name);
    }
}

bool IsAssetNameAnOwner(const std::string& name)
{
    return IsAssetNameValid(name) && std::regex_match(name, OWNER_INDICATOR);
}

bool CNewAsset::IsNull() const
{
    return strName == "";
}

bool CNewAsset::IsValid(std::string& strError, CAssetsCache& assetCache, bool fCheckMempool, bool fCheckDuplicateInputs)
{
    strError = "";

    // Check our current passets to see if the asset has been created yet
    if (fCheckDuplicateInputs) {
        if (assetCache.CheckIfAssetExists(this->strName)) {
            strError = std::string("Invalid parameter: asset_name '") + strName + std::string("' has already been used");
            return false;
        }
    }

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
        strError = "Invalid parameter: asset_name must only consist of valid characters and have a size between 3 and 31 characters. See help for more details.";

    if (nAmount <= 0)
        strError  = "Invalid parameter: asset amount can't be equal to or less than zero.";

    if (units < 0 || units > 8)
        strError  = "Invalid parameter: units must be between 0-8.";

    if (nReissuable != 0 && nReissuable != 1)
        strError  = "Invalid parameter: reissuable must be 0 or 1";

    if (nHasIPFS != 0 && nHasIPFS != 1)
        strError  = "Invalid parameter: has_ipfs must be 0 or 1.";

    if (nHasIPFS && strIPFSHash.size() != 40)
        strError  = "Invalid parameter: ipfs_hash must be 40 bytes (currently " + std::to_string(strIPFSHash.size()) + ").";

    return strError == "";
}

CNewAsset::CNewAsset(const CNewAsset& asset)
{
    this->strName = asset.strName;
    this->nAmount = asset.nAmount;
    this->units = asset.units;
    this->nHasIPFS = asset.nHasIPFS;
    this->nReissuable = asset.nReissuable;
    this->strIPFSHash = asset.strIPFSHash;
}

CNewAsset& CNewAsset::operator=(const CNewAsset& asset)
{
    this->strName = asset.strName;
    this->nAmount = asset.nAmount;
    this->units = asset.units;
    this->nHasIPFS = asset.nHasIPFS;
    this->nReissuable = asset.nReissuable;
    this->strIPFSHash = asset.strIPFSHash;
    return *this;
}

std::string CNewAsset::ToString()
{
    std::stringstream ss;
    ss << "Printing an asset" << "\n";
    ss << "name : " << strName << "\n";
    ss << "amount : " << nAmount << "\n";
    ss << "units : " << std::to_string(units) << "\n";
    ss << "reissuable : " << std::to_string(nReissuable) << "\n";
    ss << "has_ipfs : " << std::to_string(nHasIPFS) << "\n";

    if (nHasIPFS)
        ss << "ipfs_hash : " << strIPFSHash;

    return ss.str();
}

CNewAsset::CNewAsset(const std::string& strName, const CAmount& nAmount, const int& units, const int& nReissuable, const int& nHasIPFS, const std::string& strIPFSHash)
{
    this->SetNull();
    this->strName = strName;
    this->nAmount = nAmount;
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
    script << OP_RVN_ASSET << vchMessage << OP_DROP;
}

void CNewAsset::ConstructOwnerTransaction(CScript& script) const
{
    CDataStream ssOwner(SER_NETWORK, PROTOCOL_VERSION);
    ssOwner << std::string(this->strName + OWNER);

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(RVN_R); // r
    vchMessage.push_back(RVN_V); // v
    vchMessage.push_back(RVN_N); // n
    vchMessage.push_back(RVN_O); // o

    vchMessage.insert(vchMessage.end(), ssOwner.begin(), ssOwner.end());
    script << OP_RVN_ASSET << vchMessage << OP_DROP;
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

bool ReissueAssetFromTransaction(const CTransaction& tx, CReissueAsset& reissue, std::string& strAddress)
{
    // Check to see if the transaction is a reissue tx
    if (!tx.IsReissueAsset())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 1].scriptPubKey;

    return ReissueAssetFromScript(scriptPubKey, reissue, strAddress);
}

bool IsNewOwnerTxValid(const CTransaction& tx, const std::string& assetName, const std::string& address, std::string& errorMsg)
{
    // TODO when ready to ship. Put the owner validation code in own method if needed
    std::string ownerName;
    std::string ownerAddress;
    if (!OwnerFromTransaction(tx, ownerName, ownerAddress)) {
        errorMsg = "bad-txns-bad-owner";
        return false;
    }

    int size = ownerName.size();

    if (ownerAddress != address) {
        errorMsg = "bad-txns-owner-address-mismatch";
        return false;
    }

    if (size < OWNER_LENGTH + MIN_ASSET_LENGTH) {
        errorMsg = "bad-txns-owner-asset-length";
        return false;
    }

    if (ownerName != std::string(assetName + OWNER)) {
        errorMsg = "bad-txns-owner-name-mismatch";
        return false;
    }

    return true;
}

bool OwnerFromTransaction(const CTransaction& tx, std::string& ownerName, std::string& strAddress)
{
    // Check to see if the transaction is an new asset issue tx
    if (!tx.IsNewAsset())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 2].scriptPubKey;

    return OwnerAssetFromScript(scriptPubKey, ownerName, strAddress);
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

bool OwnerAssetFromScript(const CScript& scriptPubKey, std::string& assetName, std::string& strAddress)
{
    if (!IsScriptOwnerAsset(scriptPubKey))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchOwnerAsset;
    vchOwnerAsset.insert(vchOwnerAsset.end(), scriptPubKey.begin() + 31, scriptPubKey.end());
    CDataStream ssOwner(vchOwnerAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssOwner >> assetName;
    } catch(std::exception& e) {
        std::cout << "Failed to get the owner asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool ReissueAssetFromScript(const CScript& scriptPubKey, CReissueAsset& reissue, std::string& strAddress)
{
    if (!IsScriptReissueAsset(scriptPubKey))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchTransferAsset;
    vchTransferAsset.insert(vchTransferAsset.end(), scriptPubKey.begin() + 31, scriptPubKey.end());
    CDataStream ssReissue(vchTransferAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssReissue >> reissue;
    } catch(std::exception& e) {
        std::cout << "Failed to get the reissue asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool CTransaction::IsNewAsset() const
{
    // Reissuing an Asset must contain at least 3 CTxOut( Raven Burn Tx, Any Number of other Outputs ..., Owner Asset Change Tx, Reissue Tx)
    if (vout.size() < 3)
        return false;

    // Check for the assets data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    // Check to make sure the owner asset is created
    if (!CheckOwnerDataTx(vout[vout.size() - 2]))
        return false;

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is places in a random position in the CWalletTx
    for (auto out : vout)
        if (CheckIssueBurnTx(out))
            return true;

    return false;
}

bool CTransaction::IsReissueAsset() const
{
    // Reissuing an Asset must contain at least 3 CTxOut ( Raven Burn Tx, Any Number of other Outputs ..., Reissue Asset Tx, Owner Asset Change Tx)
    if (vout.size() < 3)
        return false;

    // Check for the reissue asset data CTxOut. This will always be the last output in the transaction
    if (!CheckReissueDataTx(vout[vout.size() - 1]))
        return false;

    // Check that there is an asset transfer, this will be the owner asset change
    bool ownerFound = false;
    for (auto out : vout)
        if (CheckTransferOwnerTx(out)) {
            ownerFound = true;
            break;
        }

    if (!ownerFound)
        return false;

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is placed in a random position in the CWalletTx
    for (auto out : vout)
        if (CheckReissueBurnTx(out))
            return true;

    return false;
}

CAssetTransfer::CAssetTransfer(const std::string& strAssetName, const CAmount& nAmount)
{
    this->strName = strAssetName;
    this->nAmount = nAmount;
}

bool CAssetTransfer::IsValid(std::string& strError) const
{
    strError = "";

    if (!IsAssetNameValid(std::string(strName)))
        strError = "Invalid parameter: asset_name must only consist of valid characters and have a size between 3 and 31 characters. See help for more details.";

    if (nAmount <= 0)
        strError  = "Invalid parameter: asset amount can't be equal to or less than zero.";

    return strError == "";
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
    script << OP_RVN_ASSET << vchMessage << OP_DROP;
}

CReissueAsset::CReissueAsset(const std::string &strAssetName, const CAmount &nAmount, const int &nReissuable,
                             const std::string &strIPFSHash)
{

    this->strName = strAssetName;
    this->strIPFSHash = strIPFSHash;
    this->nReissuable = int8_t(nReissuable);
    this->nAmount = nAmount;
}

bool CReissueAsset::IsValid(std::string &strError, CAssetsCache& assetCache) const
{
    strError = "";

    CNewAsset asset;
    if (!assetCache.GetAssetIfExists(this->strName, asset)) {
        strError = std::string("Unable to reissue asset: asset_name '") + strName + std::string("' doesn't exsist in the database");
        return false;
    }

    if (!asset.nReissuable) {
        // Check to make sure the asset can be reissued
        strError = "Unable to reissue asset: reissuable is set to false";
        return false;
    }

    if (asset.nAmount + this->nAmount > MAX_MONEY) {
        strError = std::string("Unable to reissue asset: asset_name '") + strName +
                   std::string("' the amount trying to reissue is to large");
        return false;
    }

    if (nAmount % int64_t(pow(10, (MAX_UNIT - asset.units))) != 0) {
        strError = "Unable to reissue asset: amount must be divisable by the smaller unit assigned to the asset";
        return false;
    }

    if (strIPFSHash != "" && strIPFSHash.size() != 40) {
        strError = "Unable to reissue asset: new ipfs_hash must be 40 bytes.";
        return false;
    }

    if (nAmount <= 0) {
        strError = "Unable to reissue asset: amount must be 1 or larger";
        return false;
    }

    return true;
}

void CReissueAsset::ConstructTransaction(CScript& script) const
{
    CDataStream ssReissue(SER_NETWORK, PROTOCOL_VERSION);
    ssReissue << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(RVN_R); // r
    vchMessage.push_back(RVN_V); // v
    vchMessage.push_back(RVN_N); // n
    vchMessage.push_back(RVN_R); // r

    vchMessage.insert(vchMessage.end(), ssReissue.begin(), ssReissue.end());
    script << OP_RVN_ASSET << vchMessage << OP_DROP;
}

bool CReissueAsset::IsNull() const
{
    return strName == "" || nAmount == 0;
}

bool CAssetsCache::GetAssetsOutPoints(const std::string& strName, std::set<COutPoint>& outpoints)
{
    if (mapMyUnspentAssets.count(strName)) {
        outpoints = mapMyUnspentAssets.at(strName);
        return true;
    }

    return false;
}

bool CAssetsCache::AddTransferAsset(const CAssetTransfer& transferAsset, const std::string& address, const COutPoint& out, const CTxOut& txOut)
{
    CAssetCachePossibleMine possibleMine(transferAsset.strName, out, txOut);
    if (!AddPossibleOutPoint(possibleMine))
        return false;

    AddToAssetBalance(transferAsset.strName, address, transferAsset.nAmount);

    // Add to cache so we can save to database
    CAssetCacheNewTransfer newTransfer(CAssetTransfer(transferAsset.strName, transferAsset.nAmount), address, out);

    if (setNewTransferAssetsToRemove.count(newTransfer))
        setNewTransferAssetsToRemove.erase(newTransfer);

    setNewTransferAssetsToAdd.insert(newTransfer);

    return true;
}

void CAssetsCache::AddToAssetBalance(const std::string& strName, const std::string& address, const CAmount& nAmount)
{
    auto pair = std::make_pair(strName, address);
    // Add to map address -> amount map

    // Get the best amount
    if (!GetBestAssetAddressAmount(*this, strName, address))
        mapAssetsAddressAmount.insert(make_pair(pair, 0));

    // Add the new amount to the balance
    mapAssetsAddressAmount.at(pair) += nAmount;

    // Add to map of addresses
    if (!mapAssetsAddresses.count(strName)) {
        mapAssetsAddresses.insert(std::make_pair(strName, std::set<std::string>()));
    }
    mapAssetsAddresses.at(strName).insert(address);
}

bool CAssetsCache::TrySpendCoin(const COutPoint& out, const CTxOut& txOut)
{

    if (vpwallets.size() == 0) {
        CAssetCachePossibleMine possibleMine("", out, txOut);
        setPossiblyMineRemove.insert(possibleMine);
        return true;
    }

    std::pair<std::string, COutPoint> pairToRemove = std::make_pair("", COutPoint());
    for (auto setOuts : mapMyUnspentAssets) {
        // If we own one of the assets, we need to update our databases and memory
        if (setOuts.second.count(out)) {

            // Placeholder strings that will get set if you successfully get the transfer or asset from the script
            std::string address = "";
            std::string assetName = "";

            // Get the New Asset or Transfer Asset from the scriptPubKey
            if (txOut.scriptPubKey.IsNewAsset()) {
                CNewAsset asset;
                if (AssetFromScript(txOut.scriptPubKey, asset, address))
                    assetName = asset.strName;
            } else if (txOut.scriptPubKey.IsTransferAsset()) {
                CAssetTransfer transfer;
                if (TransferAssetFromScript(txOut.scriptPubKey, transfer, address))
                    assetName = transfer.strName;
            } else if (txOut.scriptPubKey.IsOwnerAsset()) {
                if (!OwnerAssetFromScript(txOut.scriptPubKey, assetName, address))
                    return error("%s : ERROR Failed to get owner asset from the OutPoint: %s", __func__, out.ToString());
            } else if (txOut.scriptPubKey.IsReissueAsset()) {
                CReissueAsset reissue;
                if (ReissueAssetFromScript(txOut.scriptPubKey, reissue, address))
                    assetName = reissue.strName;
            }

            // If we got the address and the assetName, proceed to remove it from the database, and in memory objects
            if (address != "" && assetName != "") {
                CAssetCacheSpendAsset spend(assetName, address);

                mapAssetsAddressAmount[make_pair(assetName, address)] = 0;
                pairToRemove = std::make_pair(assetName, out);

                if (mapAssetsAddresses.count(assetName))
                    mapAssetsAddresses.at(assetName).erase(address);

                // Update the cache so we can save to database
                vSpentAssets.push_back(spend);

            } else {
                return error("%s : ERROR Failed to get asset from the OutPoint: %s", __func__, out.ToString());
            }
            break;
        }
    }

    if (pairToRemove.first != "" && !pairToRemove.second.IsNull() && mapMyUnspentAssets.count(pairToRemove.first)) {
        mapMyUnspentAssets.at(pairToRemove.first).erase(pairToRemove.second);

        // Update the cache so we know which assets set of outpoint we need to save to database
        setChangeOwnedOutPoints.insert(pairToRemove.first);
    }

    return true;
}

bool CAssetsCache::AddToMyUpspentOutPoints(const std::string& strName, const COutPoint& out)
{
    if (!mapMyUnspentAssets.count(strName)) {
        std::set<COutPoint> setOuts;
        setOuts.insert(out);
        mapMyUnspentAssets.insert(std::make_pair(strName, setOuts));
    } else {
        if (!mapMyUnspentAssets[strName].insert(out).second)
            return error("%s: Tried adding an asset to my map of upspent assets, but it already exsisted in the set of assets: %s, COutPoint: %s", __func__, strName, out.ToString());
    }

    // Add the outpoint to the set so we know what we need to database
    setChangeOwnedOutPoints.insert(strName);

    LogPrintf("%s: Added an asset that I own Asset Name : %s, COutPoint: %s\n", __func__, strName, out.ToString());

    return true;
}

bool CAssetsCache::ContainsAsset(const CNewAsset& asset)
{
    return CheckIfAssetExists(asset.strName);
}

bool CAssetsCache::ContainsAsset(const std::string& assetName)
{
    return CheckIfAssetExists(assetName);
}

bool CAssetsCache::AddPossibleOutPoint(const CAssetCachePossibleMine& possibleMine)
{
    if (vpwallets.size() == 0) {
        setPossiblyMineAdd.insert(possibleMine);
        return true;
    }

    // If asset is in an CTxOut that I own add it to my cache
    if (vpwallets[0]->IsMine(possibleMine.txOut) == ISMINE_SPENDABLE) {
        if (!AddToMyUpspentOutPoints(possibleMine.assetName, possibleMine.out))
            return error("%s: Failed to add an asset I own to my Unspent Asset Cache. asset %s",
                                        __func__, possibleMine.assetName);
    }

    return true;
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
            return error("%s : Failed to get asset from script while trying to undo asset spend. OutPoint : %s", __func__,
                         out.ToString());
        assetName = asset.strName;
        nAmount = asset.nAmount;
    } else if (IsScriptTransferAsset(coin.out.scriptPubKey)) {
        CAssetTransfer transfer;
        if (!TransferAssetFromScript(coin.out.scriptPubKey, transfer, strAddress))
            return error("%s : Failed to get transfer asset from script while trying to undo asset spend. OutPoint : %s", __func__,
                         out.ToString());

        assetName = transfer.strName;
        nAmount = transfer.nAmount;
    } else if (IsScriptOwnerAsset(coin.out.scriptPubKey)) {
        std::string ownerName;
        if (!OwnerAssetFromScript(coin.out.scriptPubKey, ownerName, strAddress))
            return error("%s : Failed to get owner asset from script while trying to undo asset spend. OutPoint : %s", __func__, out.ToString());
        assetName = ownerName;
        nAmount = OWNER_ASSET_AMOUNT;
    }

    if (assetName == "" || strAddress == "" || nAmount == 0)
        return error("%s : AssetName, Address or nAmount is invalid., Asset Name: %s, Address: %s, Amount: %d", __func__, assetName, strAddress, nAmount);

    if (!AddBackSpentAsset(coin, assetName, strAddress, nAmount, out))
        return error("%s : Failed to add back the spent asset. OutPoint : %s", __func__, out.ToString());

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

    // Get the map address amount from database if the map doesn't have it already
    if (!GetBestAssetAddressAmount(*this, assetName, address))
        mapAssetsAddressAmount.insert(std::make_pair(pair, 0));

    mapAssetsAddressAmount.at(pair) += nAmount;

    // Add the undoAmount to the vector so we know what changes are dirty and what needs to be saved to database
    CAssetCacheUndoAssetAmount undoAmount(assetName, address, nAmount);
    vUndoAssetAmount.push_back(undoAmount);

    CAssetCachePossibleMine possibleMine(assetName, out, coin.out);
    if (!AddPossibleOutPoint(possibleMine))
        return false;

    return true;
}

//! Changes Memory Only
bool CAssetsCache::UndoTransfer(const CAssetTransfer& transfer, const std::string& address, const COutPoint& outToRemove)
{
    // Make sure we are in a valid state to undo the tranfer of the asset

    if (!GetBestAssetAddressAmount(*this, transfer.strName, address))
        return error("%s : Failed to get the assets address balance from the database");

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
    mapAssetsAddressAmount[pair] -= transfer.nAmount;

    // If the balance is now 0, remove it from the list
    if (mapAssetsAddressAmount.at(pair) == 0) {
        mapAssetsAddresses.at(transfer.strName).erase(address);
    }

    // If this transfer asset was added to my map of unspents remove the COutPoint
    if (mapMyUnspentAssets.count(transfer.strName))
        if (mapMyUnspentAssets.at(transfer.strName).count(outToRemove))
            mapMyUnspentAssets.at(transfer.strName).erase(outToRemove);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::RemoveNewAsset(const CNewAsset& asset, const std::string address)
{
    if (!CheckIfAssetExists(asset.strName))
        return error("%s : Tried removing an asset that didn't exist. Asset Name : %s", __func__, asset.strName);

    // Remove the new asset from my unspent outpoints
    if (mapMyUnspentAssets.count(asset.strName)) {
        mapMyUnspentAssets.erase(asset.strName);

        // Add the asset name to the set, so I know which asset outpoint changes to write to database
        setChangeOwnedOutPoints.insert(asset.strName);
    }

    if (mapAssetsAddresses.count(asset.strName))
        mapAssetsAddresses[asset.strName].erase(address);

    mapAssetsAddressAmount[std::make_pair(asset.strName, address)] = 0;

    CAssetCacheNewAsset newAsset(asset, address);

    if (setNewAssetsToAdd.count(newAsset))
        setNewAssetsToAdd.erase(newAsset);

    setNewAssetsToRemove.insert(newAsset);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::AddNewAsset(const CNewAsset& asset, const std::string address)
{
    if(CheckIfAssetExists(asset.strName))
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

    CAssetCacheNewAsset newAsset(asset, address);

    if (setNewAssetsToRemove.count(newAsset))
        setNewAssetsToRemove.erase(newAsset);

    setNewAssetsToAdd.insert(newAsset);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::AddReissueAsset(const CReissueAsset& reissue, const std::string address, const COutPoint& out)
{
    auto pair = std::make_pair(reissue.strName, address);

    CNewAsset assetData;
    if (!GetAssetIfExists(reissue.strName, assetData))
        return error("%s: Tried reissuing an asset, but that asset didn't exist: %s", __func__, reissue.strName);

    // Insert the asset into the assets address map
    if (mapAssetsAddresses.count(reissue.strName)) {
        if (!mapAssetsAddresses[reissue.strName].count(address))
            mapAssetsAddresses[reissue.strName].insert(address);
    } else {
        std::set<std::string> setAddresses;
        setAddresses.insert(address);
        mapAssetsAddresses.insert(std::make_pair(reissue.strName, setAddresses));
    }

    // Add the reissued amount to the address amount map
    if (!GetBestAssetAddressAmount(*this, reissue.strName, address))
        mapAssetsAddressAmount.insert(make_pair(pair, 0));

    // Add the reissued amount to the amount in the map
    mapAssetsAddressAmount[pair] += reissue.nAmount;

    // Insert the reissue information into the reissue map
    if (!mapReissuedAssetData.count(reissue.strName)) {
        assetData.nAmount += reissue.nAmount;
        assetData.nReissuable = reissue.nReissuable;
        if (reissue.strIPFSHash != "") {
            assetData.nHasIPFS = 1;
            assetData.strIPFSHash = reissue.strIPFSHash;
        }
        mapReissuedAssetData.insert(make_pair(reissue.strName, assetData));
    } else {
        mapReissuedAssetData.at(reissue.strName).nAmount += reissue.nAmount;
        mapReissuedAssetData.at(reissue.strName).nReissuable = reissue.nReissuable;
        if (reissue.strIPFSHash != "") {
            mapReissuedAssetData.at(reissue.strName).nHasIPFS = 1;
            mapReissuedAssetData.at(reissue.strName).strIPFSHash = reissue.strIPFSHash;
        }
    }

    CAssetCacheReissueAsset reissueAsset(reissue, address, out);

    if (setNewReissueToRemove.count(reissueAsset))
        setNewReissueToRemove.erase(reissueAsset);

    setNewReissueToAdd.insert(reissueAsset);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::RemoveReissueAsset(const CReissueAsset& reissue, const std::string address, const COutPoint& out, const std::vector<std::pair<std::string, std::string> >& vUndoIPFS)
{
    auto pair = std::make_pair(reissue.strName, address);

    CNewAsset assetData;
    if (!GetAssetIfExists(reissue.strName, assetData))
        return error("%s: Tried undoing reissue of an asset, but that asset didn't exist: %s", __func__, reissue.strName);

    // Remove the reissued asset outpoint if it belongs to my unspent assets
    if (mapMyUnspentAssets.count(reissue.strName)) {
        mapMyUnspentAssets.at(reissue.strName).erase(out);

        // Add the asset name to the set, so I know which asset outpoint changes to write to database
        setChangeOwnedOutPoints.insert(reissue.strName);
    }

    // Get the best amount form the database or dirty cache
    if (!GetBestAssetAddressAmount(*this, reissue.strName, address))
        return error("%s : Trying to undo reissue of an asset but the assets amount isn't in the database", __func__);

    mapAssetsAddressAmount[pair] -= reissue.nAmount;

    if (mapAssetsAddressAmount[pair] < 0)
        return error("%s : Tried undoing reissue of an asset, but the assets amount went negative: %s", __func__, reissue.strName);

    // If the undid amount is now 0. Remove the address from the set of addresses
    if (mapAssetsAddresses.count(reissue.strName) && mapAssetsAddressAmount[pair] == 0) {
        mapAssetsAddresses.at(reissue.strName).erase(address);
    }

    // Change the asset data by undoing what was reissued
    assetData.nAmount -= reissue.nAmount;
    assetData.nReissuable = 1;

    // Find the ipfs hash in the undoblock data and restore the ipfs hash to its previous hash
    for (auto undoIPFS : vUndoIPFS) {
        if (undoIPFS.first == reissue.strName) {
            assetData.strIPFSHash = undoIPFS.second;

            if (assetData.strIPFSHash == "")
                assetData.nHasIPFS = 0;
            break;
        }
    }

    mapReissuedAssetData[assetData.strName] = assetData;

    CAssetCacheReissueAsset reissueAsset(reissue, address, out);

    if (setNewReissueToAdd.count(reissueAsset))
        setNewReissueToAdd.erase(reissueAsset);

    setNewReissueToRemove.insert(reissueAsset);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::AddOwnerAsset(const std::string& assetsName, const std::string address)
{
    if (mapAssetsAddresses.count(assetsName)) {
        if (mapAssetsAddresses[assetsName].count(address))
            return error("%s : Tried adding an owner asset, but it already exsisted in the map of assets addresses: %s",
                         __func__, assetsName);

        mapAssetsAddresses[assetsName].insert(address);

    } else {
        std::set<std::string> setAddresses;
        setAddresses.insert(address);
        mapAssetsAddresses.insert(std::make_pair(assetsName, setAddresses));
    }

    // Insert the asset into the assests address amount map
    mapAssetsAddressAmount[std::make_pair(assetsName, address)] = OWNER_ASSET_AMOUNT;


    // Update the cache
    CAssetCacheNewOwner newOwner(assetsName, address);

    if (setNewOwnerAssetsToRemove.count(newOwner))
        setNewOwnerAssetsToRemove.erase(newOwner);

    setNewOwnerAssetsToAdd.insert(newOwner);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::RemoveOwnerAsset(const std::string& assetsName, const std::string address)
{
    if (mapMyUnspentAssets.count(assetsName))
        mapMyUnspentAssets.erase(assetsName);

    if (mapAssetsAddresses.count(assetsName))
        mapAssetsAddresses[assetsName].erase(address);

    auto pair = std::make_pair(assetsName, address);

    mapAssetsAddressAmount[pair] = 0;

    // Update the cache
    CAssetCacheNewOwner newOwner(assetsName, address);
    if (setNewOwnerAssetsToAdd.count(newOwner))
        setNewOwnerAssetsToAdd.erase(newOwner);

    setNewOwnerAssetsToRemove.insert(newOwner);

    return true;
}

//! Changes Memory Only
bool CAssetsCache::RemoveTransfer(const CAssetTransfer &transfer, const std::string &address, const COutPoint &out)
{
    if (!UndoTransfer(transfer, address, out))
        return error("%s : Failed to undo the transfer", __func__);

    // Update the cache
    setChangeOwnedOutPoints.insert(transfer.strName);

    CAssetCacheNewTransfer newTransfer(transfer, address, out);
    if (setNewTransferAssetsToAdd.count(newTransfer))
        setNewTransferAssetsToAdd.erase(newTransfer);

    setNewTransferAssetsToRemove.insert(newTransfer);

    return true;
}

bool CAssetsCache::Flush(bool fSoftCopy, bool fFlushDB)
{
    try {
        if (fFlushDB) {
            bool dirty = false;
            std::string message;

            // Remove new assets from the database
            for (auto newAsset : setNewAssetsToRemove) {

                passetsCache->Erase(newAsset.asset.strName);
                if (!passetsdb->EraseAssetData(newAsset.asset.strName)) {
                    dirty = true;
                    message = "_Failed Erasing New Asset Data from database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }

                if (!passetsdb->EraseAssetAddressQuantity(newAsset.asset.strName, newAsset.address)) {
                    dirty = true;
                    message = "_Failed Erasing Address Balance from database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            // Add the new assets to the database
            for (auto newAsset : setNewAssetsToAdd) {

                passetsCache->Put(newAsset.asset.strName, newAsset.asset);
                if (!passetsdb->WriteAssetData(newAsset.asset)) {
                    dirty = true;
                    message = "_Failed Writing New Asset Data to database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }

                if (!passetsdb->WriteAssetAddressQuantity(newAsset.asset.strName, newAsset.address,
                                                          newAsset.asset.nAmount)) {
                    dirty = true;
                    message = "_Failed Writing Address Balance to database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            // Remove the new owners from database
            for (auto ownerAsset : setNewOwnerAssetsToRemove) {

                if (!passetsdb->EraseAssetAddressQuantity(ownerAsset.assetName, ownerAsset.address)) {
                    dirty = true;
                    message = "_Failed Erasing Owner Address Balance from database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            // Add the new owners to database
            for (auto ownerAsset : setNewOwnerAssetsToAdd) {

                if (!passetsdb->WriteAssetAddressQuantity(ownerAsset.assetName, ownerAsset.address,
                                                          OWNER_ASSET_AMOUNT)) {
                    dirty = true;
                    message = "_Failed Writing Owner Address Balance to database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            // Undo the transfering by updating the balances in the database
            for (auto undoTransfer : setNewTransferAssetsToRemove) {
                auto pair = std::make_pair(undoTransfer.transfer.strName, undoTransfer.address);
                if (mapAssetsAddressAmount.count(pair)) {
                    if (mapAssetsAddressAmount.at(pair) == 0) {
                        if (!passetsdb->EraseAssetAddressQuantity(undoTransfer.transfer.strName,
                                                                  undoTransfer.address)) {
                            dirty = true;
                            message = "_Failed Erasing Address Quantity from database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    } else {
                        if (!passetsdb->WriteAssetAddressQuantity(undoTransfer.transfer.strName, undoTransfer.address,
                                                                  mapAssetsAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing updated Address Quantity to database when undoing transfers";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    }
                }
            }

            // Save the new transfers by updating the quantity in the database
            for (auto newTransfer : setNewTransferAssetsToAdd) {
                auto pair = std::make_pair(newTransfer.transfer.strName, newTransfer.address);
                // During init and reindex it disconnects and verifies blocks, can create a state where vNewTransfer will contain transfers that have already been spent. So if they aren't in the map, we can skip them.
                if (mapAssetsAddressAmount.count(pair)) {
                    if (!passetsdb->WriteAssetAddressQuantity(newTransfer.transfer.strName, newTransfer.address,
                                                              mapAssetsAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing new address quantity to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }

            for (auto newReissue : setNewReissueToAdd) {
                auto reissue_name = newReissue.reissue.strName;

                if (mapReissuedAssetData.count(reissue_name)) {
                    if(!passetsdb->WriteAssetData(mapReissuedAssetData.at(reissue_name))) {
                        dirty = true;
                        message = "_Failed Writing reissue asset data to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }

                    passetsCache->Erase(reissue_name);
                }
            }

            for (auto undoReissue : setNewReissueToRemove) {
                auto reissue_name = undoReissue.reissue.strName;

                if (mapReissuedAssetData.count(reissue_name)) {
                    if(!passetsdb->WriteAssetData(mapReissuedAssetData.at(reissue_name))) {
                        dirty = true;
                        message = "_Failed Writing undo reissue asset data to database";
                    }

                    auto pair = make_pair(undoReissue.reissue.strName, undoReissue.address);
                    if (mapAssetsAddressAmount.count(pair)) {
                        if (mapAssetsAddressAmount.at(pair) == 0) {
                            if (!passetsdb->EraseAssetAddressQuantity(reissue_name, undoReissue.address)) {
                                dirty = true;
                                message = "_Failed Erasing Address Balance from database";
                            }
                        } else {
                            if (!passetsdb->WriteAssetAddressQuantity(reissue_name, undoReissue.address, mapAssetsAddressAmount.at(pair))) {
                                dirty = true;
                                message = "_Failed Writing the undo of reissue of asset from database";
                            }
                        }
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }

                    passetsCache->Erase(reissue_name);
                }
            }


            // Undo the asset spends by updating there balance in the database
            for (auto undoSpend : vUndoAssetAmount) {
                auto pair = std::make_pair(undoSpend.assetName, undoSpend.address);
                if (mapAssetsAddressAmount.count(pair)) {
                    if (!passetsdb->WriteAssetAddressQuantity(undoSpend.assetName, undoSpend.address,
                                                              mapAssetsAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing updated Address Quantity to database when undoing spends";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }

            // Save my outpoints to the database
            for (auto updateOutPoints : setChangeOwnedOutPoints) {
                if (mapMyUnspentAssets.count(updateOutPoints)) {
                    if (!passetsdb->WriteMyAssetsData(updateOutPoints, mapMyUnspentAssets.at(updateOutPoints))) {
                        dirty = true;
                        message = "_Failed Writing my set of outpoints to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }

            // Save the assets that have been spent by erasing the quantity in the database
            for (auto spentAsset : vSpentAssets) {
                if (!passetsdb->EraseAssetAddressQuantity(spentAsset.assetName, spentAsset.address)) {
                    dirty = true;
                    message = "_Failed Erasing a Spent Asset, from database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            ClearDirtyCache();
        }

        if (fSoftCopy) {
            passets->Copy(*this);
        }

        return true;
    } catch (const std::runtime_error& e) {
        return error("%s : %s ", __func__, std::string("System error while flushing assets: ") + e.what());
    }
}

//! Get the amount of memory the cache is using
size_t CAssetsCache::DynamicMemoryUsage() const
{
    // TODO make sure this is accurate
    return memusage::DynamicUsage(mapAssetsAddresses) + memusage::DynamicUsage(mapAssetsAddressAmount) + memusage::DynamicUsage(mapMyUnspentAssets);
}

//! Get an estimated size of the cache in bytes that will be needed inorder to save to database
size_t CAssetsCache::GetCacheSize() const
{
    size_t size = 0;
    size += 32 * setChangeOwnedOutPoints.size(); // COutPoint is 32 bytes
    size += 80 * setNewOwnerAssetsToAdd.size(); // CNewAsset is max 80 bytes
    size -= 80 * setNewOwnerAssetsToRemove.size(); // CNewAsset is max 80 bytes
    size += (1 + 32 + 40) * vUndoAssetAmount.size(); // 8 bit CAmount, 32 byte assetName, 40 bytes address
    size += (1 + 32 + 40) * setNewTransferAssetsToRemove.size(); // 8 bit CAmount, 32 byte assetName, 40 bytes address
    size += (32 + 40) * setNewTransferAssetsToAdd.size();
    size -= (32 + 40) * vSpentAssets.size();

    return size;
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

bool CheckIssueBurnTx(const CTxOut& txOut)
{
    // Check the first transaction is the 500 Burn amount to the burn address
    if (txOut.nValue != GetIssueAssetBurnAmount())
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

bool CheckReissueBurnTx(const CTxOut& txOut)
{
    // Check the first transaction and verify that the correct RVN Amount
    if (txOut.nValue != GetReissueAssetBurnAmount())
        return false;

    // Extract the destination
    CTxDestination destination;
    if (!ExtractDestination(txOut.scriptPubKey, destination))
        return false;

    // Verify destination is valid
    if (!IsValidDestination(destination))
        return false;

    // Check destination address is the correct burn address
    if (EncodeDestination(destination) != Params().ReissueAssetBurnAddress())
        return false;

    return true;
}

bool CheckIssueDataTx(const CTxOut& txOut)
{
    // Verify 'rvnq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptNewAsset(scriptPubKey);
}

bool CheckReissueDataTx(const CTxOut& txOut)
{
    // Verify 'rvnr' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptReissueAsset(scriptPubKey);
}

bool CheckOwnerDataTx(const CTxOut& txOut)
{
    // Verify 'rvnq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptOwnerAsset(scriptPubKey);
}

bool CheckTransferOwnerTx(const CTxOut& txOut)
{
    // Verify 'rvnq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptTransferAsset(scriptPubKey);
}

bool IsScriptNewAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 39) {
        if (scriptPubKey[25] == OP_RVN_ASSET && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_Q) {
            return true;
        }
    }

    return false;
}

bool IsScriptOwnerAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 30) {
        if (scriptPubKey[25] == OP_RVN_ASSET && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_O) {
            return true;
        }
    }

    return false;
}

bool IsScriptReissueAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 39) {
        if (scriptPubKey[25] == OP_RVN_ASSET && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_R) {
            return true;
        }
    }

    return false;
}

bool IsScriptTransferAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 30) {
        if (scriptPubKey[25] == OP_RVN_ASSET && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_T) {
            return true;
        }
    }

    return false;
}

void UpdatePossibleAssets()
{
    if (passets) {
        for (auto item : passets->setPossiblyMineRemove) {
            // If the CTxOut is mine add it to the list of unspent outpoints
            if (vpwallets[0]->IsMine(item.txOut) == ISMINE_SPENDABLE) {
                if (!passets->TrySpendCoin(item.out, item.txOut)) // Boolean true means only change the in memory data. We will want to save at the same time that RVN coin saves its cache
                    error("%s: Failed to add an asset I own to my Unspent Asset Database. asset %s",
                          __func__, item.assetName);
            }
        }

        for (auto item : passets->setPossiblyMineAdd) {
            // If the CTxOut is mine add it to the list of unspent outpoints
            if (vpwallets[0]->IsMine(item.txOut) == ISMINE_SPENDABLE) {
                if (!passets->AddToMyUpspentOutPoints(item.assetName, item.out)) // Boolean true means only change the in memory data. We will want to save at the same time that RVN coin saves its cache
                    error("%s: Failed to add an asset I own to my Unspent Asset Database. asset %s",
                                 __func__, item.assetName);
            }
        }

        std::vector<std::pair<std::string, COutPoint> > toRemove;
        for (auto item : passets->mapMyUnspentAssets) {
            for (auto out : item.second) {
                if (pcoinsTip->AccessCoin(out).IsSpent())
                    toRemove.emplace_back(std::make_pair(item.first, out));
            }
        }

        for (auto remove : toRemove) {
            passets->mapMyUnspentAssets.at(remove.first).erase(remove.second);
        }

    }
}


//! Returns a boolean on if the asset exists
bool CAssetsCache::CheckIfAssetExists(const std::string& name)
{
    // TODO we need to add some Locks to this I would think

    // Create objects that will be used to check the dirty cache
    CNewAsset asset;
    asset.strName = name;
    CAssetCacheNewAsset cachedAsset(asset, "");

    // Check the dirty caches first and see if it was recently added or removed
    if (setNewAssetsToRemove.count(cachedAsset))
        return false;

    if (setNewAssetsToAdd.count(cachedAsset))
        return true;

    // Check the cache, if it doesn't exist in the cache. Try and read it from database
    if (passetsCache) {
        if (passetsCache->Exists(name)) {
            return true;
        } else {
            if (passetsdb) {
                CNewAsset readAsset;
                if (passetsdb->ReadAssetData(name, readAsset)) {
                    passetsCache->Put(readAsset.strName, readAsset);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CAssetsCache::GetAssetIfExists(const std::string& name, CNewAsset& asset)
{
    // Check the map that contains the reissued asset data. If it is in this map, it hasn't been saved to disk yet
    if (mapReissuedAssetData.count(name)) {
        asset = mapReissuedAssetData.at(name);
        return true;
    }

    // Create objects that will be used to check the dirty cache
    CNewAsset tempAsset;
    tempAsset.strName = name;
    CAssetCacheNewAsset cachedAsset(tempAsset, "");

    // Check the dirty caches first and see if it was recently added or removed
    if (setNewAssetsToRemove.count(cachedAsset)) {
        return false;
    }

    auto setIterator = setNewAssetsToAdd.find(cachedAsset);
    if (setIterator != setNewAssetsToAdd.end()) {
        asset = setIterator->asset;
        return true;
    }

    // Check the cache, if it doesn't exist in the cache. Try and read it from database
    if (passetsCache) {
        if (passetsCache->Exists(name)) {
            asset = passetsCache->Get(name);
            return true;
        }
    }

    if (passetsdb && passetsCache) {
        CNewAsset readAsset;
        if (passetsdb->ReadAssetData(name, readAsset)) {
            asset = readAsset;
            passetsCache->Put(readAsset.strName, readAsset);
            return true;
        }
    }

    return false;
}

bool GetAssetFromCoin(const Coin& coin, std::string& strName, CAmount& nAmount)
{
    if (!coin.IsAsset())
        return false;

    // Determine the type of asset that the scriptpubkey contains and return the name and amount
    if (coin.out.scriptPubKey.IsNewAsset()) {
        CNewAsset asset;
        std::string address;
        if (!AssetFromScript(coin.out.scriptPubKey, asset, address))
            return false;
        strName = asset.strName;
        nAmount = asset.nAmount;
        return true;
    } else if (coin.out.scriptPubKey.IsTransferAsset()) {
        CAssetTransfer asset;
        std::string address;
        if (!TransferAssetFromScript(coin.out.scriptPubKey, asset, address))
            return false;
        strName = asset.strName;
        nAmount = asset.nAmount;
        return true;
    } else if (coin.out.scriptPubKey.IsOwnerAsset()) {
        std::string name;
        std::string address;
        if (!OwnerAssetFromScript(coin.out.scriptPubKey, name, address))
            return false;
        strName = name;
        nAmount = OWNER_ASSET_AMOUNT;
        return true;
    } else if (coin.out.scriptPubKey.IsReissueAsset()) {
        CReissueAsset reissue;
        std::string address;
        if (!ReissueAssetFromScript(coin.out.scriptPubKey, reissue, address))
            return false;
        strName = reissue.strName;
        nAmount = reissue.nAmount;
        return true;
    }

    return false;
}

bool CheckAssetOwner(const std::string& assetName)
{
    if (passets->mapMyUnspentAssets.count(assetName + OWNER)) {
        return true;
    }

    return false;
}

CAmount GetIssueAssetBurnAmount()
{
    return Params().IssueAssetBurnAmount();
}

CAmount GetReissueAssetBurnAmount()
{
    return Params().ReissueAssetBurnAmount();
}

CAmount GetIssueSubAssetBurnAmount()
{
    return Params().IssueSubAssetBurnAmount();
}

CAmount GetIssueUniqueAssetBurnAmount()
{
    return Params().IssueUniqueAssetBurnAmount();
}

//! This will get the amount that an address for a certain asset contains from the database if they cache doesn't already have it
bool GetBestAssetAddressAmount(CAssetsCache& cache, const std::string& assetName, const std::string& address)
{
    auto pair = make_pair(assetName, address);

    // If the caches map has the pair, return true because the map already contains the best dirty amount
    if (cache.mapAssetsAddressAmount.count(pair))
        return true;

    // If the database contains the assets address amount, insert it into the database and return true
    CAmount nDBAmount;
    if (passetsdb->ReadAssetAddressQuantity(pair.first, pair.second, nDBAmount)) {
        cache.mapAssetsAddressAmount.insert(make_pair(pair, nDBAmount));
        return true;
    }

    // The amount wasn't found return false
    return false;
}

