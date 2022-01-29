#include <mw/mmr/LeafSet.h>
#include <mw/crypto/Hasher.h>

using namespace mmr;

LeafSet::Ptr LeafSet::Open(const FilePath& leafset_dir, const uint32_t file_index)
{
    File file = GetPath(leafset_dir, file_index);
    if (!file.Exists()) {
        file.Create();
    }

    mmr::LeafIndex nextLeafIdx = mmr::LeafIndex::At(0);
    if (file.GetSize() < 8) {
        file.Write(nextLeafIdx.Serialized());
    } else {
        nextLeafIdx = LeafIndex::Deserialize(file.ReadBytes(0, 8));
    }

    MemMap mappedFile{ file };
    mappedFile.Map();
    return std::shared_ptr<LeafSet>(new LeafSet{ leafset_dir, std::move(mappedFile), nextLeafIdx });
}

FilePath LeafSet::GetPath(const FilePath& leafset_dir, const uint32_t file_index)
{
    return leafset_dir.GetChild(StringUtil::Format("leaf{:0>6}.dat", file_index));
}

void LeafSet::ApplyUpdates(
    const uint32_t file_index,
    const mmr::LeafIndex& nextLeafIdx,
    const std::unordered_map<uint64_t, uint8_t>& modifiedBytes)
{
    for (auto byte : modifiedBytes) {
        m_modifiedBytes[byte.first + 8] = byte.second;
    }

    // In case of rewind, make sure to clear everything above the new next
    for (size_t idx = nextLeafIdx.Get(); idx < m_nextLeafIdx.Get(); idx++) {
        Remove(mmr::LeafIndex::At(idx));
    }

    m_nextLeafIdx = nextLeafIdx;

    Flush(file_index);
}

void LeafSet::Flush(const uint32_t file_index)
{
    m_mmap.Unmap();

    std::vector<uint8_t> nextLeafIdxBytes = m_nextLeafIdx.Serialized();
    assert(nextLeafIdxBytes.size() == 8);

    for (uint8_t i = 0; i < 8; i++) {
        m_modifiedBytes[i] = nextLeafIdxBytes[i];
    }

    FilePath new_leafset_path = GetPath(m_dir, file_index);
    m_mmap.GetFile().CopyTo(new_leafset_path);

    File new_leafset_file(std::move(new_leafset_path));
    new_leafset_file.WriteBytes(m_modifiedBytes);

    m_mmap = MemMap{ new_leafset_file };
    m_mmap.Map();

    m_modifiedBytes.clear();
}

void LeafSet::Cleanup(const uint32_t current_file_index) const
{
    uint32_t file_index = current_file_index;
    while (file_index > 0) {
        FilePath prev_leafset = GetPath(m_dir, --file_index);
        if (prev_leafset.Exists()) {
            prev_leafset.Remove();
        } else {
            break;
        }
    }
}

uint8_t LeafSet::GetByte(const uint64_t byteIdx) const
{
    // Offset by 8 bytes, since first 8 bytes in file represent the next leaf index
    const uint64_t byteIdxWithOffset = byteIdx + 8;

    auto iter = m_modifiedBytes.find(byteIdxWithOffset);
    if (iter != m_modifiedBytes.cend())
    {
        return iter->second;
    }
    else if (byteIdxWithOffset < m_mmap.size())
    {
        return m_mmap.ReadByte(byteIdxWithOffset);
    }

    return 0;
}

void LeafSet::SetByte(const uint64_t byteIdx, const uint8_t value)
{
    m_modifiedBytes[byteIdx + 8] = value;
}