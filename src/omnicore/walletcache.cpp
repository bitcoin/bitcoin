/**
 * @file walletcache.cpp
 *
 * Provides a cache of wallet balances and functionality for determining whether
 * Omni state changes affected anything in the wallet.
 */

#include "omnicore/walletcache.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/tally.h"
#include "omnicore/wallettxs.h"

#include "init.h"
#include "sync.h"
#include "uint256.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <stdint.h>
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mastercore
{
//! Global vector of Omni transactions in the wallet
std::vector<uint256> walletTXIDCache;

//! Map of wallet balances
static std::map<std::string, CMPTally> walletBalancesCache;

/**
 * Adds a txid to the wallet txid cache, performing duplicate detection.
 */
void WalletTXIDCacheAdd(const uint256& hash)
{
    if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: Adding tx to txid cache : %s\n", hash.GetHex());
    if (std::find(walletTXIDCache.begin(), walletTXIDCache.end(), hash) != walletTXIDCache.end()) {
        PrintToLog("ERROR: Wallet TXID Cache blocked duplicate insertion for %s\n", hash.GetHex());
    } else {
        walletTXIDCache.push_back(hash);
    }
}

/**
 * Performs initial population of the wallet txid cache.
 */
void WalletTXIDCacheInit()
{
    if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: WalletTXIDCacheInit requested\n");
#ifdef ENABLE_WALLET
    LOCK2(cs_tally, pwalletMain->cs_wallet);

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->wtxOrdered;

    // Iterate through the wallet, checking if each transaction is Omni (via levelDB)
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        const CWalletTx* pwtx = it->second.first;
        if (pwtx != NULL) {
            // get the hash of the transaction and check leveldb to see if this is an Omni tx, if so add to cache
            const uint256& hash = pwtx->GetHash();
            if (p_txlistdb->exists(hash)) {
                walletTXIDCache.push_back(hash);
                if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: Adding tx to txid cache : %s\n", hash.GetHex());
            }
        }
    }
#endif
}

/**
 * Updates the cache with the latest state, returning true if changes were made to wallet addresses (including watch only).
 *
 * Also prepares a list of addresses that were changed (for future usage).
 */
int WalletCacheUpdate()
{
    if (msc_debug_walletcache) PrintToLog("WALLETCACHE: Update requested\n");
    int numChanges = 0;
    std::set<std::string> changedAddresses;

    LOCK(cs_tally);

    for (std::unordered_map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        const std::string& address = my_it->first;

        // determine if this address is in the wallet
        int addressIsMine = IsMyAddress(address);
        if (!addressIsMine) {
            if (msc_debug_walletcache) PrintToLog("WALLETCACHE: Ignoring non-wallet address %s\n", address);
            continue; // ignore this address, not in wallet
        }

        // obtain & init the tally
        CMPTally& tally = my_it->second;
        tally.init();

        // check cache for miss on address
        std::map<std::string, CMPTally>::iterator search_it = walletBalancesCache.find(address);
        if (search_it == walletBalancesCache.end()) { // cache miss, new address
            ++numChanges;
            changedAddresses.insert(address);
            walletBalancesCache.insert(std::make_pair(address,tally));
            if (msc_debug_walletcache) PrintToLog("WALLETCACHE: *CACHE MISS* - %s not in cache\n", address);
            continue;
        }

        // check cache for miss on balance - TODO TRY AND OPTIMIZE THIS
        CMPTally &cacheTally = search_it->second;
        uint32_t propertyId;
        while (0 != (propertyId = (tally.next()))) {
            if (tally.getMoney(propertyId, BALANCE) != cacheTally.getMoney(propertyId, BALANCE) ||
                    tally.getMoney(propertyId, PENDING) != cacheTally.getMoney(propertyId, PENDING) ||
                    tally.getMoney(propertyId, SELLOFFER_RESERVE) != cacheTally.getMoney(propertyId, SELLOFFER_RESERVE) ||
                    tally.getMoney(propertyId, ACCEPT_RESERVE) != cacheTally.getMoney(propertyId, ACCEPT_RESERVE) ||
                    tally.getMoney(propertyId, METADEX_RESERVE) != cacheTally.getMoney(propertyId, METADEX_RESERVE)) { // cache miss, balance
                ++numChanges;
                changedAddresses.insert(address);
                walletBalancesCache.erase(search_it);
                walletBalancesCache.insert(std::make_pair(address,tally));
                if (msc_debug_walletcache) PrintToLog("WALLETCACHE: *CACHE MISS* - %s balance for property %d differs\n", address, propertyId);
                break;
            }
        }
    }
    if (msc_debug_walletcache) PrintToLog("WALLETCACHE: Update finished - there were %d changes\n", numChanges);
    return numChanges;
}


} // namespace mastercore
