// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stxo.h"
#include "main.h"

CSTXO::CSTXO(CBlockIndex* pIndexBest, unsigned int nNumberOfBlocks)
{
    unsigned int i = 0;
    CBlockIndex* pindex = pIndexBest;
    while (pindex && pindex->pprev)
    {
        CBlock block;
        ReadBlockFromDisk(block, pindex);
        BOOST_FOREACH(const CTransaction& tx, block.vtx)
        {
            if (!tx.IsCoinBase())
            {
                BOOST_FOREACH(const CTxIn& txin, tx.vin)
                    setSpentCoins.insert(txin.prevout);
            }
        }

        pindex = pindex->pprev;
        if (++i == nNumberOfBlocks)
            break;
    }
}
