// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "server.h"

#include "sync.h"

#include <map>

static std::map<const void*, Consensus::CVersionBitsState*> mVersionBitsCache;
static CCriticalSection cs_versionbits;

/**
 * The key is the pointer of the CBlockIndex and the value, the corresponding Consensus::CVersionBitsState.
 */
void ConcurrentVersionBitsCacheSetter(const void* blockIndex, Consensus::CVersionBitsState* versionBitsState)
{
    LOCK(cs_versionbits);
    mVersionBitsCache[blockIndex] = versionBitsState;
}

const Consensus::CVersionBitsState* ConcurrentVersionBitsCacheGetter(const void* blockIndex)
{
    LOCK(cs_versionbits);
    const Consensus::CVersionBitsState* cachedState = NULL;
    if (mVersionBitsCache.count(blockIndex))
        cachedState = mVersionBitsCache.at(blockIndex);
    return cachedState;
}

CVersionBitsCacheImplementation::CVersionBitsCacheImplementation()
{
    Set = ConcurrentVersionBitsCacheSetter;
    Get = ConcurrentVersionBitsCacheGetter;
}
    
CVersionBitsCacheImplementation::~CVersionBitsCacheImplementation()
{
    // TODO mVersionBitsCache singleton?
}

namespace Global {

CVersionBitsCacheImplementation versionBitsStateCache;

} // namespace Global
