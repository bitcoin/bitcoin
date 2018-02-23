// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/MIT.

#ifndef BITCOIN_WALLET_UTIL_H
#define BITCOIN_WALLET_UTIL_H

#include <chainparamsbase.h>
#include <util.h>

//! Get the path of the wallet directory.
fs::path GetWalletDir();

#endif // BITCOIN_WALLET_UTIL_H
