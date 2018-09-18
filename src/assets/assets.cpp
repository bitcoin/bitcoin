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
#include <rpc/protocol.h>
#include <net.h>
#include "assets.h"
#include "assetdb.h"
#include "assettypes.h"
#include "protocol.h"
#include "wallet/coincontrol.h"
#include "utilmoneystr.h"
#include "coins.h"
#include "wallet/wallet.h"

std::map<uint256, std::string> mapReissuedTx;
std::map<std::string, uint256> mapReissuedAssets;

// excluding owner tag ('!')
static const auto MAX_NAME_LENGTH = 31;
static const auto MAX_CHANNEL_NAME_LENGTH = 12;

// min lengths are expressed by quantifiers
static const std::regex ROOT_NAME_CHARACTERS("^[A-Z0-9._]{3,}$");
static const std::regex SUB_NAME_CHARACTERS("^[A-Z0-9._]+$");
static const std::regex UNIQUE_TAG_CHARACTERS(R"(^[-A-Za-z0-9@$%&*()[\]{}_.?:]+$)");
static const std::regex CHANNEL_TAG_CHARACTERS("^[A-Z0-9._]+$");
static const std::regex VOTE_TAG_CHARACTERS("^[A-Z0-9._]+$");

static const std::regex DOUBLE_PUNCTUATION("^.*[._]{2,}.*$");
static const std::regex LEADING_PUNCTUATION("^[._].*$");
static const std::regex TRAILING_PUNCTUATION("^.*[._]$");

static const std::string SUB_NAME_DELIMITER = "/";
static const std::string UNIQUE_TAG_DELIMITER = "#";
static const std::string CHANNEL_TAG_DELIMITER = "~";
static const std::string VOTE_TAG_DELIMITER = "^";

static const std::regex UNIQUE_INDICATOR(R"(^[^^~#!]+#[^~#!\/]+$)");
static const std::regex CHANNEL_INDICATOR(R"(^[^^~#!]+~[^~#!\/]+$)");
static const std::regex OWNER_INDICATOR(R"(^[^^~#!]+!$)");
static const std::regex VOTE_INDICATOR(R"(^[^^~#!]+\^[^~#!\/]+$)");

static const std::regex RAVEN_NAMES("^RVN$|^RAVEN$|^RAVENCOIN$|^RAVENC0IN$|^RAVENCO1N$|^RAVENC01N$");

