#ifndef OMNICORE_WALLETCACHE_H
#define OMNICORE_WALLETCACHE_H

class uint256;

#include <vector>

namespace mastercore
{
//! Global vector of Omni transactions in the wallet
extern std::vector<uint256> walletTXIDCache;

/** Adds a txid to the wallet txid cache, performing duplicate detection */
void WalletTXIDCacheAdd(const uint256& hash);

/** Performs initial population of the wallet txid cache */
void WalletTXIDCacheInit();

/** Updates the cache and returns whether any wallet addresses were changed */
int WalletCacheUpdate();
}

#endif // OMNICORE_WALLETCACHE_H
