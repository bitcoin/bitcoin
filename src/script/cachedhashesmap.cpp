// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cachedhashesmap.h"

bool CachedHashesMap::TryGet(const uint256& txId, CachedHashes* hashes)
{
    LOCK(cs);
    auto iter = map.find(txId);
    if (iter == map.end())
        return false;
    *hashes = iter->second;
    return true;
}

void CachedHashesMap::TrySet(const uint256& txId, const CachedHashes& hashes)
{
    LOCK(cs);
    map[txId].Merge(hashes);
}
