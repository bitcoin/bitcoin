// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STXO_H
#define BITCOIN_STXO_H

#include <set>

class CBlockIndex;
class COutPoint;

class CSTXO
{
private:
    std::set<COutPoint> setSpentCoins; // spent transaction outputs
public:

    CSTXO(CBlockIndex* pIndexBest, unsigned int nNumberOfBlocks);

    bool IsRecentlySpent(const COutPoint& outpoint) const {
        return (setSpentCoins.count(outpoint) == 1);
    }
};

#endif // BITCOIN_STXO_H
