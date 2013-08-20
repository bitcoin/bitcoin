// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "core.h"
#include "wallet.h"

/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(CReserveKey& reservekey);

#endif // BITCOIN_MINER_H
