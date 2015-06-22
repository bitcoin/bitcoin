// Provides a cache of wallet balances and functionality for determining whether Omni state changes affected anything in the wallet
#include "omnicore/omnicore.h"

#include "init.h"
#include "wallet.h"

#include <boost/algorithm/string.hpp>

#include <map>
#include <string>

using std::map;
using std::string;

using boost::algorithm::token_compress_on;

using namespace mastercore;

std::map<string, CMPTally> walletBalancesCache;

//! Global vector of Omni transactions in the wallet
std::vector<uint256> walletTXIDCache;

/**
 * Adds a txid to the wallet txid cache, performing duplicate detection
 */
void WalletTXIDCacheAdd(uint256 hash)
{
    if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: Adding tx to txid cache : %s\n",hash.GetHex());
    if (std::find(walletTXIDCache.begin(), walletTXIDCache.end(), hash) != walletTXIDCache.end()) {
        PrintToLog("ERROR: Wallet TXID Cache blocked duplicate insertion for %s\n",hash.GetHex());
    } else {
        walletTXIDCache.push_back(hash);
    }
}

/**
 * Performs initial population of the wallet txid cache
 */
void WalletTXIDCacheInit()
{
    if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: WalletTXIDCacheInit requested\n");
    // Prepare a few items and get lock
    CWallet *wallet = pwalletMain;
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // STO has no inbound transaction, use STO receipts to insert an STO into the wallet txid cache
    string mySTOReceipts = s_stolistdb->getMySTOReceipts("");
    std::vector<std::string> vecReceipts;
    boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), token_compress_on);
    int64_t lastTXBlock = 999999;

    // Iterate through the wallet, checking if each transaction is Omni (via levelDB)
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0) {
            uint256 blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) continue;
            CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;

            // look for an STO receipt to see if we need to insert it into txid cache
            for(uint32_t i = 0; i<vecReceipts.size(); i++) {
                if (!vecReceipts[i].empty()) {
                    std::vector<std::string> svstr;
                    boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), token_compress_on);
                    if (svstr.size() != 4) {
                        PrintToLog("ERROR: Unexpected number of tokens enumerating STO receipt %s\n", vecReceipts[i]);
                        continue;
                    }
                    if ((atoi(svstr[1]) < lastTXBlock) && (atoi(svstr[1]) > blockHeight)) {
                        uint256 hash;
                        hash.SetHex(svstr[0]);
                        if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: Adding STO to txid cache : %s\n",hash.GetHex());
                        walletTXIDCache.push_back(hash);
                    }
                }
            }

            // get the hash of the transaction and check leveldb to see if this is an Omni tx, if so add to cache
            uint256 hash = pwtx->GetHash();
            if (p_txlistdb->exists(hash)) {
                walletTXIDCache.push_back(hash);
                if (msc_debug_walletcache) PrintToLog("WALLETTXIDCACHE: Adding tx to txid cache : %s\n",hash.GetHex());
            }
            lastTXBlock = blockHeight;
        }
    }
}

/**
 * Updates the cache with the latest state, returning true if changes were made to wallet addresses (including watch only)
 * Also prepares a list of addresses that were changed (for future usage)
 */
int WalletCacheUpdate()
{
    if (msc_debug_walletcache) PrintToLog("WALLETCACHE: Update requested\n");
    int numChanges = 0;
    std::set<std::string> changedAddresses;

    LOCK(cs_tally);

    for(std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
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

