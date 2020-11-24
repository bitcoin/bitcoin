// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSET_H
#define SYSCOIN_SERVICES_ASSET_H
#include <primitives/transaction.h>
#include <pubkey.h>
class UniValue;
static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const unsigned int MAX_SYMBOL_SIZE = 12; // up to 9 characters
static const unsigned int MIN_SYMBOL_SIZE = 4;
static const unsigned int MAX_AUXFEES = 10;
// this should be set well after mempool expiry (DEFAULT_MEMPOOL_EXPIRY)
// so that miner will expire txs before they hit expiry errors if for some reason they aren't getting mined (geth issue etc)
static const int64_t MAINNET_MAX_MINT_AGE = 302400; // 0.5 week in seconds, should send to network in half a week or less
static const int64_t MAINNET_MAX_VALIDATE_AGE = 1512000; // 2.5 weeks in seconds
static const int64_t MAINNET_MIN_MINT_AGE = 3600; // 1 hr
static const uint32_t DOWNLOAD_ETHEREUM_TX_ROOTS = 60000; // roughly 1 week
static const uint32_t MAX_ETHEREUM_TX_ROOTS = DOWNLOAD_ETHEREUM_TX_ROOTS*4;
uint32_t GenerateSyscoinGuid(const COutPoint& outPoint);
std::string stringFromSyscoinTx(const int &nVersion);
std::string assetFromTx(const int &nVersion);
enum {
    ASSET_UPDATE_DATA=1, // can you update public data field?
    ASSET_UPDATE_CONTRACT=2, // can you update smart contract?
    ASSET_UPDATE_SUPPLY=4, // can you update supply?
    ASSET_UPDATE_NOTARY_KEY=8, // can you update notary?
    ASSET_UPDATE_NOTARY_DETAILS=16, // can you update notary details?
    ASSET_UPDATE_AUXFEE=32, // can you update aux fees?
    ASSET_UPDATE_CAPABILITYFLAGS=64, // can you update capability flags?
    ASSET_INIT=128, // set when creating an asset
    ASSET_CAPABILITY_ALL=255
};
class CAuxFee {
public:
    CAmount nBound;
    uint16_t nPercent;
    CAuxFee() {
        SetNull();
    }
    CAuxFee(const CAmount &nBoundIn, const uint16_t &nPercentIn):nBound(nBoundIn),nPercent(nPercentIn) {}
    SERIALIZE_METHODS(CAuxFee, obj) {
        READWRITE(Using<AmountCompression>(obj.nBound), obj.nPercent);
    }
    inline friend bool operator==(const CAuxFee &a, const CAuxFee &b) {
        return (
        a.nBound == b.nBound && a.nPercent == b.nPercent
        );
    }

    inline friend bool operator!=(const CAuxFee &a, const CAuxFee &b) {
        return !(a == b);
    }
    inline void SetNull() { nPercent = 0; nBound = 0;}
    inline bool IsNull() const { return (nPercent == 0 && nBound == 0); }
};
class CAuxFeeDetails {
public:
    std::vector<unsigned char> vchAuxFeeKeyID;
    std::vector<CAuxFee> vecAuxFees;
    CAuxFeeDetails() {
        SetNull();
    }
    CAuxFeeDetails(const UniValue& value, const uint8_t &nPrecision);
    SERIALIZE_METHODS(CAuxFeeDetails, obj) {
        READWRITE(obj.vchAuxFeeKeyID, obj.vecAuxFees);
    }
    inline friend bool operator==(const CAuxFeeDetails &a, const CAuxFeeDetails &b) {
        return (
        a.vecAuxFees == b.vecAuxFees && a.vchAuxFeeKeyID == b.vchAuxFeeKeyID
        );
    }

    inline friend bool operator!=(const CAuxFeeDetails &a, const CAuxFeeDetails &b) {
        return !(a == b);
    }
    inline void SetNull() { vecAuxFees.clear(); vchAuxFeeKeyID.clear();}
    inline bool IsNull() const { return (vecAuxFees.empty() && vchAuxFeeKeyID.empty()); }
    UniValue ToJson() const;
};
class CNotaryDetails {
public:
    std::string strEndPoint;
    uint8_t bEnableInstantTransfers;
    uint8_t bRequireHD;
    CNotaryDetails() {
        SetNull();
    }
    CNotaryDetails(const UniValue& value);
    SERIALIZE_METHODS(CNotaryDetails, obj) {
        READWRITE(obj.strEndPoint, obj.bEnableInstantTransfers, obj.bRequireHD);
    }
    inline friend bool operator==(const CNotaryDetails &a, const CNotaryDetails &b) {
        return (
        a.strEndPoint == b.strEndPoint && a.bEnableInstantTransfers == b.bEnableInstantTransfers && a.bRequireHD == b.bRequireHD
        );
    }

