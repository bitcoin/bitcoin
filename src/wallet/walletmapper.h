// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETMAPPER_H
#define BITCOIN_WALLET_WALLETMAPPER_H

#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "streams.h"

class CWalletMapper {
private:
    CWallet *pwallet;
public:
    CWalletMapper(CWallet *pWalletIn) : pwallet(pWalletIn) {}
    bool ReadKeyValue(CDataStream& ssKey, CDataStream& ssValue, CWalletScanState &wss, std::string& strType, std::string& strErr, uint64_t &accountingEntryNumberRef);
};
#endif // BITCOIN_WALLET_WALLETDB_H
