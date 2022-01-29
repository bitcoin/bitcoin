#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>
#include <cassert>

using namespace mmr;

LeafIndex MemMMR::AddLeaf(const Leaf& leaf)
{
    m_leaves.push_back(leaf);
    m_hashes.push_back(leaf.GetHash());

    auto nextIdx = leaf.GetNodeIndex().GetNext();
    while (!nextIdx.IsLeaf()) {
        mw::Hash leftHash = GetHash(nextIdx.GetLeftChild());
        m_hashes.push_back(MMRUtil::CalcParentHash(nextIdx, leftHash, m_hashes.back()));
        nextIdx = nextIdx.GetNext();
    }

    return leaf.GetLeafIndex();
}

Leaf MemMMR::GetLeaf(const LeafIndex& leafIdx) const
{
    assert(leafIdx.Get() < m_leaves.size());
    return m_leaves[leafIdx.Get()];
}

mw::Hash MemMMR::GetHash(const Index& idx) const
{
    assert(idx.GetPosition() < m_hashes.size());
    return m_hashes[idx.GetPosition()];
}

LeafIndex MemMMR::GetNextLeafIdx() const noexcept
{
    return LeafIndex::At(GetNumLeaves());
}

uint64_t MemMMR::GetNumLeaves() const noexcept
{
    return m_leaves.size();
}

void MemMMR::Rewind(const uint64_t numLeaves)
{
    assert(numLeaves <= m_leaves.size());
    m_leaves.resize(numLeaves);
    m_hashes.resize(GetNextLeafIdx().GetPosition());
}