// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
#define SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
#include <string>
#include <amount.h>
class CAssetCoinInfo;
class UniValue;
class CWalletTx;
bool SysWtxToJSON(const CWalletTx& wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue& output);
bool ListTransactionSyscoinInfo(const CWalletTx& wtx, const CAssetCoinInfo assetInfo, const std::string strCategory, UniValue& output);
bool AssetWtxToJSON(const CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry); 
bool AssetMintWtxToJson(const CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
bool AssetAllocationWtxToJSON(const CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
bool AllocationWtxToJson(const CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
#endif // SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
