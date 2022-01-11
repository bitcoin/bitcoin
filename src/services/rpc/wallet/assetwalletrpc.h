// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
#define SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
#include <string>
#include <consensus/amount.h>
class CAssetCoinInfo;
class UniValue;
namespace wallet { 
class CWalletTx;
}
bool SysWtxToJSON(const wallet::CWalletTx& wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue& output);
bool ListTransactionSyscoinInfo(const wallet::CWalletTx& wtx, const CAssetCoinInfo assetInfo, const std::string strCategory, UniValue& output);
bool AssetWtxToJSON(const wallet::CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry); 
bool AssetMintWtxToJson(const wallet::CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
bool AssetAllocationWtxToJSON(const wallet::CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
bool AllocationWtxToJson(const wallet::CWalletTx &wtx, const CAssetCoinInfo &assetInfo, const std::string &strCategory, UniValue &entry);
#endif // SYSCOIN_SERVICES_RPC_WALLET_ASSETWALLETRPC_H
