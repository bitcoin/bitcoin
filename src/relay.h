// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RELAY_H
#define BITCOIN_RELAY_H

#include <net.h>
#include <uint256.h>

/** Announce transaction to all peers */
void RelayTransaction(const uint256& txid, CConnman* connman);

#endif // BITCOIN_RELAY_H