    inline friend bool operator!=(const CNotaryDetails &a, const CNotaryDetails &b) {
        return !(a == b);
    }
    inline void SetNull() { strEndPoint.clear();  bEnableInstantTransfers = bRequireHD = 0;}
    inline bool IsNull() const { return strEndPoint.empty(); }
    UniValue ToJson() const;
};

class CAsset: public CAssetAllocation {
public:
    std::vector<unsigned char> vchContract;
    std::vector<unsigned char> vchPrevContract;
    std::string strSymbol;
    std::string strPubData;
    std::string strPrevPubData;
    CAmount nTotalSupply;
    CAmount nMaxSupply;
    uint8_t nPrecision;
    uint8_t nUpdateCapabilityFlags;
    uint8_t nPrevUpdateCapabilityFlags;
    std::vector<unsigned char> vchNotaryKeyID;
    std::vector<unsigned char> vchPrevNotaryKeyID;
    CNotaryDetails notaryDetails;
    CNotaryDetails prevNotaryDetails;
    CAuxFeeDetails auxFeeDetails;
    CAuxFeeDetails prevAuxFeeDetails;
    uint8_t nUpdateMask;
    CAsset() {
        SetNull();
    }
    explicit CAsset(const CTransaction &tx);
    
    inline void ClearAsset() {
        strPubData.clear();
        vchContract.clear();
        voutAssets.clear();
        strPrevPubData.clear();
        vchPrevContract.clear();
        strSymbol.clear();
        nPrevUpdateCapabilityFlags = nUpdateCapabilityFlags = 0;
        nTotalSupply = 0;
        nMaxSupply = 0;
        vchNotaryKeyID.clear();
        vchPrevNotaryKeyID.clear();
        notaryDetails.SetNull();
        prevNotaryDetails.SetNull();
        auxFeeDetails.SetNull();
        prevAuxFeeDetails.SetNull();
        nUpdateMask = 0;
    }

    SERIALIZE_METHODS(CAsset, obj) {
        READWRITEAS(CAssetAllocation, obj);
        READWRITE(obj.nPrecision, obj.nUpdateMask);
        if(obj.nUpdateMask & ASSET_INIT) {
            READWRITE(obj.strSymbol, Using<AmountCompression>(obj.nMaxSupply));
        }
        if(obj.nUpdateMask & ASSET_UPDATE_CONTRACT) {
            READWRITE(obj.vchContract);
            READWRITE(obj.vchPrevContract);
        }
        if(obj.nUpdateMask & ASSET_UPDATE_DATA) {
            READWRITE(obj.strPubData);
            READWRITE(obj.strPrevPubData);
        }
        if(obj.nUpdateMask & ASSET_UPDATE_SUPPLY) {
            READWRITE(Using<AmountCompression>(obj.nTotalSupply));
        }
        if(obj.nUpdateMask & ASSET_UPDATE_NOTARY_KEY) {
            READWRITE(obj.vchNotaryKeyID);
            READWRITE(obj.vchPrevNotaryKeyID);
        }
        if(obj.nUpdateMask & ASSET_UPDATE_NOTARY_DETAILS) {
            READWRITE(obj.notaryDetails);
            READWRITE(obj.prevNotaryDetails);
        }
        if(obj.nUpdateMask & ASSET_UPDATE_AUXFEE) {
            READWRITE(obj.auxFeeDetails);
            READWRITE(obj.prevAuxFeeDetails);
        }
        if(obj.nUpdateMask & ASSET_UPDATE_CAPABILITYFLAGS) {
            READWRITE(obj.nUpdateCapabilityFlags);
            READWRITE(obj.nPrevUpdateCapabilityFlags);
        }
    }

    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
        a.voutAssets == b.voutAssets
        );
    }


    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
    // set precision to an invalid amount so isnull will identity this asset as invalid state
    inline void SetNull() { ClearAsset(); nPrecision = 9; }
    inline bool IsNull() const { return nPrecision == 9; }
    bool UnserializeFromTx(const CTransaction &tx);
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    void SerializeData(std::vector<unsigned char>& vchData);
};
static CAsset emptyAsset;
bool GetAsset(const uint32_t &nAsset,CAsset& txPos);
bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nAsset, CAssetsMap mapAssetIn, const CAssetsMap &mapAssetOut);
#endif // SYSCOIN_SERVICES_ASSET_H
