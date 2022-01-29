#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <mw/file/AppendOnlyFile.h>
#include <mw/file/FilePath.h>
#include <mw/models/crypto/Hash.h>
#include <mw/mmr/LeafIndex.h>
#include <mw/mmr/Leaf.h>
#include <mw/mmr/PruneList.h>
#include <mw/interfaces/db_interface.h>

/// <summary>
/// An interface for interacting with MMRs.
/// </summary>
class IMMR
{
public:
    using Ptr = std::shared_ptr<IMMR>;
    virtual ~IMMR() = default;

    /// <summary>
    /// Adds a new leaf with the given data to the end of the MMR.
    /// </summary>
    /// <param name="data">The serialized leaf data.</param>
    /// <returns>The LeafIndex where the data was added.</returns>
    virtual mmr::LeafIndex AddLeaf(const mmr::Leaf& leaf) = 0;
    mmr::LeafIndex Add(const std::vector<uint8_t>& data) { return AddLeaf(mmr::Leaf::Create(GetNextLeafIdx(), data)); }
    mmr::LeafIndex Add(const Traits::ISerializable& serializable) { return AddLeaf(mmr::Leaf::Create(GetNextLeafIdx(), serializable.Serialized())); }

    /// <summary>
    /// Retrieves the leaf at the given leaf index.
    /// </summary>
    /// <param name="leafIdx">The leaf index.</param>
    /// <returns>The retrieved Leaf.</returns>
    /// <throws>std::exception if index is beyond the end of the MMR.</throws>
    /// <throws>std::exception if leaf at given index has been pruned.</throws>
    virtual mmr::Leaf GetLeaf(const mmr::LeafIndex& leafIdx) const = 0;

    /// <summary>
    /// Retrieves the hash at the given MMR index.
    /// </summary>
    /// <param name="idx">The index, which may or may not be a leaf.</param>
    /// <returns>The hash of the leaf or node at the index.</returns>
    /// <throws>std::exception if index is beyond the end of the MMR.</throws>
    /// <throws>std::exception if node at the given index has been pruned.</throws>
    virtual mw::Hash GetHash(const mmr::Index& idx) const = 0;

    /// <summary>
    /// Retrieves the index of the next leaf to be added to the MMR.
    /// eg. If the MMR contains 3 leaves (0, 1, 2), this will return LeafIndex 3.
    /// </summary>
    /// <returns>The next leaf index.</returns>
    virtual mmr::LeafIndex GetNextLeafIdx() const noexcept = 0;

    /// <summary>
    /// Gets the number of leaves in the MMR, including those that have been pruned.
    /// eg. If the MMR contains leaves 0, 1, and 2, this will return 3.
    /// </summary>
    /// <returns>The number of (pruned and unpruned) leaves in the MMR.</returns>
    virtual uint64_t GetNumLeaves() const noexcept = 0;

    /// <summary>
    /// "Rewinds" the MMR to the given number of leaves.
    /// In other words, it shrinks the MMR to the given size.
    /// </summary>
    /// <param name="numLeaves">The total number of (pruned and unpruned) leaves in the MMR.</param>
    virtual void Rewind(const uint64_t numLeaves) = 0;
    void Rewind(const mmr::LeafIndex& nextLeaf) { Rewind(nextLeaf.Get()); }
    
    /// <summary>
    /// Unlike a Merkle tree, an MMR generally has no single root so we need a method to compute one.
    /// The process we use is called "bagging the peaks." We first identify the peaks (nodes with no parents).
    /// We then "bag" them by hashing them iteratively from the right, using the total size of the MMR as prefix. 
    /// </summary>
    /// <returns>The root hash of the MMR.</returns>
    mw::Hash Root() const;

    /// <summary>
    /// Adds the given leaves to the MMR.
    /// This also updates the database and MMR files when the MMR is not a cache.
    /// Typically, this is called from a derived PMMRCache when its changes are being flushed/committed.
    /// </summary>
    /// <param name="file_index">The index of the MMR files. This should be incremented with each write.</param>
    /// <param name="firstLeafIdx">The LeafIndex of the first leaf being added.</param>
    /// <param name="leaves">The leaves being added to the MMR.</param>
    /// <param name="pBatch">A wrapper around a DB Batch. Required when called for an MMR (ie, non-cache).</param>
    virtual void BatchWrite(
        const uint32_t file_index,
        const mmr::LeafIndex& firstLeafIdx,
        const std::vector<mmr::Leaf>& leaves,
        const std::unique_ptr<mw::DBBatch>& pBatch
    ) = 0;
};

