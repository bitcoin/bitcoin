// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_COREWALLET_H
#define BITCOIN_COREWALLT_COREWALLET_H

#include "corewallet/corewallet_wallet.h"

namespace CoreWallet {
    void RegisterSignals();
    void UnregisterSignals();
    
    CoreWallet::Wallet* GetWallet();
};

#endif // BITCOIN_COREWALLT_COREWALLET_H
