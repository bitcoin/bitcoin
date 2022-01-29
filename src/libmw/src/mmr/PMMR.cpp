#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>
#include <mw/mmr/PruneList.h>
#include <mw/common/Logger.h>
#include <mw/db/LeafDB.h>
#include <mw/exceptions/NotFoundException.h>

using namespace mmr;

PMMR::Ptr PMMR::Open(
    const char dbPrefix,
    const FilePath& mmr_dir,
    const uint32_t file_index,
    const mw::DBWrapper::Ptr& pDBWrapper,
    const PruneList::CPtr& pPruneList)
{
    auto pHashFile = AppendOnlyFile::Load(
        GetPath(mmr_dir, dbPrefix, file_index)
    );
    return std::make_shared<PMMR>(
        dbPrefix,
        mmr_dir,
        pHashFile,
        pDBWrapper,
        pPruneList
    );
}

FilePath PMMR::GetPath(const FilePath& dir, const char prefix, const uint32_t file_index)
{
    return dir.GetChild(StringUtil::Format("{}{:0>6}.dat", prefix, file_index));
}

LeafIndex PMMR::AddLeaf(const mmr::Leaf& leaf)
{
    m_leafMap[leaf.GetLeafIndex()] = m_leaves.size();
    m_leaves.push_back(leaf);
    m_pHashFile->Append(leaf.GetHash().vec());

    auto rightHash = leaf.GetHash();
    auto nextIdx = leaf.GetNodeIndex().GetNext();
    while (!nextIdx.IsLeaf()) {
        const mw::Hash leftHash = GetHash(nextIdx.GetLeftChild());
        rightHash = MMRUtil::CalcParentHash(nextIdx, leftHash, rightHash);

        m_pHashFile->Append(rightHash.vec());
        nextIdx = nextIdx.GetNext();
    }

    return leaf.GetLeafIndex();
}

Leaf PMMR::GetLeaf(const LeafIndex& idx) const
{
    auto it = m_leafMap.find(idx);
    if (it != m_leafMap.end()) {
        return m_leaves[it->second];
    }

    LeafDB ldb(m_dbPrefix, m_pDatabase.get());
    auto pLeaf = ldb.Get(idx);
    if (!pLeaf) {
        ThrowNotFound_F("Can't get leaf at position {}", idx.GetPosition());
    }

    return std::move(*pLeaf);
}

mw::Hash PMMR::GetHash(const Index& idx) const
{
    uint64_t pos = idx.GetPosition();
    if (m_pPruneList) {
        pos -= m_pPruneList->GetShift(idx);
    }

    return mw::Hash(m_pHashFile->Read(pos * mw::Hash::size(), mw::Hash::size()));
}

uint64_t PMMR::GetNumLeaves() const noexcept
{
    uint64_t num_hashes = (m_pHashFile->GetSize() / mw::Hash::size());
    if (m_pPruneList) {
        num_hashes += m_pPruneList->GetTotalShift();
    }

    return Index::At(num_hashes).GetLeafIndex();
}

uint64_t PMMR::GetNumNodes() const noexcept
{
    return LeafIndex::At(GetNumLeaves()).GetPosition();
}

void PMMR::Rewind(const uint64_t numLeaves)
{
    LOG_TRACE_F("Rewinding to {}", numLeaves);

    LeafIndex next_leaf_idx = LeafIndex::At(numLeaves);
    uint64_t pos = next_leaf_idx.GetPosition();
    if (m_pPruneList) {
        pos -= m_pPruneList->GetShift(next_leaf_idx);
    }

    m_pHashFile->Rewind(pos * mw::Hash::size());
}

void PMMR::BatchWrite(
    const uint32_t file_index,
    const LeafIndex& firstLeafIdx,
    const std::vector<Leaf>& leaves,
    const std::unique_ptr<mw::DBBatch>& pBatch)
{
    LOG_TRACE_F("Writing batch {} with first leaf {}", file_index, firstLeafIdx.Get());

    Rewind(firstLeafIdx.Get());
    for (const Leaf& leaf : leaves) {
        AddLeaf(leaf);
    }

    m_pHashFile->Commit(GetPath(m_dir, m_dbPrefix, file_index));

    // Update database
    LeafDB(m_dbPrefix, m_pDatabase.get(), pBatch.get())
        .Add(m_leaves);

    m_leaves.clear();
    m_leafMap.clear();
}

void PMMR::Cleanup(const uint32_t current_file_index) const
{
    uint32_t file_index = current_file_index;
    while (file_index > 0) {
        FilePath prev_hashfile = GetPath(m_dir, m_dbPrefix, --file_index);
        if (prev_hashfile.Exists()) {
            prev_hashfile.Remove();
        } else {
            break;
        }
    }
}