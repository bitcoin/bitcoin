#ifndef OMNICORE_FEES_H
#define OMNICORE_FEES_H

#include "leveldb/db.h"

#include "omnicore/log.h"
#include "omnicore/persistence.h"

#include <set>
#include <stdint.h>
#include <boost/filesystem.hpp>

typedef std::pair<int, int64_t> feeCacheItem;
typedef std::pair<std::string, int64_t> feeHistoryItem;

/** LevelDB based storage for the MetaDEx fee cache
 */
class COmniFeeCache : public CDBBase
{
public:
    COmniFeeCache(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading fee cache database: %s\n", status.ToString());
    }

    virtual ~COmniFeeCache()
    {
        if (msc_debug_fees) PrintToLog("COmniFeeCache closed\n");
    }

    // Show Fee Cache DB statistics
    void printStats();
    // Show Fee Cache DB records
    void printAll();

    // Sets the distribution thresholds to total tokens for a property / OMNI_FEE_THRESHOLD
    void UpdateDistributionThresholds(uint32_t propertyId);
    // Returns the distribution threshold for a property
    int64_t GetDistributionThreshold(const uint32_t &propertyId);
    // Return a set containing fee cache history items
    std::set<feeCacheItem> GetCacheHistory(const uint32_t &propertyId);
    // Gets the current amount of the fee cache for a property
    int64_t GetCachedAmount(const uint32_t &propertyId);
    // Prunes entries over 50 blocks old from the entry for a property
    void PruneCache(const uint32_t &propertyId, int block);
    // Rolls back the cache to an earlier state (eg in event of a reorg) - block is *inclusive* (ie entries=block will get deleted)
    void RollBackCache(int block);
    // Zeros a property in the fee cache
    void ClearCache(const uint32_t &propertyId, int block);
    // Adds a fee to the cache (eg on a completed trade)
    void AddFee(const uint32_t &propertyId, int block, const int64_t &amount);
    // Evaluates fee caches for all properties against threshold and executes distribution if threshold met
    void EvalCache(const uint32_t &propertyId, int block);
    // Performs distribution of fees
    void DistributeCache(const uint32_t &propertyId, int block);
};

/** LevelDB based storage for the MetaDEx fee distributions
 */
class COmniFeeHistory : public CDBBase
{
public:
    COmniFeeHistory(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading fee history database: %s\n", status.ToString());
    }

    virtual ~COmniFeeHistory()
    {
        if (msc_debug_fees) PrintToLog("COmniFeeHistory closed\n");
    }

    // Show Fee History DB statistics
    void printStats();
    // Show Fee History DB records
    void printAll();

    // Roll back history in event of reorg
    void RollBackHistory(int block);
    // Count Fee History DB records
    int CountRecords();
    // Record a fee distribution
    void RecordFeeDistribution(const uint32_t &propertyId, int block, int64_t total, std::set<feeHistoryItem> feeRecipients);
    // Retrieve the recipients for a fee distribution
    std::set<feeHistoryItem> GetFeeDistribution(int id);
    // Retrieve fee distributions for a property
    std::set<int> GetDistributionsForProperty(const uint32_t &propertyId);
    // Populate data about a fee distribution
    bool GetDistributionData(int id, uint32_t *propertyId, int *block, int64_t *total);
    // Retrieve fee distribution receipts for an address
};

namespace mastercore
{
    extern COmniFeeCache *p_feecache;
    extern COmniFeeHistory *p_feehistory;
}

#endif // OMNICORE_FEES_H