bool IsRootNameValid(const std::string& name)
{
    return std::regex_match(name, ROOT_NAME_CHARACTERS)
        && !std::regex_match(name, DOUBLE_PUNCTUATION)
        && !std::regex_match(name, LEADING_PUNCTUATION)
        && !std::regex_match(name, TRAILING_PUNCTUATION)
        && !std::regex_match(name, RAVEN_NAMES);
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

bool IsVoteTagValid(const std::string& tag)
{
    return std::regex_match(tag, VOTE_TAG_CHARACTERS);
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

bool IsAssetNameASubasset(const std::string& name)
{
    std::vector<std::string> parts;
    boost::split(parts, name, boost::is_any_of(SUB_NAME_DELIMITER));

    if (!IsRootNameValid(parts.front())) return false;

    return parts.size() > 1;
}

bool IsAssetNameValid(const std::string& name, AssetType& assetType)
{
    assetType = AssetType::INVALID;
    if (std::regex_match(name, UNIQUE_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(UNIQUE_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsUniqueTagValid(parts.back());
        if (!valid) return false;
        assetType = AssetType::UNIQUE;
        return true;
    }
    else if (std::regex_match(name, CHANNEL_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(CHANNEL_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsChannelTagValid(parts.back());
        if (parts.back().size() > MAX_CHANNEL_NAME_LENGTH) return false;
        if (!valid) return false;
        assetType = AssetType::MSGCHANNEL;
        return true;
    }
    else if (std::regex_match(name, OWNER_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        bool valid = IsNameValidBeforeTag(name.substr(0, name.size() - 1));
        if (!valid) return false;
        assetType = AssetType::OWNER;
        return true;
    } else if (std::regex_match(name, VOTE_INDICATOR))
    {
        if (name.size() > MAX_NAME_LENGTH) return false;
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(VOTE_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsVoteTagValid(parts.back());
        if (!valid) return false;
        assetType = AssetType::VOTE;
        return true;
    }
    else
    {
        if (name.size() > MAX_NAME_LENGTH - 1) return false;  //Assets and sub-assets need to leave one extra char for OWNER indicator
        bool valid = IsNameValidBeforeTag(name);
        if (!valid) return false;
        assetType = IsAssetNameASubasset(name) ? AssetType::SUB : AssetType::ROOT;
        return true;
    }
}

bool IsAssetNameValid(const std::string& name)
{
    AssetType _assetType;
    return IsAssetNameValid(name, _assetType);
}

bool IsAssetNameAnOwner(const std::string& name)
{
    return IsAssetNameValid(name) && std::regex_match(name, OWNER_INDICATOR);
}

std::string GetParentName(const std::string& name)
{
    AssetType type;
    if (!IsAssetNameValid(name, type))
        return "";

    auto index = std::string::npos;
    if (type == AssetType::SUB) {
        index = name.find_last_of(SUB_NAME_DELIMITER);
    } else if (type == AssetType::UNIQUE) {
        index = name.find_last_of(UNIQUE_TAG_DELIMITER);
    } else if (type == AssetType::MSGCHANNEL) {
        index = name.find_last_of(CHANNEL_TAG_DELIMITER);
    } else if (type == AssetType::VOTE) {
        index = name.find_last_of(VOTE_TAG_DELIMITER);
    } else if (type == AssetType::ROOT)
        return name;

    if (std::string::npos != index)
    {
        return name.substr(0, index);
    }

    return name;
}

std::string GetUniqueAssetName(const std::string& parent, const std::string& tag)
{
    if (!IsRootNameValid(parent))
        return "";

    if (!IsUniqueTagValid(tag))
        return "";

    return parent + "#" + tag;
}

bool CNewAsset::IsNull() const
{
    return strName == "";
}

bool CNewAsset::IsValid(std::string& strError, CAssetsCache& assetCache, bool fCheckMempool, bool fCheckDuplicateInputs) const
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

    AssetType assetType;
    if (!IsAssetNameValid(std::string(strName), assetType)) {
        strError = "Invalid parameter: asset_name must only consist of valid characters and have a size between 3 and 30 characters. See help for more details.";
        return false;
    }

    if (assetType == AssetType::UNIQUE) {
        if (units != UNIQUE_ASSET_UNITS) {
            strError = "Invalid parameter: units must be " + std::to_string(UNIQUE_ASSET_UNITS / COIN);
            return false;
        }
        if (nAmount != UNIQUE_ASSET_AMOUNT) {
            strError = "Invalid parameter: amount must be " + std::to_string(UNIQUE_ASSET_AMOUNT);
            return false;
        }
        if (nReissuable != 0) {
            strError = "Invalid parameter: reissuable must be 0";
            return false;
        }
    }

    if (IsAssetNameAnOwner(std::string(strName))) {
        strError = "Invalid parameters: asset_name can't have a '!' at the end of it. See help for more details.";
        return false;
    }

    if (nAmount <= 0) {
        strError = "Invalid parameter: asset amount can't be equal to or less than zero.";
        return false;
    }

    if (nAmount > MAX_MONEY) {
        strError = "Invalid parameter: asset amount greater than max money: " + std::to_string(MAX_MONEY / COIN);
        return false;
    }

    if (units < 0 || units > 8) {
        strError = "Invalid parameter: units must be between 0-8.";
        return false;
    }

    if (!CheckAmountWithUnits(nAmount, units)) {
        strError = "Invalid parameter: amount must be divisible by the smaller unit assigned to the asset";
        return false;
    }

    if (nReissuable != 0 && nReissuable != 1) {
        strError = "Invalid parameter: reissuable must be 0 or 1";
        return false;
    }

    if (nHasIPFS != 0 && nHasIPFS != 1) {
        strError = "Invalid parameter: has_ipfs must be 0 or 1.";
        return false;
    }

    if (nHasIPFS && strIPFSHash.size() != 34) {
        strError = "Invalid parameter: ipfs_hash must be 34 bytes.";
        return false;
    }

    return true;
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

CNewAsset::CNewAsset(const std::string& strName, const CAmount& nAmount)
{
    this->SetNull();
    this->strName = strName;
    this->nAmount = nAmount;
    this->units = int8_t(DEFAULT_UNITS);
    this->nReissuable = int8_t(DEFAULT_REISSUABLE);
    this->nHasIPFS = int8_t(DEFAULT_HAS_IPFS);
    this->strIPFSHash = DEFAULT_IPFS;
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
    script << OP_RVN_ASSET << ToByteVector(vchMessage) << OP_DROP;
}

void CNewAsset::ConstructOwnerTransaction(CScript& script) const
{
    CDataStream ssOwner(SER_NETWORK, PROTOCOL_VERSION);
    ssOwner << std::string(this->strName + OWNER_TAG);

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(RVN_R); // r
    vchMessage.push_back(RVN_V); // v
    vchMessage.push_back(RVN_N); // n
    vchMessage.push_back(RVN_O); // o

    vchMessage.insert(vchMessage.end(), ssOwner.begin(), ssOwner.end());
    script << OP_RVN_ASSET << ToByteVector(vchMessage) << OP_DROP;
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

    if (ownerName != std::string(assetName + OWNER_TAG)) {
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
    int nStartingIndex = 0;
    if (!IsScriptTransferAsset(scriptPubKey, nStartingIndex))
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

bool AssetFromScript(const CScript& scriptPubKey, CNewAsset& assetNew, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptNewAsset(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchNewAsset;
    vchNewAsset.insert(vchNewAsset.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssAsset(vchNewAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssAsset >> assetNew;
    } catch(std::exception& e) {
        std::cout << "Failed to get the asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool OwnerAssetFromScript(const CScript& scriptPubKey, std::string& assetName, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptOwnerAsset(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchOwnerAsset;
    vchOwnerAsset.insert(vchOwnerAsset.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
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
    int nStartingIndex = 0;
    if (!IsScriptReissueAsset(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchReissueAsset;
    vchReissueAsset.insert(vchReissueAsset.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssReissue(vchReissueAsset, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssReissue >> reissue;
    } catch(std::exception& e) {
        std::cout << "Failed to get the reissue asset from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

//! Call VerifyNewAsset if this function returns true
bool CTransaction::IsNewAsset() const
{
    // Check for the assets data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    // Check to make sure the owner asset is created
    if (!CheckOwnerDataTx(vout[vout.size() - 2]))
        return false;

    // Don't overlap with IsNewUniqueAsset()
    if (IsScriptNewUniqueAsset(vout[vout.size() - 1].scriptPubKey))
        return false;

    return true;
}

//! To be called on CTransactions where IsNewAsset returns true
bool CTransaction::VerifyNewAsset() const
{
    // Issuing an Asset must contain at least 3 CTxOut( Raven Burn Tx, Any Number of other Outputs ..., Owner Asset Tx, New Asset Tx)
    if (vout.size() < 3)
        return false;

    // Check for the assets data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    // Check to make sure the owner asset is created
    if (!CheckOwnerDataTx(vout[vout.size() - 2]))
        return false;

    // Get the asset type
    CNewAsset asset;
    std::string address;
    if (!AssetFromScript(vout[vout.size() - 1].scriptPubKey, asset, address))
        return error("%s : Failed to get new asset from transaction: %s", __func__, this->GetHash().GetHex());

    AssetType assetType;
    IsAssetNameValid(asset.strName, assetType);

    std::string strOwnerName;
    if (!OwnerAssetFromScript(vout[vout.size() - 2].scriptPubKey, strOwnerName, address))
        return false;

    if (strOwnerName != asset.strName + OWNER_TAG)
        return false;

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is places in a random position in the CWalletTx
    for (auto out : vout)
        if (CheckIssueBurnTx(out, assetType))
            return true;

    return false;
}

//! Make sure to call VerifyNewUniqueAsset if this call returns true
bool CTransaction::IsNewUniqueAsset() const
{
    // Check trailing outpoint for issue data with unique asset name
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    if (!IsScriptNewUniqueAsset(vout[vout.size() - 1].scriptPubKey))
        return false;

    return true;
}

//! Call this function after IsNewUniqueAsset
bool CTransaction::VerifyNewUniqueAsset(CCoinsViewCache& view) const
{
    // Must contain at least 3 outpoints (RVN burn, owner change and one or more new unique assets that share a root (should be in trailing position))
    if (vout.size() < 3)
        return false;

    // check for (and count) new unique asset outpoints.  make sure they share a root.
    std::string assetRoot = "";
    int assetOutpointCount = 0;
    for (auto out : vout) {
        if (IsScriptNewUniqueAsset(out.scriptPubKey)) {
            CNewAsset asset;
            std::string address;
            if (!AssetFromScript(out.scriptPubKey, asset, address))
                return false;
            std::string root = GetParentName(asset.strName);
            if (assetRoot.compare("") == 0)
                assetRoot = root;
            if (assetRoot.compare(root) != 0)
                return false;
            assetOutpointCount += 1;
        }
    }
    if (assetOutpointCount == 0)
        return false;

    // check for burn outpoint (must account for each new asset)
    bool fBurnOutpointFound = false;
    for (auto out : vout)
        if (CheckIssueBurnTx(out, AssetType::UNIQUE, assetOutpointCount)) {
            fBurnOutpointFound = true;
            break;
        }
    if (!fBurnOutpointFound)
        return false;

    // check for owner change outpoint that matches root
    bool fOwnerOutFound = false;
    for (auto out : vout) {
        if (CheckTransferOwnerTx(out)) {
            fOwnerOutFound = true;
            break;
        }
    }

    if (!fOwnerOutFound)
        return false;

    // The owner change output must match a corresponding owner input
    bool fFoundCorrectInput = false;
    for (unsigned int i = 0; i < vin.size(); ++i) {
        const COutPoint &prevout = vin[i].prevout;
        const Coin& coin = view.AccessCoin(prevout);
        assert(!coin.IsSpent());

        int nType = -1;
        bool fOwner = false;
        if (coin.out.scriptPubKey.IsAssetScript(nType, fOwner)) {
            std::string strAssetName;
            CAmount nAssetAmount;
            if (!GetAssetInfoFromCoin(coin, strAssetName, nAssetAmount))
                continue;
            if (IsAssetNameAnOwner(strAssetName)) {
                if (strAssetName == assetRoot + OWNER_TAG) {
                    fFoundCorrectInput = true;
                    break;
                }
            }
        }
    }

    if (!fFoundCorrectInput)
        return false;

    return true;
}

bool CTransaction::IsReissueAsset() const
{
    // Check for the reissue asset data CTxOut. This will always be the last output in the transaction
    if (!CheckReissueDataTx(vout[vout.size() - 1]))
        return false;

    return true;
}

//! To be called on CTransactions where IsReissueAsset returns true
bool CTransaction::VerifyReissueAsset(CCoinsViewCache& view) const
{
    // Reissuing an Asset must contain at least 3 CTxOut ( Raven Burn Tx, Any Number of other Outputs ..., Reissue Asset Tx, Owner Asset Change Tx)
    if (vout.size() < 3)
        return false;

    // Check for the reissue asset data CTxOut. This will always be the last output in the transaction
    if (!CheckReissueDataTx(vout[vout.size() - 1]))
        return false;

    // Check that there is an asset transfer, this will be the owner asset change
    bool fOwnerOutFound = false;
    for (auto out : vout) {
        if (CheckTransferOwnerTx(out)) {
            fOwnerOutFound = true;
            break;
        }
    }

    if (!fOwnerOutFound)
        return false;

    CReissueAsset reissue;
    std::string address;
    if (!ReissueAssetFromScript(vout[vout.size() - 1].scriptPubKey, reissue, address))
        return false;

    bool fFoundCorrectInput = false;
    for (unsigned int i = 0; i < vin.size(); ++i) {
        const COutPoint &prevout = vin[i].prevout;
        const Coin& coin = view.AccessCoin(prevout);
        assert(!coin.IsSpent());

        int nType = -1;
        bool fOwner = false;
        if (coin.out.scriptPubKey.IsAssetScript(nType, fOwner)) {
            std::string strAssetName;
            CAmount nAssetAmount;
            if (!GetAssetInfoFromCoin(coin, strAssetName, nAssetAmount))
                continue;
            if (IsAssetNameAnOwner(strAssetName)) {
                if (strAssetName == reissue.strName + OWNER_TAG) {
                    fFoundCorrectInput = true;
                    break;
                }
            }
        }
    }

    if (!fFoundCorrectInput)
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
        strError = "Invalid parameter: asset_name must only consist of valid characters and have a size between 3 and 30 characters. See help for more details.";

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
    script << OP_RVN_ASSET << ToByteVector(vchMessage) << OP_DROP;
}

CReissueAsset::CReissueAsset(const std::string &strAssetName, const CAmount &nAmount, const int &nUnits, const int &nReissuable,
                             const std::string &strIPFSHash)
{

    this->strName = strAssetName;
    this->strIPFSHash = strIPFSHash;
    this->nReissuable = int8_t(nReissuable);
    this->nAmount = nAmount;
    this->nUnits = nUnits;
}

bool CReissueAsset::IsValid(std::string &strError, CAssetsCache& assetCache) const
{
    strError = "";

    CNewAsset asset;
    if (!assetCache.GetAssetIfExists(this->strName, asset)) {
        strError = std::string("Unable to reissue asset: asset_name '") + strName + std::string("' doesn't exist in the database");
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

    if (!CheckAmountWithUnits(nAmount, asset.units)) {
        strError = "Unable to reissue asset: amount must be divisible by the smaller unit assigned to the asset";
        return false;
    }

    if (strIPFSHash != "" && strIPFSHash.size() != 34) {
        strError = "Unable to reissue asset: new ipfs_hash must be 34 bytes.";
        return false;
    }

    if (nAmount < 0) {
        strError = "Unable to reissue asset: amount must be 0 or larger";
        return false;
    }

    if (nUnits > MAX_UNIT || nUnits < -1) {
        strError = "Unable to reissue asset: unit must be less than 8 and greater than -1";
        return false;
    }

    if (nUnits < asset.units && nUnits != -1) {
        strError = "Unable to reissue asset: unit must be larger than current unit selection";
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
    script << OP_RVN_ASSET << ToByteVector(vchMessage) << OP_DROP;
}

bool CReissueAsset::IsNull() const
{
    return strName == "" || nAmount < 0;
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
    if (IsAssetNameAnOwner(strName))
        mapAssetsAddressAmount.at(pair) = OWNER_ASSET_AMOUNT;
    else
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

    // Placeholder strings that will get set if you successfully get the transfer or asset from the script
    std::string address = "";
    std::string assetName = "";
    CAmount nAmount = -1;

    // Get the asset tx data
    int nType = -1;
    bool fIsOwner = false;
    if (txOut.scriptPubKey.IsAssetScript(nType, fIsOwner)) {

        // Get the New Asset or Transfer Asset from the scriptPubKey
        if (nType == TX_NEW_ASSET && !fIsOwner) {
            CNewAsset asset;
            if (AssetFromScript(txOut.scriptPubKey, asset, address)) {
                assetName = asset.strName;
                nAmount = asset.nAmount;
            }
        } else if (nType == TX_TRANSFER_ASSET) {
            CAssetTransfer transfer;
            if (TransferAssetFromScript(txOut.scriptPubKey, transfer, address)) {
                assetName = transfer.strName;
                nAmount = transfer.nAmount;
            }
        } else if (nType == TX_NEW_ASSET && fIsOwner) {
            if (!OwnerAssetFromScript(txOut.scriptPubKey, assetName, address))
                return error("%s : ERROR Failed to get owner asset from the OutPoint: %s", __func__,
                             out.ToString());
            nAmount = OWNER_ASSET_AMOUNT;
        } else if (nType == TX_REISSUE_ASSET) {
            CReissueAsset reissue;
            if (ReissueAssetFromScript(txOut.scriptPubKey, reissue, address)) {
                assetName = reissue.strName;
                nAmount = reissue.nAmount;
            }
        }
    } else {
        // If it isn't an asset tx return true, we only fail if an error occurs
        return true;
    }

    // If we got the address and the assetName, proceed to remove it from the database, and in memory objects
    if (address != "" && assetName != "" && nAmount > 0) {
        CAssetCacheSpendAsset spend(assetName, address, nAmount);
        if (GetBestAssetAddressAmount(*this, assetName, address)) {
            auto pair = make_pair(assetName, address);
            mapAssetsAddressAmount.at(pair) -= nAmount;

            if (mapAssetsAddressAmount.at(pair) < 0)
                mapAssetsAddressAmount.at(pair) = 0;
            if (mapAssetsAddressAmount.at(pair) == 0 &&
                mapAssetsAddresses.count(assetName))
                mapAssetsAddresses.at(assetName).erase(address);

            // Update the cache so we can save to database
            vSpentAssets.push_back(spend);
        }
    } else {
        return error("%s : ERROR Failed to get asset from the OutPoint: %s", __func__, out.ToString());
    }
    // If we own one of the assets, we need to update our databases and memory
    if (mapMyUnspentAssets.count(assetName)) {
        if (mapMyUnspentAssets.at(assetName).count(out)) {
            mapMyUnspentAssets.at(assetName).erase(out);

            // Update the cache so we know which assets set of outpoint we need to save to database
            setChangeOwnedOutPoints.insert(assetName);
        }
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
            return error("%s: Tried adding an asset to my map of upspent assets, but it already existed in the set of assets: %s, COutPoint: %s", __func__, strName, out.ToString());
    }

    // Add the outpoint to the set so we know what we need to database
    setChangeOwnedOutPoints.insert(strName);

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
    int nType = -1;
    bool fIsOwner = false;
    if(coin.out.scriptPubKey.IsAssetScript(nType, fIsOwner)) {

        if (nType == TX_NEW_ASSET && !fIsOwner) {
            CNewAsset asset;
            if (!AssetFromScript(coin.out.scriptPubKey, asset, strAddress)) {
                return error("%s : Failed to get asset from script while trying to undo asset spend. OutPoint : %s",
                             __func__,
                             out.ToString());
            }
            assetName = asset.strName;

            nAmount = asset.nAmount;
        } else if (nType == TX_TRANSFER_ASSET) {
            CAssetTransfer transfer;
            if (!TransferAssetFromScript(coin.out.scriptPubKey, transfer, strAddress))
                return error(
                        "%s : Failed to get transfer asset from script while trying to undo asset spend. OutPoint : %s",
                        __func__,
                        out.ToString());

            assetName = transfer.strName;
            nAmount = transfer.nAmount;
        } else if (nType == TX_NEW_ASSET && fIsOwner) {
            std::string ownerName;
            if (!OwnerAssetFromScript(coin.out.scriptPubKey, ownerName, strAddress))
                return error(
                        "%s : Failed to get owner asset from script while trying to undo asset spend. OutPoint : %s",
                        __func__, out.ToString());
            assetName = ownerName;
            nAmount = OWNER_ASSET_AMOUNT;
        } else if (nType == TX_REISSUE_ASSET) {
            CReissueAsset reissue;
            if (!ReissueAssetFromScript(coin.out.scriptPubKey, reissue, strAddress))
                return error(
                        "%s : Failed to get reissue asset from script while trying to undo asset spend. OutPoint : %s",
                        __func__, out.ToString());
            assetName = reissue.strName;
            nAmount = reissue.nAmount;
        }
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
    // Make sure we are in a valid state to undo the transfer of the asset
    if (!GetBestAssetAddressAmount(*this, transfer.strName, address))
        return error("%s : Failed to get the assets address balance from the database. Asset : %s Address : %s" , __func__, transfer.strName, address);

    auto pair = std::make_pair(transfer.strName, address);
    if (!mapAssetsAddressAmount.count(pair))
        return error("%s : Tried undoing a transfer and the map of address amount didn't have the asset address pair. Asset : %s Address : %s" , __func__, transfer.strName, address);

    if (mapAssetsAddressAmount.at(pair) < transfer.nAmount)
        return error("%s : Tried undoing a transfer and the map of address amount had less than the amount we are trying to undo. Asset : %s Address : %s" , __func__, transfer.strName, address);

    if (!mapAssetsAddresses.count(transfer.strName))
        return error("%s : Map asset address, didn't contain an entry for this asset we are trying to undo. Asset : %s Address : %s" , __func__, transfer.strName, address);

    if (!mapAssetsAddresses.at(transfer.strName).count(address))
        return error("%s : Map of asset address didn't have the address we are trying to undo. Asset : %s Address : %s" , __func__, transfer.strName, address);

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
        return error("%s: Tried adding new asset, but it already existed in the set of assets: %s", __func__, asset.strName);

    // Insert the asset into the assets address map
    if (mapAssetsAddresses.count(asset.strName)) {
        if (mapAssetsAddresses[asset.strName].count(address))
            return error("%s : Tried adding a new asset and saving its quantity, but it already existed in the map of assets addresses: %s", __func__, asset.strName);

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
        if (reissue.nUnits != -1)
            assetData.units = reissue.nUnits;

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
bool CAssetsCache::RemoveReissueAsset(const CReissueAsset& reissue, const std::string address, const COutPoint& out, const std::vector<std::pair<std::string, CBlockAssetUndo> >& vUndoIPFS)
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
    for (auto undoItem : vUndoIPFS) {
        if (undoItem.first == reissue.strName) {
            if (undoItem.second.fChangedIPFS)
                assetData.strIPFSHash = undoItem.second.strIPFS;
            if(undoItem.second.fChangedUnits)
                assetData.units = undoItem.second.nUnits;
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
            return error("%s : Tried adding an owner asset, but it already existed in the map of assets addresses: %s",
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
                auto pair = std::make_pair(ownerAsset.assetName, ownerAsset.address);
                if (mapAssetsAddressAmount.count(pair) && mapAssetsAddressAmount.at(pair) > 0) {
                    if (!passetsdb->WriteAssetAddressQuantity(ownerAsset.assetName, ownerAsset.address,
                                                              mapAssetsAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing Owner Address Balance to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
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
                auto pair = make_pair(reissue_name, newReissue.address);
                if (mapReissuedAssetData.count(reissue_name)) {
                    if(!passetsdb->WriteAssetData(mapReissuedAssetData.at(reissue_name))) {
                        dirty = true;
                        message = "_Failed Writing reissue asset data to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }

                    passetsCache->Erase(reissue_name);

                    if (mapAssetsAddressAmount.count(pair)) {
                        if (!passetsdb->WriteAssetAddressQuantity(pair.first, pair.second,
                                                                  mapAssetsAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing reissue asset quantity to the address quantity database";
                        }

                        if (dirty) {
                            return error("%s, %s", __func__, message);
                        }
                    }
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
                auto pair = make_pair(spentAsset.assetName, spentAsset.address);
                if (mapAssetsAddressAmount.count(pair)) {
                    if (mapAssetsAddressAmount.at(make_pair(spentAsset.assetName, spentAsset.address)) == 0) {
                        if (!passetsdb->EraseAssetAddressQuantity(spentAsset.assetName, spentAsset.address)) {
                            dirty = true;
                            message = "_Failed Erasing a Spent Asset, from database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    } else  {
                        if (!passetsdb->WriteAssetAddressQuantity(spentAsset.assetName, spentAsset.address, mapAssetsAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Erasing a Spent Asset, from database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    }
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

bool CheckIssueBurnTx(const CTxOut& txOut, const AssetType& type, const int numberIssued)
{
    CAmount burnAmount = 0;
    std::string burnAddress = "";

    if (type == AssetType::SUB) {
        burnAmount = GetIssueSubAssetBurnAmount();
        burnAddress = Params().IssueSubAssetBurnAddress();
    } else if (type == AssetType::ROOT) {
        burnAmount = GetIssueAssetBurnAmount();
        burnAddress = Params().IssueAssetBurnAddress();
    } else if (type == AssetType::UNIQUE) {
        burnAmount = GetIssueUniqueAssetBurnAmount();
        burnAddress = Params().IssueUniqueAssetBurnAddress();
    } else {
        return false;
    }

    // If issuing multiple (unique) assets need to burn for each
    burnAmount *= numberIssued;

    // Check the first transaction for the required Burn Amount for the asset type
    if (!(txOut.nValue == burnAmount))
        return false;

    // Extract the destination
    CTxDestination destination;
    if (!ExtractDestination(txOut.scriptPubKey, destination))
        return false;

    // Verify destination is valid
    if (!IsValidDestination(destination))
        return false;

    // Check destination address is the burn address
    auto strDestination = EncodeDestination(destination);
    if (!(strDestination == burnAddress))
        return false;

    return true;
}

bool CheckIssueBurnTx(const CTxOut& txOut, const AssetType& type)
{
    return CheckIssueBurnTx(txOut, type, 1);
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

    int nStartingIndex = 0;
    return IsScriptNewAsset(scriptPubKey, nStartingIndex);
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
    int index = 0;
    return IsScriptNewAsset(scriptPubKey, index);
}

bool IsScriptNewAsset(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsAssetScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_NEW_ASSET && !fIsOwner;
    }
    return false;
}

bool IsScriptNewUniqueAsset(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptNewUniqueAsset(scriptPubKey, index);
}

bool IsScriptNewUniqueAsset(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner = false;
    if (!scriptPubKey.IsAssetScript(nType, fIsOwner, nStartingIndex))
        return false;

    CNewAsset asset;
    std::string address;
    if (!AssetFromScript(scriptPubKey, asset, address))
        return false;

    AssetType assetType;
    if (!IsAssetNameValid(asset.strName, assetType))
        return false;

    return AssetType::UNIQUE == assetType;
}

bool IsScriptOwnerAsset(const CScript& scriptPubKey)
{

    int index = 0;
    return IsScriptOwnerAsset(scriptPubKey, index);
}

bool IsScriptOwnerAsset(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsAssetScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_NEW_ASSET && fIsOwner;
    }

    return false;
}

bool IsScriptReissueAsset(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptReissueAsset(scriptPubKey, index);
}

bool IsScriptReissueAsset(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsAssetScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_REISSUE_ASSET;
    }

    return false;
}

bool IsScriptTransferAsset(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptTransferAsset(scriptPubKey, index);
}

bool IsScriptTransferAsset(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsAssetScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_TRANSFER_ASSET;
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

bool GetAssetInfoFromScript(const CScript& scriptPubKey, std::string& strName, CAmount& nAmount)
{
    CAssetOutputEntry data;
    if(!GetAssetData(scriptPubKey, data))
        return false;

    strName = data.assetName;
    nAmount = data.nAmount;

    return true;
}

bool GetAssetInfoFromCoin(const Coin& coin, std::string& strName, CAmount& nAmount)
{
    return GetAssetInfoFromScript(coin.out.scriptPubKey, strName, nAmount);
}

bool GetAssetData(const CScript& script, CAssetOutputEntry& data)
{
    // Placeholder strings that will get set if you successfully get the transfer or asset from the script
    std::string address = "";
    std::string assetName = "";

    int nType = 0;
    bool fIsOwner = false;
    if (!script.IsAssetScript(nType, fIsOwner)) {
        return false;
    }

    txnouttype type = txnouttype(nType);

    // Get the New Asset or Transfer Asset from the scriptPubKey
    if (type == TX_NEW_ASSET && !fIsOwner) {
        CNewAsset asset;
        if (AssetFromScript(script, asset, address)) {
            data.type = TX_NEW_ASSET;
            data.nAmount = asset.nAmount;
            data.destination = DecodeDestination(address);
            data.assetName = asset.strName;
            return true;
        }
    } else if (type == TX_TRANSFER_ASSET) {
        CAssetTransfer transfer;
        if (TransferAssetFromScript(script, transfer, address)) {
            data.type = TX_TRANSFER_ASSET;
            data.nAmount = transfer.nAmount;
            data.destination = DecodeDestination(address);
            data.assetName = transfer.strName;
            return true;
        }
    } else if (type == TX_NEW_ASSET && fIsOwner) {
        if (OwnerAssetFromScript(script, assetName, address)) {
            data.type = TX_NEW_ASSET;
            data.nAmount = OWNER_ASSET_AMOUNT;
            data.destination = DecodeDestination(address);
            data.assetName = assetName;
            return true;
        }
    } else if (type == TX_REISSUE_ASSET) {
        CReissueAsset reissue;
        if (ReissueAssetFromScript(script, reissue, address)) {
            data.type = TX_REISSUE_ASSET;
            data.nAmount = reissue.nAmount;
            data.destination = DecodeDestination(address);
            data.assetName = reissue.strName;
            return true;
        }
    }

    return false;
}

void GetAllAdministrativeAssets(CWallet *pwallet, std::vector<std::string> &names)
{
    if(!pwallet)
        return;

    GetAllMyAssets(pwallet, names, true, true);
}

void GetAllMyAssets(CWallet* pwallet, std::vector<std::string>& names, bool fIncludeAdministrator, bool fOnlyAdministrator)
{
    if(!pwallet)
        return;

    std::map<std::string, std::vector<COutput> > mapAssets;
    pwallet->AvailableAssets(mapAssets);

    for (auto item : mapAssets) {
        bool isOwner = IsAssetNameAnOwner(item.first);

        if (isOwner) {
            if (fOnlyAdministrator || fIncludeAdministrator)
                names.emplace_back(item.first);
        } else {
            if (fOnlyAdministrator)
                continue;
            names.emplace_back(item.first);
        }
    }
}

void GetAllMyAssetsFromCache(std::vector<std::string>& names)
{
    if (!passets)
        return;

    for (auto owned : passets->mapMyUnspentAssets)
        names.emplace_back(owned.first);

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

CAmount GetBurnAmount(const int nType)
{
    return GetBurnAmount((AssetType(nType)));
}

CAmount GetBurnAmount(const AssetType type)
{
    switch (type) {
        case AssetType::ROOT:
            return GetIssueAssetBurnAmount();
        case AssetType::SUB:
            return GetIssueSubAssetBurnAmount();
        case AssetType::MSGCHANNEL:
            return 0;
        case AssetType::OWNER:
            return 0;
        case AssetType::UNIQUE:
            return GetIssueUniqueAssetBurnAmount();
        case AssetType::VOTE:
            return 0;
        case AssetType::REISSUE:
            return GetReissueAssetBurnAmount();
        default:
            return 0;
    }
}

std::string GetBurnAddress(const int nType)
{
    return GetBurnAddress((AssetType(nType)));
}

std::string GetBurnAddress(const AssetType type)
{
    switch (type) {
        case AssetType::ROOT:
            return Params().IssueAssetBurnAddress();
        case AssetType::SUB:
            return Params().IssueSubAssetBurnAddress();
        case AssetType::MSGCHANNEL:
            return "";
        case AssetType::OWNER:
            return "";
        case AssetType::UNIQUE:
            return Params().IssueUniqueAssetBurnAddress();
        case AssetType::VOTE:
            return "";
        case AssetType::REISSUE:
            return Params().ReissueAssetBurnAddress();
        default:
            return "";
    }
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

//! sets _assetNames_ to the set of names of owned assets
bool GetMyOwnedAssets(CAssetsCache& cache, std::vector<std::string>& assetNames) {
    for (auto const& entry : cache.mapMyUnspentAssets) {
        assetNames.push_back(entry.first);
    }

    return true;
}

//! sets _assetNames_ to the set of names of owned assets that start with _prefix_
bool GetMyOwnedAssets(CAssetsCache& cache, const std::string prefix, std::vector<std::string>& assetNames) {
    for (auto const& entry : cache.mapMyUnspentAssets)
        if (entry.first.find(prefix) == 0)
            assetNames.push_back(entry.first);

    return true;
}

//! sets _balance_ to the total quantity of _assetName_ owned across all addresses
bool GetMyAssetBalance(CAssetsCache& cache, const std::string& assetName, CAmount& balance) {
    balance = 0;
    for (auto const& address : cache.mapAssetsAddresses[assetName]) {
        if (vpwallets.size() == 0)
            return false;

        if (IsMine(*vpwallets[0], DecodeDestination(address), SIGVERSION_BASE) & ISMINE_ALL) {
            if (!GetBestAssetAddressAmount(cache, assetName, address)) {
                return false;
            }

            auto amt = cache.mapAssetsAddressAmount[make_pair(assetName, address)];
            balance += amt;
        }
    }

    return true;
}

//! sets _balances_ with the total quantity of each asset in _assetNames_
bool GetMyAssetBalances(CAssetsCache& cache, const std::vector<std::string>& assetNames, std::map<std::string, CAmount>& balances) {
    for (auto const& assetName : assetNames) {
        CAmount balance;
        if (!GetMyAssetBalance(cache, assetName, balance))
            return false;

        // don't include zero balances
        if (balance > 0)
            balances[assetName] = balance;
    }

    return true;
}

//! sets _balances_ with the total quantity of each owned asset
bool GetMyAssetBalances(CAssetsCache& cache, std::map<std::string, CAmount>& balances) {
    std::vector<std::string> assetNames;
    if (!GetMyOwnedAssets(cache, assetNames))
        return false;

    if (!GetMyAssetBalances(cache, assetNames, balances))
        return false;

    return true;
}

// 46 char base58 --> 34 char KAW compatible
std::string DecodeIPFS(std::string encoded)
{
    std::vector<unsigned char> b;
    DecodeBase58(encoded, b);
    return std::string(b.begin(), b.end());
};

// 34 char KAW compatible --> 46 char base58
std::string EncodeIPFS(std::string decoded){
    std::vector<char> charData(decoded.begin(), decoded.end());
    std::vector<unsigned char> unsignedCharData;
    for (char c : charData)
        unsignedCharData.push_back(static_cast<unsigned char>(c));
    return EncodeBase58(unsignedCharData);
};

bool CreateAssetTransaction(CWallet* pwallet, CCoinControl& coinControl, const CNewAsset& asset, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::vector<CNewAsset> assets;
    assets.push_back(asset);
    return CreateAssetTransaction(pwallet, coinControl, assets, address, error, wtxNew, reservekey, nFeeRequired);
}

bool CreateAssetTransaction(CWallet* pwallet, CCoinControl& coinControl, const std::vector<CNewAsset> assets, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::string change_address = EncodeDestination(coinControl.destChange);

    // Validate the assets data
    std::string strError;
    for (auto asset : assets) {
        if (!asset.IsValid(strError, *passets)) {
            error = std::make_pair(RPC_INVALID_PARAMETER, strError);
            return false;
        }
    }

    if (!change_address.empty()) {
        CTxDestination destination = DecodeDestination(change_address);
        if (!IsValidDestination(destination)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + change_address);
            return false;
        }
    } else {
        // no coin control: send change to newly generated address
        CKeyID keyID;
        std::string strFailReason;
        if (!pwallet->CreateNewChangeAddress(reservekey, keyID, strFailReason)) {
            error = std::make_pair(RPC_WALLET_KEYPOOL_RAN_OUT, strFailReason);
            return false;
        }

        change_address = EncodeDestination(keyID);
        coinControl.destChange = DecodeDestination(change_address);
    }

    AssetType assetType;
    std::string parentName;
    for (auto asset : assets) {
        if (!IsAssetNameValid(asset.strName, assetType)) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "Asset name not valid");
            return false;
        }
        if (assets.size() > 1 && assetType != AssetType::UNIQUE) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "Only unique assets can be issued in bulk.");
            return false;
        }
        std::string parent = GetParentName(asset.strName);
        if (parentName.empty())
            parentName = parent;
        if (parentName != parent) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "All assets must have the same parent.");
            return false;
        }
    }

    // Assign the correct burn amount and the correct burn address depending on the type of asset issuance that is happening
    CAmount burnAmount = GetBurnAmount(assetType) * assets.size();
    CScript scriptPubKey = GetScriptForDestination(DecodeDestination(GetBurnAddress(assetType)));

    CAmount curBalance = pwallet->GetBalance();

    // Check to make sure the wallet has the RVN required by the burnAmount
    if (curBalance < burnAmount) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        return false;
    }

    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    LOCK2(cs_main, pwallet->cs_wallet);

    // Create and send the transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;

    CRecipient recipient = {scriptPubKey, burnAmount, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);

    // If the asset is a subasset or unique asset. We need to send the ownertoken change back to ourselfs
    if (assetType == AssetType::SUB || assetType == AssetType::UNIQUE) {
        // Get the script for the destination address for the assets
        CScript scriptTransferOwnerAsset = GetScriptForDestination(DecodeDestination(change_address));

        CAssetTransfer assetTransfer(parentName + OWNER_TAG, OWNER_ASSET_AMOUNT);
        assetTransfer.ConstructTransaction(scriptTransferOwnerAsset);
        CRecipient rec = {scriptTransferOwnerAsset, 0, fSubtractFeeFromAmount};
        vecSend.push_back(rec);
    }

    // Get the owner outpoints if this is a subasset or unique asset
    if (assetType == AssetType::SUB || assetType == AssetType::UNIQUE) {
        // Verify that this wallet is the owner for the asset, and get the owner asset outpoint
        for (auto asset : assets) {
            if (!VerifyWalletHasAsset(parentName + OWNER_TAG, error)) {
                return false;
            }
        }
    }

    if (!pwallet->CreateTransactionWithAssets(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl, assets, DecodeDestination(address), assetType)) {
        if (!fSubtractFeeFromAmount && burnAmount + nFeeRequired > curBalance)
            strTxError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        error = std::make_pair(RPC_WALLET_ERROR, strTxError);
        return false;
    }
    return true;
}

bool CreateReissueAssetTransaction(CWallet* pwallet, CCoinControl& coinControl, const CReissueAsset& reissueAsset, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::string asset_name = reissueAsset.strName;
    std::string change_address = EncodeDestination(coinControl.destChange);

    // Check that validitity of the address
    if (!IsValidDestinationString(address)) {
        error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address);
        return false;
    }

    if (!change_address.empty()) {
        CTxDestination destination = DecodeDestination(change_address);
        if (!IsValidDestination(destination)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + change_address);
            return false;
        }
    } else {
        CKeyID keyID;
        std::string strFailReason;
        if (!pwallet->CreateNewChangeAddress(reservekey, keyID, strFailReason)) {
            error = std::make_pair(RPC_WALLET_KEYPOOL_RAN_OUT, strFailReason);
            return false;
        }

        change_address = EncodeDestination(keyID);
        coinControl.destChange = DecodeDestination(change_address);
    }

    // Check the assets name
    if (!IsAssetNameValid(asset_name)) {
        error = std::make_pair(RPC_INVALID_PARAMS, std::string("Invalid asset name: ") + asset_name);
        return false;
    }

    if (IsAssetNameAnOwner(asset_name)) {
        error = std::make_pair(RPC_INVALID_PARAMS, std::string("Owner Assets are not able to be reissued"));
        return false;
    }

    // passets and passetsCache need to be initialized
    if (!passets) {
        error = std::make_pair(RPC_DATABASE_ERROR, std::string("passets isn't initialized"));
        return false;
    }

    if (!passetsCache) {
        error = std::make_pair(RPC_DATABASE_ERROR,
                               std::string("passetsCache isn't initialized"));
        return false;
    }

    std::string strError;
    if (!reissueAsset.IsValid(strError, *passets)) {
        error = std::make_pair(RPC_VERIFY_ERROR,
                               std::string("Failed to create reissue asset object. Error: ") + strError);
        return false;
    }

    // Verify that this wallet is the owner for the asset, and get the owner asset outpoint
    if (!VerifyWalletHasAsset(asset_name, error)) {
        return false;
    }

    // Check the wallet balance
    CAmount curBalance = pwallet->GetBalance();

    // Get the current burn amount for issuing an asset
    CAmount burnAmount = GetReissueAssetBurnAmount();

    // Check to make sure the wallet has the RVN required by the burnAmount
    if (curBalance < burnAmount) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        return false;
    }

    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    // Get the script for the destination address for the assets
    CScript scriptTransferOwnerAsset = GetScriptForDestination(DecodeDestination(change_address));

    CAssetTransfer assetTransfer(asset_name + OWNER_TAG, OWNER_ASSET_AMOUNT);
    assetTransfer.ConstructTransaction(scriptTransferOwnerAsset);

    // Get the script for the burn address
    CScript scriptPubKeyBurn = GetScriptForDestination(DecodeDestination(Params().ReissueAssetBurnAddress()));

    // Create and send the transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;
    CRecipient recipient = {scriptPubKeyBurn, burnAmount, fSubtractFeeFromAmount};
    CRecipient recipient2 = {scriptTransferOwnerAsset, 0, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);
    vecSend.push_back(recipient2);
    if (!pwallet->CreateTransactionWithReissueAsset(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl, reissueAsset, DecodeDestination(address))) {
        if (!fSubtractFeeFromAmount && burnAmount + nFeeRequired > curBalance)
            strTxError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        error = std::make_pair(RPC_WALLET_ERROR, strTxError);
        return false;
    }
    return true;
}

bool CreateTransferAssetTransaction(CWallet* pwallet, const CCoinControl& coinControl, const std::vector< std::pair<CAssetTransfer, std::string> >vTransfers, const std::string& changeAddress, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    // Initialize Values for transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;

    // Check for a balance before processing transfers
    CAmount curBalance = pwallet->GetBalance();
    if (curBalance == 0) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, std::string("This wallet doesn't contain any RVN, transfering an asset requires a network fee"));
        return false;
    }

    // Check for peers and connections
    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    // Loop through all transfers and create scriptpubkeys for them
    for (auto transfer : vTransfers) {
        std::string address = transfer.second;
        std::string asset_name = transfer.first.strName;
        CAmount nAmount = transfer.first.nAmount;

        if (!IsValidDestinationString(address)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address);
            return false;
        }

        if (!passets) {
            error = std::make_pair(RPC_DATABASE_ERROR, std::string("passets isn't initialized"));
            return false;
        }

        if (!VerifyWalletHasAsset(asset_name, error)) // Sets error if it fails
            return false;

        // If it is an ownership transfer, make a quick check to make sure the amount is 1
        if (IsAssetNameAnOwner(asset_name)) {
            if (nAmount != OWNER_ASSET_AMOUNT) {
                error = std::make_pair(RPC_INVALID_PARAMS, std::string(
                        "When transfer an 'Ownership Asset' the amount must always be 1. Please try again with the amount of 1"));
                return false;
            }
        }

        // Get the script for the burn address
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(address));

        // Update the scriptPubKey with the transfer asset information
        CAssetTransfer assetTransfer(asset_name, nAmount);
        assetTransfer.ConstructTransaction(scriptPubKey);

        CRecipient recipient = {scriptPubKey, 0, fSubtractFeeFromAmount};
        vecSend.push_back(recipient);
    }

    // Create and send the transaction
    if (!pwallet->CreateTransactionWithTransferAsset(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl)) {
        if (!fSubtractFeeFromAmount && nFeeRequired > curBalance) {
            error = std::make_pair(RPC_WALLET_ERROR, strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired)));
            return false;
        }
        error = std::make_pair(RPC_TRANSACTION_ERROR, strTxError);
        return false;
    }
    return true;
}

bool SendAssetTransaction(CWallet* pwallet, CWalletTx& transaction, CReserveKey& reserveKey, std::pair<int, std::string>& error, std::string& txid)
{
    CValidationState state;
    if (!pwallet->CommitTransaction(transaction, reserveKey, g_connman.get(), state)) {
        error = std::make_pair(RPC_WALLET_ERROR, strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason()));
        return false;
    }

    txid = transaction.GetHash().GetHex();
    return true;
}

bool VerifyWalletHasAsset(const std::string& asset_name, std::pair<int, std::string>& pairError)
{
    CWallet* pwallet;
    if (vpwallets.size() > 0)
        pwallet = vpwallets[0];
    else {
        pairError = std::make_pair(RPC_WALLET_ERROR, strprintf("Wallet not found. Can't verify if it contains: %s", asset_name));
        return false;
    }

    std::vector<COutput> vCoins;
    std::map<std::string, std::vector<COutput> > mapAssetCoins;
    pwallet->AvailableAssets(mapAssetCoins);

    if (mapAssetCoins.count(asset_name))
        return true;

    pairError = std::make_pair(RPC_INVALID_REQUEST, strprintf("Wallet doesn't have asset: %s", asset_name));
    return false;
}

// Return true if the amount is valid with the units passed in
bool CheckAmountWithUnits(const CAmount& nAmount, const uint8_t nUnits)
{
    return nAmount % int64_t(pow(10, (MAX_UNIT - nUnits))) == 0;
}