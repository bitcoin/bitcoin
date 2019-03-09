// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCDUMP_H
#define BITCOIN_WALLET_RPCDUMP_H

#include <wallet/wallet.h>

class UniValue;

UniValue ProcessImport(CWallet * const pwallet, const UniValue& data, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet);

#endif //BITCOIN_WALLET_RPCDUMP_H
