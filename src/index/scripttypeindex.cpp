#include <index/scripttypeindex.h>
#include <common/args.h>
#include <primitives/block.h>
#include <logging.h>

std::unique_ptr<ScriptTypeIndex> g_script_type_index;

ScriptTypeIndex::ScriptTypeIndex(std::unique_ptr<interfaces::Chain> chain, size_t cache_size)
    : BaseIndex(std::move(chain), "scripttypeindex")
    , m_db(std::make_unique<BaseIndex::DB>(gArgs.GetDataDirNet() / "indexes" / "scripttypeindex",
                                           cache_size, /*memory=*/false, /*wipe=*/false))
{
}

ScriptTypeIndex::~ScriptTypeIndex() = default;

bool ScriptTypeIndex::CustomAppend(const interfaces::BlockInfo& block) {
    assert(block.data);
    ScriptTypeBlockStats stats = ComputeStats(*block.data);
    m_db->Write(std::make_pair(DB_SCRIPT_TYPE_STATS, block.hash), stats);
    return true; 
}
bool ScriptTypeIndex::CustomRemove(const interfaces::BlockInfo& block) { return true; }
bool ScriptTypeIndex::LookupStats(const uint256& block_hash, ScriptTypeBlockStats& stats) const { return false; }
ScriptTypeBlockStats ScriptTypeIndex::ComputeStats(const CBlock& block) const { 
    
    // Init stats
    ScriptTypeBlockStats stats{};

    // loop through all transactions in the block and check every output to determine the script type
    for (const auto& tx :block.vtx) {
        for(const auto& out : tx->vout) {
            std::vector<std::vector<unsigned char>> solutions; // unused, but needed for Solver
            TxoutType script_type = Solver(out.scriptPubKey, solutions);
            size_t type_idx = static_cast<size_t>(script_type);

            // skip unknown script types
            if (type_idx >= ScriptTypeBlockStats::TXOUT_TYPE_COUNT) {
                LogError("ScriptTypeIndex: Unknown script type %d\n", static_cast<int>(script_type));
                continue;
            }

            // update stats
            stats.output_counts[type_idx]++;
            stats.output_values[type_idx] += out.nValue;
        }
    }

    return stats; 
}