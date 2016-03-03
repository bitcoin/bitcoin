#ifndef OMNICORE_FEES_H
#define OMNICORE_FEES_H

#include "leveldb/db.h"

#include "omnicore/log.h"
#include "omnicore/persistence.h"

#include <set>
#include <stdint.h>
#include <boost/filesystem.hpp>

typedef std::pair<int, int64_t> feeCacheItem;

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

    // Return a set containing fee cache history items
    std::set<feeCacheItem> GetCacheHistory(const uint32_t &propertyId);
    // Gets the current amount of the fee cache for a property
    int64_t GetCachedAmount(const uint32_t &propertyId);
    // Prunes entries over 50 blocks old from the entry for a property
    void PruneCache(const uint32_t &propertyId, int block);
    // Rolls back the cache to an earlier state (eg in event of a reorg) - block is *inclusive* (ie entries=block will get deleted)
    void RollBackCache(int block);
    // Adds a fee to the cache (eg on a completed trade)
    void AddFee(const uint32_t &propertyId, int block, const uint64_t &amount);
    // Evaluates fee caches for all properties against threshold and executes distribution if threshold met
    void EvalCache();
};

namespace mastercore
{
    extern COmniFeeCache *p_feecache;
}

#endif // OMNICORE_FEES_H
