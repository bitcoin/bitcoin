// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSET_H
#define SYSCOIN_SERVICES_ASSET_H
#include <amount.h>
class UniValue;
class CTransaction;
class TxValidationState;
class COutPoint;
class CAsset;

static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const unsigned int MAX_SYMBOL_SIZE = 12; // up to 9 characters
static const unsigned int MIN_SYMBOL_SIZE = 4;
static const unsigned int MAX_AUXFEES = 10;
uint32_t GenerateSyscoinGuid(const COutPoint& outPoint);
std::string stringFromSyscoinTx(const int &nVersion);
std::string assetFromTx(const int &nVersion);
static CAsset emptyAsset;
bool GetAsset(const uint32_t &nBaseAsset,CAsset& txPos);
bool ExistsNFTAsset(const uint64_t &nAsset);
bool GetAssetPrecision(const uint32_t &nBaseAsset, uint8_t& nPrecision);
bool GetAssetNotaryKeyID(const uint32_t &nBaseAsset, std::vector<unsigned char>& keyID);
bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nBaseAsset, CAssetsMap mapAssetIn, const CAssetsMap &mapAssetOut);
UniValue AssetPublicDataToJson(const std::string &strPubData);
uint32_t GetBaseAssetID(const uint64_t &nAsset);
uint32_t GetNFTID(const uint64_t &nAsset);
uint64_t CreateAssetID(const uint32_t &NFTID, const uint32_t &nBaseAsset);
bool FillNotarySig(std::vector<CAssetOut> & voutAssets, const uint64_t& nBaseAsset, const std::vector<unsigned char> &vchSig);
#endif // SYSCOIN_SERVICES_ASSET_H
