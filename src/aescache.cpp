#include "aescache.h"

CBlockAesCache *aesCache = NULL;

CBlockAesCache::CBlockAesCache(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "blocks" / "cache", nCacheSize, fMemory, fWipe) {
}

bool CBlockAesCache::WriteHash(const uint256 &hash, const uint256 &aesHash)
{
    return Write(std::make_pair('y', hash), aesHash);
}

bool CBlockAesCache::ReadHash(const uint256 &hash, uint256 &aesHash)
{
    return Read(std::make_pair('y', hash), aesHash);
}
