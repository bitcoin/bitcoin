#ifndef AESCACHE_H
#define AESCACHE_H

#include "uint256.h"
#include "leveldbwrapper.h"

class CBlockAesCache : public CLevelDBWrapper
{
  public:
    CBlockAesCache(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
  private:
    CBlockAesCache(const CBlockAesCache&);
    void operator=(const CBlockAesCache&);
  public:
    bool WriteHash(const uint256 &hash, const uint256 &aesHash);
    bool ReadHash(const uint256 &hash, uint256 &aesHash);
};

#endif // AESCACHE_H
