// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <relay.h>

void RelayTransaction(const uint256& txid, CConnman* connman)
{
    connman.ForEachNode([&txid](CNode* node) {
        node->PushTransactionInventory(txid);
    });
}
