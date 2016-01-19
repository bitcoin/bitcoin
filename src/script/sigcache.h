// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGCACHE_H
#define BITCOIN_SCRIPT_SIGCACHE_H

#include "script/interpreter.h"
#include "sync.h"

#include <vector>

// DoS prevention: limit cache size to less than 40MB (over 500000
// entries on 64-bit systems).
static const unsigned int DEFAULT_MAX_SIG_CACHE_SIZE = 40;

class CPubKey;

/**
 * A thread safe map which caches mid state witness signature hash calculation by transaction id
 */
class CachedHashesMap
{
private:
    std::map<uint256, CachedHashes> map;
    CCriticalSection cs;
public:
    bool TryGet(uint256 txId, CachedHashes* hashes)
    {
        LOCK(cs);
        if(!map.count(txId))
            return false;
        *hashes = map[txId];
        return true;
    }
    bool TrySet(uint256 txId, const CachedHashes& hashes)
    {
        LOCK(cs);
        if(map.count(txId))
            return false;
        map[txId] = hashes;
        return true;
    }
};

class CachingTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    bool store;

public:
    CachingTransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amount, bool storeIn, CachedHashes& cachedHashesIn) : TransactionSignatureChecker(txToIn, nInIn, amount, cachedHashesIn), store(storeIn) {}

    bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;
};

#endif // BITCOIN_SCRIPT_SIGCACHE_H
