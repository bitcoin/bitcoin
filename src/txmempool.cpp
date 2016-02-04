// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"

#include "core.h"
#include "main.h" // for CTransaction

using namespace std;

bool CTxMemPool::addUnchecked(const uint256& hash, CTransaction &tx)
{
    // Add to memory pool without checking anything.  Don't call this directly,
    // call CTxMemPool::accept to properly check the transaction first.
    {
        mapTx[hash] = tx;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            mapNextTx[tx.vin[i].prevout] = CInPoint(&mapTx[hash], i);
        nTransactionsUpdated++;
    }
    return true;
}


bool CTxMemPool::remove(const CTransaction &tx, bool fRecursive)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        uint256 hash = tx.GetHash();
        if (mapTx.count(hash))
        {
            if (fRecursive)
            {
                for (unsigned int i = 0; i < tx.vout.size(); i++)
                {
                    std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                    if (it != mapNextTx.end())
                        remove(*it->second.ptx, true);
                };
            };
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
                mapNextTx.erase(txin.prevout);
            mapTx.erase(hash);
            
            if (tx.nVersion == ANON_TXN_VERSION)
            {
                // -- remove key images
                for (unsigned int i = 0; i < tx.vin.size(); ++i)
                {
                    const CTxIn& txin = tx.vin[i];

                    if (!txin.IsAnonInput())
                        continue;
                    
                    ec_point vchImage;
                    txin.ExtractKeyImage(vchImage);
                    
                    mapKeyImage.erase(vchImage);
                };
            };
            
            nTransactionsUpdated++;
        };
    }
    return true;
}

bool CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    LOCK(cs);
    BOOST_FOREACH(const CTxIn &txin, tx.vin)
    {
        std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end())
        {
            const CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
                remove(txConflict, true);
        };
    };
    return true;
}

void CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    mapKeyImage.clear();
    ++nTransactionsUpdated;
}

void CTxMemPool::queryHashes(std::vector<uint256>& vtxid)
{
    vtxid.clear();

    LOCK(cs);
    vtxid.reserve(mapTx.size());
    for (map<uint256, CTransaction>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back((*mi).first);
}

bool CTxMemPool::lookup(uint256 hash, CTransaction& result) const
{
    LOCK(cs);
    std::map<uint256, CTransaction>::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return false;
    result = i->second;
    return true;
}
