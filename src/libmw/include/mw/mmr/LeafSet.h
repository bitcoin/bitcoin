#pragma once

#include <mw/common/Macros.h>
#include <mw/common/BitSet.h>
#include <mw/file/File.h>
#include <mw/file/MemMap.h>
#include <mw/models/crypto/Hash.h>
#include <mw/mmr/LeafIndex.h>
#include <unordered_map>

class ILeafSet
{
public:
	using Ptr = std::shared_ptr<ILeafSet>;

	virtual ~ILeafSet() = default;

	virtual uint8_t GetByte(const uint64_t byteIdx) const = 0;
	virtual void SetByte(const uint64_t byteIdx, const uint8_t value) = 0;

	void Add(const mmr::LeafIndex& idx);
	void Remove(const mmr::LeafIndex& idx);
	bool Contains(const mmr::LeafIndex& idx) const noexcept;
	mw::Hash Root() const;
	void Rewind(const uint64_t numLeaves, const std::vector<mmr::LeafIndex>& leavesToAdd);
	const mmr::LeafIndex& GetNextLeafIdx() const noexcept { return m_nextLeafIdx; }
	BitSet ToBitSet() const;

	virtual void ApplyUpdates(
		const uint32_t file_index,
		const mmr::LeafIndex& nextLeafIdx,
		const std::unordered_map<uint64_t, uint8_t>& modifiedBytes
	) = 0;

protected:
	uint8_t BitToByte(const uint8_t bit) const;

	ILeafSet(const mmr::LeafIndex& nextLeafIdx)
		: m_nextLeafIdx(nextLeafIdx) { }

	mmr::LeafIndex m_nextLeafIdx;
};

class LeafSet : public ILeafSet
{
public:
	using Ptr = std::shared_ptr<LeafSet>;

	static LeafSet::Ptr Open(const FilePath& leafset_dir, const uint32_t file_index);
	static FilePath GetPath(const FilePath& leafset_dir, const uint32_t file_index);

	uint8_t GetByte(const uint64_t byteIdx) const final;
	void SetByte(const uint64_t byteIdx, const uint8_t value) final;

	void ApplyUpdates(
		const uint32_t file_index,
		const mmr::LeafIndex& nextLeafIdx,
		const std::unordered_map<uint64_t, uint8_t>& modifiedBytes
	) final;
	void Flush(const uint32_t file_index);
	void Cleanup(const uint32_t current_file_index) const;

private:
	LeafSet(FilePath dir, MemMap&& mmap, const mmr::LeafIndex& nextLeafIdx)
        : ILeafSet(nextLeafIdx), m_dir(std::move(dir)), m_mmap(std::move(mmap)) {}

	FilePath m_dir;
	MemMap m_mmap;
	std::unordered_map<uint64_t, uint8_t> m_modifiedBytes;
};

class LeafSetCache : public ILeafSet
{
public:
	using Ptr = std::shared_ptr<LeafSetCache>;
	using UPtr = std::unique_ptr<LeafSetCache>;

	LeafSetCache(const ILeafSet::Ptr& pBacked)
		: ILeafSet(pBacked->GetNextLeafIdx()), m_pBacked(pBacked) { }

	uint8_t GetByte(const uint64_t byteIdx) const final;
	void SetByte(const uint64_t byteIdx, const uint8_t value) final;

	void ApplyUpdates(
		const uint32_t file_index,
		const mmr::LeafIndex& nextLeafIdx,
		const std::unordered_map<uint64_t, uint8_t>& modifiedBytes
	) final;
	void Flush(const uint32_t file_index);

private:
	ILeafSet::Ptr m_pBacked;
	std::unordered_map<uint64_t, uint8_t> m_modifiedBytes;
};