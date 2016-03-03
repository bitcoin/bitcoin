/**
 * @file fees.cpp
 *
 * This file contains code for handling Omni fees.
 */

#include "omnicore/fees.h"

#include "omnicore/omnicore.h"
#include "omnicore/log.h"
#include "omnicore/sp.h"

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
        --endIt;
        feeCacheItem mostRecentItem = *endIt;
        return mostRecentItem.second;
    } else {
        return 0; // property has never generated a fee
    }
}

// Adds a fee to the cache (eg on a completed trade)
void COmniFeeCache::AddFee(const uint32_t &propertyId, int block, const uint64_t &amount)
{
    if (msc_debug_fees) PrintToLog("Starting AddFee for prop %d (block %d amount %d)...\n", propertyId, block, amount);

    // Get current cached fee
    int64_t currentCachedAmount = GetCachedAmount(propertyId);
    if (msc_debug_fees) PrintToLog("   Current cached amount %d\n", currentCachedAmount);

    // Add new fee and rewrite record
    int64_t newCachedAmount = currentCachedAmount + amount; // TODO: should be some overflow detection in here
    if (msc_debug_fees) PrintToLog("   New cached amount %d\n", newCachedAmount);
    const std::string key = strprintf("%010d", propertyId);
    std::set<feeCacheItem> sCacheHistoryItems = GetCacheHistory(propertyId);
    if (msc_debug_fees) PrintToLog("   Iterating cache history (%d items)...\n",sCacheHistoryItems.size());
    std::string newValue;
    if (!sCacheHistoryItems.empty()) {
        for (std::set<feeCacheItem>::iterator it = sCacheHistoryItems.begin(); it != sCacheHistoryItems.end(); it++) {
            feeCacheItem tempItem = *it;
            if (tempItem.first == block) continue; // this is an older entry for the same block, discard it
            if (!newValue.empty()) newValue += ",";
            newValue += strprintf("%d:%d", tempItem.first, tempItem.second);
            if (msc_debug_fees) PrintToLog("      Readding entry: block %d amount %d\n", tempItem.first, tempItem.second);
        }
        if (!newValue.empty()) newValue += ",";
    }
    if (msc_debug_fees) PrintToLog("   Adding requested entry: block %d new amount %d\n", block, newCachedAmount);
    newValue += strprintf("%d:%d", block, newCachedAmount);
    leveldb::Status status = pdb->Put(writeoptions, key, newValue);
    assert(status.ok());
    ++nWritten;
    if (msc_debug_fees) PrintToLog("AddFee completed for property %d (new=%s [%s])\n", propertyId, newValue, status.ToString());

    // Call for pruning (we only prune when we update a record)
    PruneCache(propertyId, block);

    return;
}

// Rolls back the cache to an earlier state (eg in event of a reorg) - block is *inclusive* (ie entries=block will get deleted)
void COmniFeeCache::RollBackCache(int block)
{
/*
    !!draft code, not yet tested!!

    assert(pdb);

    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t propertyId = startPropertyId; propertyId < mastercore::_my_sps->peekNextSPID(ecosystem); propertyId++) {
            const std::string key = strprintf("%010d", propertyId);
            std::set<feeCacheItem> sCacheHistoryItems = GetCacheHistory(propertyId);
            if (!sCacheHistoryItems.empty()) {
                std::set<feeCacheItem>::iterator mostRecentIt = sCacheHistoryItems.end();
                std::string newValue;
                feeCacheItem mostRecentItem = *mostRecentIt;
                if (mostRecentItem.first < block) return; // all entries are unaffected by this rollback, nothing to do
                for (std::set<feeCacheItem>::iterator it = sCacheHistoryItems.begin(); it != sCacheHistoryItems.end(); it++) {
                    feeCacheItem tempItem = *it;
                    if (tempItem.first >= block) continue; // discard this entry
                    if (!newValue.empty()) newValue += ",";
                    newValue += strprintf("%d:%d", tempItem.first, tempItem.second);
                }
                leveldb::Status status = pdb->Put(writeoptions, key, newValue);
                assert(status.ok());
                if (msc_debug_fees) PrintToLog("Rolling back fee cache for property %d, new=%s [%s])\n", propertyId, newValue, status.ToString());
            }
        }
    }
*/
}

// Evaluates fee caches for all properties against threshold and executes distribution if threshold met
void COmniFeeCache::EvalCache()
{
    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t propertyId = startPropertyId; propertyId < mastercore::_my_sps->peekNextSPID(ecosystem); propertyId++) {
            if (GetCachedAmount(propertyId) > feeDistributionThreshold) {
                // Execute distribution of fees for this property
            }
        }
    }
}

// Performs distribution of fees

// Prunes entries over 50 blocks old from the entry for a property
void COmniFeeCache::PruneCache(const uint32_t &propertyId, int block)
{
    if (msc_debug_fees) PrintToLog("Starting PruneCache for prop %d block %d...\n", propertyId, block);
    assert(pdb);

    int pruneBlock = block-50;
    if (msc_debug_fees) PrintToLog("Removing entries prior to block %d...\n", pruneBlock);
    const std::string key = strprintf("%010d", propertyId);
    std::set<feeCacheItem> sCacheHistoryItems = GetCacheHistory(propertyId);
    if (msc_debug_fees) PrintToLog("   Iterating cache history (%d items)...\n",sCacheHistoryItems.size());
    if (!sCacheHistoryItems.empty()) {
        std::set<feeCacheItem>::iterator startIt = sCacheHistoryItems.begin();
        feeCacheItem firstItem = *startIt;
        if (firstItem.first >= pruneBlock) {
            if (msc_debug_fees) PrintToLog("Endingg PruneCache - no matured entries found.\n");
            return; // all entries are above supplied block value, nothing to do
        }
        std::string newValue;
        for (std::set<feeCacheItem>::iterator it = sCacheHistoryItems.begin(); it != sCacheHistoryItems.end(); it++) {
            feeCacheItem tempItem = *it;
            if (tempItem.first < pruneBlock) {
                if (msc_debug_fees) {
                    PrintToLog("      Skipping matured entry: block %d amount %d\n", tempItem.first, tempItem.second);
                    continue; // discard this entry
                }
            }
            if (!newValue.empty()) newValue += ",";
            newValue += strprintf("%d:%d", tempItem.first, tempItem.second);
            if (msc_debug_fees) PrintToLog("      Readding immature entry: block %d amount %d\n", tempItem.first, tempItem.second);
        }
        // make sure the pruned cache isn't completely empty, if it is, prune down to just the most recent entry
        if (newValue.empty()) {
            std::set<feeCacheItem>::iterator mostRecentIt = sCacheHistoryItems.end();
            feeCacheItem mostRecentItem = *mostRecentIt;
            newValue = strprintf("%d:%d", mostRecentItem.first, mostRecentItem.second);
            if (msc_debug_fees) PrintToLog("   All entries matured and pruned - readding most recent entry: block %d amount %d\n", mostRecentItem.first, mostRecentItem.second);
        }
        leveldb::Status status = pdb->Put(writeoptions, key, newValue);
        assert(status.ok());
        if (msc_debug_fees) PrintToLog("PruneCache completed for property %d (new=%s [%s])\n", propertyId, newValue, status.ToString());
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
    if (status.IsNotFound()) {
        return sCacheHistoryItems; // no cache, return empty set
    }
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
    }

    return sCacheHistoryItems;
}


