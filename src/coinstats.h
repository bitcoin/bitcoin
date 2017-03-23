// Copyright (c) 2014-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSTATS_H
#define BITCOIN_COINSTATS_H

#include "coins.h"
#include "txdb.h"
#include "coinsbyscript.h"

struct CCoinsStats
{
    int nHeight{};
    uint256 hashBlock;
    uint64_t nTransactions{};
    uint64_t nTransactionOutputs{};
    uint64_t nAddresses{};
    uint64_t nAddressesOutputs{}; // equal nTransactionOutputs (if addressindex is enabled)
    uint64_t nSerializedSize{};
    uint256 hashSerialized;
    CAmount nTotalAmount{};

    CCoinsStats() {}
};

bool GetUTXOStats(CCoinsView *view, CCoinsViewByScriptDB *viewbyscriptdb, CCoinsStats &stats);

#endif // BITCOIN_COINSTATS_H