/// <summary>
/// Memory-only MMR with no pruning or file I/O.
/// </summary>
class MemMMR : public IMMR
{
public:
    using Ptr = std::shared_ptr<MemMMR>;

    MemMMR() = default;
    virtual ~MemMMR() = default;

    mmr::LeafIndex AddLeaf(const mmr::Leaf& leaf) final;
    mmr::Leaf GetLeaf(const mmr::LeafIndex& leafIdx) const final;
    mw::Hash GetHash(const mmr::Index& idx) const final;

    mmr::LeafIndex GetNextLeafIdx() const noexcept final;
    uint64_t GetNumLeaves() const noexcept final;
    void Rewind(const uint64_t numLeaves) final;

    void BatchWrite(
        const uint32_t,
        const mmr::LeafIndex&,
        const std::vector<mmr::Leaf>&,
        const std::unique_ptr<mw::DBBatch>&) final {}

private:
    std::vector<mw::Hash> m_hashes;
    std::vector<mmr::Leaf> m_leaves;
};

class PMMR : public IMMR
{
public:
    using Ptr = std::shared_ptr<PMMR>;

    static PMMR::Ptr Open(
        const char dbPrefix,
        const FilePath& mmr_dir,
        const uint32_t file_index,
        const mw::DBWrapper::Ptr& pDBWrapper,
        const std::shared_ptr<const PruneList>& pPruneList
    );

    PMMR(const char dbPrefix,
        const FilePath& mmr_dir,
        const AppendOnlyFile::Ptr& pHashFile,
        const std::shared_ptr<mw::DBWrapper>& pDBWrapper,
        const PruneList::CPtr& pPruneList
    ) :
        m_dbPrefix(dbPrefix),
        m_dir(mmr_dir),
        m_pHashFile(pHashFile),
        m_pDatabase(pDBWrapper),
        m_pPruneList(pPruneList) { }

    virtual ~PMMR() = default;

    static FilePath GetPath(const FilePath& dir, const char prefix, const uint32_t file_index);

    mmr::LeafIndex AddLeaf(const mmr::Leaf& leaf) final;

    mmr::Leaf GetLeaf(const mmr::LeafIndex& leafIdx) const final;
    mw::Hash GetHash(const mmr::Index& idx) const final;
    mmr::LeafIndex GetNextLeafIdx() const noexcept final { return mmr::LeafIndex::At(GetNumLeaves()); }

    uint64_t GetNumLeaves() const noexcept final;
    uint64_t GetNumNodes() const noexcept;
    void Rewind(const uint64_t numLeaves) final;

    void BatchWrite(
        const uint32_t file_index,
        const mmr::LeafIndex& firstLeafIdx,
        const std::vector<mmr::Leaf>& leaves,
        const std::unique_ptr<mw::DBBatch>& pBatch
    ) final;
    void Cleanup(const uint32_t current_file_index) const;

private:
    char m_dbPrefix;
    FilePath m_dir;
    AppendOnlyFile::Ptr m_pHashFile;
    std::vector<mmr::Leaf> m_leaves;
    std::map<mmr::LeafIndex, size_t> m_leafMap;
    std::shared_ptr<mw::DBWrapper> m_pDatabase;
    PruneList::CPtr m_pPruneList;
};

class PMMRCache : public IMMR
{
public:
    using Ptr = std::shared_ptr<PMMRCache>;

    PMMRCache(const IMMR::Ptr& pBacked)
        : m_pBase(pBacked), m_firstLeaf(pBacked->GetNextLeafIdx()){ }
    virtual ~PMMRCache() = default;

    mmr::LeafIndex AddLeaf(const mmr::Leaf& leaf) final;

    mmr::Leaf GetLeaf(const mmr::LeafIndex& leafIdx) const final;
    mmr::LeafIndex GetNextLeafIdx() const noexcept final;
    uint64_t GetNumLeaves() const noexcept final { return GetNextLeafIdx().Get(); }
    mw::Hash GetHash(const mmr::Index& idx) const final;

    void Rewind(const uint64_t numLeaves) final;

    void BatchWrite(
        const uint32_t file_index,
        const mmr::LeafIndex& firstLeafIdx,
        const std::vector<mmr::Leaf>& leaves,
        const std::unique_ptr<mw::DBBatch>& pBatch
    ) final;

    void Flush(const uint32_t index, const std::unique_ptr<mw::DBBatch>& pBatch);

private:
    IMMR::Ptr m_pBase;
    mmr::LeafIndex m_firstLeaf;
    std::vector<mmr::Leaf> m_leaves;
    std::vector<mw::Hash> m_nodes;
};