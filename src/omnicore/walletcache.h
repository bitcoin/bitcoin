#ifndef OMNICORE_WALLETCACHE_H
#define OMNICORE_WALLETCACHE_H

#include <string>

/** Adds a txid to the wallet txid cache, performing duplicate detection */
void WalletTXIDCacheAdd(uint256 hash);

/** Performs initial population of the wallet txid cache */
void WalletTXIDCacheInit();

/** Updates the cache and returns whether any wallet addresses were changed */
int WalletCacheUpdate();

//! Global vector of Omni transactions in the wallet
extern std::vector<uint256> walletTXIDCache;
#endif // OMNICORE_WALLETCACHE_H


