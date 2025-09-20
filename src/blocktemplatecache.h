#ifndef BITCOIN_BLOCKTEMPLATECACHE_H
#define BITCOIN_BLOCKTEMPLATECACHE_H

#include <node/miner.h>
#include <validationinterface.h>

#include <memory>
#include <vector>

class ChainstateManager;
class CTxMemPool;
class CValidationInterface;

using node::CBlockTemplate;
using Options = node::BlockAssembler::Options;

static constexpr size_t DEFAULT_CACHE_SIZE{10};

/**
 * BlockTemplateCache provides a thread-safe interface for creating and reusing
 * block templates with configurable cache size.
 *
 * The cache stores templates with their respective config options.
 * When a block template is requested:
 * - If a template with matching options exists and the interval
 *   has not elapsed, the cached template is returned.
 * - If no template exists or the interval has elapsed, a new template is generated,
 *   stored in the cache, and returned.
 * - After insertion, we evict the oldest template if the cache overflows.
 *
 * The cache inherits from CValidationInterface to receive notifications
 * about connected blocks, which triggers cache invalidation,
 * ensuring stale templates are not returned.
 */
class BlockTemplateCache : public CValidationInterface
{
public:
    virtual ~BlockTemplateCache() = default;

    /**
     * If a cached template exists with identical options and its age does not
     * exceed the specified interval, the cached template is returned.
     * Otherwise, a new template is created and stored in the cache.
     *
     * @param options The block assembly options to use.
     * @param interval Maximum age of a cached template to be considered fresh.
     * @return std::shared_ptr<CBlockTemplate> Shared pointer to the block template.
     */
    virtual std::shared_ptr<CBlockTemplate> GetBlockTemplate(
        const Options& options,
        std::chrono::seconds interval
    ) = 0;
};

std::shared_ptr<BlockTemplateCache> MakeBlockTemplateCache(
    CTxMemPool* mempool,
    ChainstateManager& chainman,
    size_t cache_size = DEFAULT_CACHE_SIZE
);

#endif // BITCOIN_BLOCKTEMPLATECACHE_H
