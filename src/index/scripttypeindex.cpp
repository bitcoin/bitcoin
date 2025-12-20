#include <index/scripttypeindex.h>
#include <common/args.h>

std::unique_ptr<ScriptTypeIndex> g_script_type_index;

ScriptTypeIndex::ScriptTypeIndex(std::unique_ptr<interfaces::Chain> chain, size_t cache_size)
    : BaseIndex(std::move(chain), "scripttypeindex")
    , m_db(std::make_unique<BaseIndex::DB>(gArgs.GetDataDirNet() / "indexes" / "scripttypeindex",
                                           cache_size, /*memory=*/false, /*wipe=*/false))
{
}

ScriptTypeIndex::~ScriptTypeIndex() = default;

bool ScriptTypeIndex::CustomAppend(const interfaces::BlockInfo& block) { return true; }
bool ScriptTypeIndex::CustomRemove(const interfaces::BlockInfo& block) { return true; }
bool ScriptTypeIndex::LookupStats(const uint256& block_hash, ScriptTypeBlockStats& stats) const { return false; }
ScriptTypeBlockStats ScriptTypeIndex::ComputeStats(const CBlock& block) const { return {}; }