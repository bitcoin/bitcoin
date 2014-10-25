// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINRPC_WALLET_H_
#define _BITCOINRPC_WALLET_H_ 1

#include "wallet.h"
#include "walletdb.h"

#include <stdint.h>

CAmount GetAccountBalance(CWalletDB& walletdb, const std::string& strAccount, int nMinDepth, const isminefilter& filter);
CAmount GetAccountBalance(const std::string& strAccount, int nMinDepth, const isminefilter& filter);

#endif
