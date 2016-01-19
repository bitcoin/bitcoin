// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_CACHEDHASHESMAP_H
#define SRC_CACHEDHASHESMAP_H


#include "script/interpreter.h"
#include "sync.h"

class CachedHashes;
/**
 * A thread safe map which caches mid state witness signature hash calculation by transaction id
 */
class CachedHashesMap
{
private:
    std::map<uint256, CachedHashes> map;
    CCriticalSection cs;
public:
    bool TryGet(const uint256& txId, CachedHashes* hashes);
    void TrySet(const uint256& txId, const CachedHashes& hashes);
};


#endif //SRC_CACHEDHASHESMAP_H
