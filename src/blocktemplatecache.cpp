#include <blocktemplatecache.h>
#include <node/miner.h>
#include <txmempool.h>
#include <validation.h>

#include <algorithm>
#include <deque>
#include <memory>

using node::BlockAssembler;

class BlockTemplateCacheImpl final: public BlockTemplateCache
{
public:
    BlockTemplateCacheImpl(CTxMemPool* mempool, ChainstateManager& chainman, size_t cache_size);
    std::shared_ptr<node::CBlockTemplate> GetBlockTemplate(const Options& options, std::chrono::seconds interval)
        override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Overridden from CValidationInterface. */
    void BlockConnected(ChainstateRole /*unused*/, const std::shared_ptr<const CBlock>& /*unused*/, const CBlockIndex * /*unused*/)
        override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
private:
    std::shared_ptr<CBlockTemplate> CreateBlockTemplateInternal(const Options& options) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

    struct Template {
        Options m_options;
        NodeClock::time_point m_time_generated;
        std::shared_ptr<CBlockTemplate> m_block_template;

        bool IntervalElapsed(const std::chrono::seconds& interval) const;
        bool OptionsEqual(const Options& options) const;
    };

    std::deque<Template> m_block_templates;
    CTxMemPool* m_mempool{nullptr};
    ChainstateManager& m_chainman;
    size_t m_cache_size;
    mutable Mutex m_mutex;
};

BlockTemplateCacheImpl::BlockTemplateCacheImpl(CTxMemPool* mempool, ChainstateManager& chainman, size_t cache_size)
    : m_mempool(mempool), m_chainman(chainman), m_cache_size(cache_size) {}

std::shared_ptr<BlockTemplateCache> MakeBlockTemplateCache(CTxMemPool* mempool, ChainstateManager& chainman, size_t cache_size)
{
    return std::make_shared<BlockTemplateCacheImpl>(mempool, chainman, cache_size);
}

void BlockTemplateCacheImpl::BlockConnected(
    ChainstateRole /*unused*/,
    const std::shared_ptr<const CBlock>& /*unused*/,
    const CBlockIndex* /*unused*/)
{
    LOCK(m_mutex);
    m_block_templates.clear();
}

bool BlockTemplateCacheImpl::Template::IntervalElapsed(const std::chrono::seconds& interval) const
{
    auto now = NodeClock::now();
    auto elapsed = now - m_time_generated;
    if (elapsed < std::chrono::seconds::zero()) {
        // m_time_generated is in the future; node clock is bumped to the past
        // hence no way to check the elapsed time just assume interval elapsed.
        return true;
    }
    return elapsed >= interval;
}

bool BlockTemplateCacheImpl::Template::OptionsEqual(const Options& other) const
{
    return m_options.use_mempool == other.use_mempool &&
           m_options.block_reserved_weight == other.block_reserved_weight &&
           m_options.coinbase_output_max_additional_sigops == other.coinbase_output_max_additional_sigops &&
           m_options.nBlockMaxWeight == other.nBlockMaxWeight &&
           m_options.blockMinFeeRate == other.blockMinFeeRate;
}

std::shared_ptr<CBlockTemplate> BlockTemplateCacheImpl::CreateBlockTemplateInternal(const Options& options)
{
    BlockAssembler assembler{m_chainman.ActiveChainstate(), m_mempool, options, BlockAssembler::ALLOW_OVERSIZED_BLOCKS};
    Template t;
    t.m_block_template = assembler.CreateNewBlock();
    t.m_options = options;
    t.m_time_generated = NodeClock::now();
    Assume(m_block_templates.size() <= m_cache_size);
    m_block_templates.emplace_back(t);
    if (m_block_templates.size() > m_cache_size) m_block_templates.pop_front();
    return t.m_block_template;
}

std::shared_ptr<CBlockTemplate> BlockTemplateCacheImpl::GetBlockTemplate(const Options& options, std::chrono::seconds interval)
{
    LOCK2(cs_main, m_mutex);
    auto it = std::find_if(
        m_block_templates.begin(),
        m_block_templates.end(),
        [&](const Template& t) { return t.OptionsEqual(options); }
    );
    if (it == m_block_templates.end() || it->IntervalElapsed(interval)) {
        return CreateBlockTemplateInternal(options);
    }
    if (options.test_block_validity && !it->m_options.test_block_validity) {
        if (BlockValidationState state{TestBlockValidity(m_chainman.ActiveChainstate(), it->m_block_template->block,
                               /*check_pow=*/false, /*check_merkle_root=*/false)}; !state.IsValid()) {
            throw std::runtime_error(strprintf("TestBlockValidity failed: %s", state.ToString()));
        }
        it->m_options.test_block_validity = true;
    }
    return it->m_block_template;
}

