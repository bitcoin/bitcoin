// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LEGACYWALLET_H
#define BITCOIN_LEGACYWALLET_H

#include "wallet/wallet.h"

extern CWallet* pwalletMain;

namespace CLegacyWalletModule {
    void RegisterSignals();
    void UnregisterSignals();
    
    std::string GetWalletFile();
};

#endif // BITCOIN_LEGACYWALLET_H
