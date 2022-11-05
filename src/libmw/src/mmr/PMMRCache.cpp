#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>
#include <mw/common/Logger.h>

using namespace mmr;

LeafIndex PMMRCache::AddLeaf(const Leaf& leaf)
{
    m_nodes.push_back(leaf.GetHash());

    auto rightHash = leaf.GetHash();
    auto nextIdx = leaf.GetNodeIndex().GetNext();
    while (!nextIdx.IsLeaf()) {
        const mw::Hash leftHash = GetHash(nextIdx.GetLeftChild());
        rightHash = MMRUtil::CalcParentHash(nextIdx, leftHash, rightHash);

        m_nodes.push_back(rightHash);
        nextIdx = nextIdx.GetNext();
    }

    m_leaves.push_back(leaf);
    return leaf.GetLeafIndex();
}

Leaf PMMRCache::GetLeaf(const LeafIndex& leafIdx) const
{
    if (leafIdx < m_firstLeaf) {
        return m_pBase->GetLeaf(leafIdx);
    }

    const uint64_t cacheIdx = leafIdx.Get() - m_firstLeaf.Get();
    if (cacheIdx > m_leaves.size()) {
        throw std::out_of_range("Attempting to access non-existent leaf");
    }

    return m_leaves[cacheIdx];
}

LeafIndex PMMRCache::GetNextLeafIdx() const noexcept
{
    if (m_leaves.empty()) {
        return m_firstLeaf;
    } else {
        return m_leaves.back().GetLeafIndex().Next();
    }
}

mw::Hash PMMRCache::GetHash(const Index& idx) const
{
    if (idx < m_firstLeaf.GetPosition()) {
        return m_pBase->GetHash(idx);
    } else {
        const uint64_t vecIdx = idx.GetPosition() - m_firstLeaf.GetPosition();
        assert(m_nodes.size() > vecIdx);
        return m_nodes[vecIdx];
    }
}

void PMMRCache::Rewind(const uint64_t numLeaves)
{
    LOG_TRACE_F("Rewinding to {}", numLeaves);

    LeafIndex nextLeaf = LeafIndex::At(numLeaves);
    if (nextLeaf <= m_firstLeaf) {
        m_firstLeaf = nextLeaf;
        m_leaves.clear();
        m_nodes.clear();
    } else if (!m_leaves.empty()) {
        auto iter = m_leaves.begin();
        while (iter != m_leaves.end() && iter->GetLeafIndex() < nextLeaf) {
            iter++;
        }

        if (iter != m_leaves.end()) {
            m_leaves.erase(iter, m_leaves.end());
        }

        const uint64_t numNodes = GetNumNodes() - m_firstLeaf.GetPosition();
        if (m_nodes.size() > numNodes) {
            m_nodes.erase(m_nodes.begin() + numNodes, m_nodes.end());
        }
    }
}

void PMMRCache::BatchWrite(
    const uint32_t /*index*/,
    const LeafIndex& firstLeafIdx,
    const std::vector<Leaf>& leaves,
    const std::unique_ptr<mw::DBBatch>&)
{
    LOG_TRACE_F("Writing batch {}", firstLeafIdx.Get());
    Rewind(firstLeafIdx.Get());
    for (const Leaf& leaf : leaves) {
        Add(leaf.vec());
    }
}

void PMMRCache::Flush(const uint32_t file_index, const std::unique_ptr<mw::DBBatch>& pBatch)
{
    LOG_TRACE_F(
        "Flushing {} leaves at {} with file index {}",
        m_leaves.size(),
        m_firstLeaf.Get(),
        file_index
    );
    m_pBase->BatchWrite(file_index, m_firstLeaf, m_leaves, pBatch);
    m_firstLeaf = GetNextLeafIdx();
    m_leaves.clear();
    m_nodes.clear();
}