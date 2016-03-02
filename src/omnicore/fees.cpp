/**
 * @file fees.cpp
 *
 * This file contains code for handling Omni fees.
 */

#include "omnicore/fees.h"

#include "omnicore/omnicore.h"
#include "omnicore/log.h"

#include "leveldb/db.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// Arbitrary value used for testing - TODO: how are we calculating the threshold for distribution?
int64_t feeDistributionThreshold = 1000000;

// Gets the current amount of the fee cache for a property
int64_t COmniFeeCache::GetCachedAmount(const uint32_t &propertyId)
{
    assert(pdb);

    // Get the fee history, set is sorted by block so last entry is most recent
    std::set<feeCacheItem> sCacheHistoryItems = GetCacheHistory(propertyId);
    if (!sCacheHistoryItems.empty()) {
        std::set<feeCacheItem>::iterator endIt = sCacheHistoryItems.end();
        feeCacheItem mostRecentItem = *endIt;
        return mostRecentItem.second;
    } else {
        return 0; // property has never generated a fee
    }
}

// Adds a fee to the cache (eg on a completed trade)
void COmniFeeCache::AddFee(const uint32_t &propertyId, int block, const uint64_t &amount)
{
    // Get current cached fee
    int64_t currentCachedAmount = GetCachedAmount(propertyId);

    // Add new fee and rewrite record
    int64_t newCachedAmount = currentCachedAmount + amount; // TODO: should be some overflow detection in here
    const std::string key = strprintf("%010d", propertyId);
    std::string oldValue;
    leveldb::Status getStatus = pdb->Get(readoptions, key, &oldValue);
    assert(getStatus.ok());
    if (!oldValue.empty()) oldValue=+",";
    const std::string newValue = strprintf("%s%d:%d", oldValue, block, newCachedAmount);
    leveldb::Status putStatus = pdb->Put(writeoptions, key, newValue);
    assert(putStatus.ok());
    if (msc_debug_fees) PrintToLog("Adding fee %d to property %d (old=%s [%s], new=%s [%s])\n", amount, propertyId, oldValue, newValue, getStatus.ToString(), putStatus.ToString());
    ++nWritten;

    // Call for pruning (we only prune when we update a record)
    PruneCache(propertyId, block);

    return;
}

// Rolls back the cache to an earlier state (eg in event of a reorg)

// Evaluates fee caches for all properties against threshold and executes distribution if threshold met

// Performs distribution of fees

// Prunes entries over 50 blocks old from the entry for a property
void COmniFeeCache::PruneCache(const uint32_t &propertyId, int block)
{
    assert(pdb);

    int pruneBlock = block-50;
    const std::string key = strprintf("%010d", propertyId);
    std::set<feeCacheItem> sCacheHistoryItems = GetCacheHistory(propertyId);
    if (!sCacheHistoryItems.empty()) {
        std::set<feeCacheItem>::iterator startIt = sCacheHistoryItems.begin();
        feeCacheItem firstItem = *startIt;
        if (firstItem.first >= pruneBlock) return; // all entries are above supplied block value, nothing to do
        std::string newValue;
        for (std::set<feeCacheItem>::iterator it = sCacheHistoryItems.begin(); it != sCacheHistoryItems.end(); it++) {
            feeCacheItem tempItem = *it;
            if (tempItem.first < pruneBlock) continue; // discard this entry
            if (!newValue.empty()) newValue += ",";
            newValue += strprintf("%d:%d", tempItem.first, tempItem.second);
        }
        // make sure the pruned cache isn't completely empty, if it is, prune down to just the most recent entry
        if (newValue.empty()) {
            std::set<feeCacheItem>::iterator mostRecentIt = sCacheHistoryItems.end();
            feeCacheItem mostRecentItem = *mostRecentIt;
            newValue = strprintf("%d:%d", mostRecentItem.first, mostRecentItem.second);
        }
        leveldb::Status status = pdb->Put(writeoptions, key, newValue);
        if (msc_debug_fees) PrintToLog("Pruning fee cache for property %d, new=%s [%s])\n", propertyId, newValue, status.ToString());
    } else {
        return; // nothing to do
    }
}

// Show Fee Cache DB statistics
void COmniFeeCache::printStats()
{
    PrintToLog("COmniFeeCache stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

// Show Fee Cache DB records
void COmniFeeCache::printAll()
{
    int count = 0;
    leveldb::Iterator* it = NewIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
        ++count;
        PrintToConsole("entry #%8d= %s:%s\n", count, it->key().ToString(), it->value().ToString());
    }
    delete it;
}

// Return a set containing fee cache history items
std::set<feeCacheItem> COmniFeeCache::GetCacheHistory(const uint32_t &propertyId)
{
    assert(pdb);

    const std::string key = strprintf("%010d", propertyId);

    std::set<feeCacheItem> sCacheHistoryItems;
    std::string strValue;
    leveldb::Status status = pdb->Get(readoptions, key, &strValue);
    assert(status.ok());
    std::vector<std::string> vCacheHistoryItems;
    boost::split(vCacheHistoryItems, strValue, boost::is_any_of(","), boost::token_compress_on);
    for (std::vector<std::string>::iterator it = vCacheHistoryItems.begin(); it != vCacheHistoryItems.end(); ++it) {
        std::vector<std::string> vCacheHistoryItem;
        boost::split(vCacheHistoryItem, *it, boost::is_any_of(":"), boost::token_compress_on);
        if (2 != vCacheHistoryItem.size()) {
            PrintToConsole("ERROR: vCacheHistoryItem has unexpected number of elements: %d (raw %s)!\n", vCacheHistoryItem.size(), *it);
            continue;
        }
        int64_t cacheItemBlock = boost::lexical_cast<int64_t>(vCacheHistoryItem[0]);
        int64_t cacheItemAmount = boost::lexical_cast<int64_t>(vCacheHistoryItem[1]);
        sCacheHistoryItems.insert(std::make_pair(cacheItemBlock, cacheItemAmount));
        if (msc_debug_fees) PrintToConsole("Retrieved fee item (block %d, amount %d)\n", cacheItemBlock, cacheItemAmount);
    }

    return sCacheHistoryItems;
}


