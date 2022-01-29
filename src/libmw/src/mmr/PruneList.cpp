#include <mw/mmr/PruneList.h>
#include <mw/file/File.h>

using namespace mmr;

PruneList::Ptr PruneList::Open(const FilePath& parent_dir, const uint32_t file_index)
{
    File file = GetPath(parent_dir, file_index);

    BitSet bitset;
    if (file.Exists()) {
        bitset = BitSet::From(file.ReadBytes());
    }

    uint64_t total_shift = bitset.count();
    return std::shared_ptr<PruneList>(new PruneList(parent_dir, std::move(bitset), total_shift));
}

FilePath PruneList::GetPath(const FilePath& dir, const uint32_t file_index)
{
    return dir.GetChild(StringUtil::Format("prun{:0>6}.dat", file_index));
}

uint64_t PruneList::GetShift(const Index& index) const noexcept
{
    assert(!m_compacted.test(index.GetPosition()));

    // NOTE: rank() uses an inefficient algorithm.
    // A shift cache with an efficient "nearest-neighbor" searching algorithm would be better.
    return m_compacted.rank(index.GetPosition());
}

uint64_t PruneList::GetShift(const LeafIndex& index) const noexcept
{
    return GetShift(index.GetNodeIndex());
}

void PruneList::Commit(const uint32_t file_index, const BitSet& compacted)
{
    File(GetPath(m_dir, file_index))
        .Write(compacted.bytes());

    m_compacted = compacted;
    m_totalShift = compacted.count();
}