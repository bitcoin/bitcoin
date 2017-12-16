#ifndef OMNICORE_WALLETCACHE_H
#define OMNICORE_WALLETCACHE_H

class uint256;

#include <vector>

namespace mastercore
{
/** Updates the cache and returns whether any wallet addresses were changed */
int WalletCacheUpdate();
}

#endif // OMNICORE_WALLETCACHE_H
