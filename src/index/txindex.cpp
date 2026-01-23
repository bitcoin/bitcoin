// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>

#include <chain.h>
#include <common/args.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/disktxpos.h>
#include <index/txindex_key.h>
#include <interfaces/chain.h>
#include <interfaces/types.h>
#include <kernel/cs_main.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/log.h>
#include <validation.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

std::unique_ptr<TxIndex> g_txindex;

namespace {
SipHasher13UJ ReadOrCreateTxidHasher(CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    if (!db.Read(txindex::DB_TXID_HASH_SALT, salt)) {
        FastRandomContext rng{};
        salt = {rng.rand64(), rng.rand64()};
        db.Write(txindex::DB_TXID_HASH_SALT, salt, /*fSync=*/true);
    }
    return SipHasher13UJ{salt.first, salt.second};
}
} // namespace

/** Access to the txindex database (indexes/txindex/) */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Write a block of transaction positions to the DB.
    void WriteTxs(const interfaces::BlockInfo& block);

    /// Used to hash the txid to compute the prefix.
    const SipHasher13UJ m_hasher;

    /// Whether the database contains any legacy ('t' + txid) entries.
    const bool m_has_legacy;

    CBlockLocator ReadBestBlock() const override;
    void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator) override;

private:
    DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool has_legacy);
};

static fs::path TxIndexDBPath() { return gArgs.GetDataDirNet() / "indexes" / "txindex"; }

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    // Bloom filters are built for every key but only consulted by point reads,
    // which iterators bypass: the per-tx hashed ('x') lookups seek with an
    // iterator, and the 's'/'h' point reads are at most one per block against a
    // tiny keyspace. Only the legacy entries' per-tx point lookups benefit, so
    // enable the filters only for databases still containing them.
    DB(n_cache_size, f_memory, f_wipe,
       /*has_legacy=*/!f_memory && !f_wipe && CDBWrapper::HasKeyStartingWith(TxIndexDBPath(), txindex::DB_TXINDEX))
{}

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe, bool has_legacy) :
    BaseIndex::DB(TxIndexDBPath(), n_cache_size, f_memory, f_wipe, /*f_obfuscate=*/false, /*f_bloom=*/has_legacy),
    m_hasher{ReadOrCreateTxidHasher(*this)},
    m_has_legacy{has_legacy}
{}

CBlockLocator TxIndex::DB::ReadBestBlock() const
{
    CBlockLocator locator;
    if (Read(txindex::DB_BEST_BLOCK_V2, locator)) {
        return locator;
    }
    // If we don't have a locator yet, start from the legacy best block.
    return BaseIndex::DB::ReadBestBlock();
}

void TxIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(txindex::DB_BEST_BLOCK_V2, locator);
}

void TxIndex::DB::WriteTxs(const interfaces::BlockInfo& block)
{
    // A block may be submitted again after it was already indexed, e.g. when it
    // reconnects after a reorg or is re-processed after an unclean shutdown. It
    // keeps its original sequence number, so skip it to avoid duplicate entries.
    if (Exists(txindex::BlockHashKey{block.hash})) return;

    uint32_t block_seq{0};
    Read(txindex::DB_NEXT_BLOCK_SEQ, block_seq);

    CDBBatch batch(*this);
    batch.Write(txindex::BlockHashKey{block.hash}, block_seq);
    batch.Write(txindex::BlockSeqKey{block_seq}, block.hash);
    batch.Write(txindex::DB_NEXT_BLOCK_SEQ, block_seq + 1);
    uint32_t tx_offset_in_block{txindex::BLOCK_HEADER_SIZE + GetSizeOfCompactSize(block.data->vtx.size())};
    for (const auto& tx : block.data->vtx) {
        const txindex::DBKey key{txindex::CreateKeyPrefix(m_hasher, tx->GetHash()),
                                 txindex::BlockTxPosition{block_seq, tx_offset_in_block}};
        batch.Write(key, txindex::EMPTY_VALUE);
        tx_offset_in_block += tx->ComputeTotalSize();
    }
    WriteBatch(batch);
}

TxIndex::TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txindex", "txidx"), m_db(std::make_unique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{
    if (m_db->m_has_legacy) {
        LogInfo("txindex contains entries in the legacy format, which uses excessive disk space. "
                "To reclaim disk space, stop the node, delete %s and restart to rebuild the index.",
                fs::PathToString(TxIndexDBPath()));
    }
}

TxIndex::~TxIndex() = default;

bool TxIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (block.height == 0) return true;

    assert(block.data);
    m_db->WriteTxs(block);
    return true;
}

BaseIndex::DB& TxIndex::GetDB() const { return *m_db; }

std::optional<TxIndexResult> TxIndex::FindTx(const Txid& tx_hash) const
{
    struct Candidate {
        FlatFilePos tx_position;
        uint256 block_hash;
        uint32_t block_seq;
        //! Whether this candidate's block is currently in the active chain.
        //! Active chain candidates are attempted first, so duplicate entries
        //! in both active and stale blocks will always return the active block hash.
        bool in_active_chain;
    };
    std::vector<Candidate> candidates;
    {
        std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
        const txindex::TxHashKeyPrefix prefix{txindex::CreateKeyPrefix(m_db->m_hasher, tx_hash)};
        txindex::DBKey key{prefix, {}};
        for (it->Seek(key); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
            uint256 candidate_block_hash;
            if (!m_db->Read(txindex::BlockSeqKey{key.pos.block_seq}, candidate_block_hash)) {
                LogWarning("Block sequence %u not found for txid %s", key.pos.block_seq, tx_hash.ToString());
                continue;
            }
            LOCK(cs_main);
            const CBlockIndex* block_index{m_chainstate->m_blockman.LookupBlockIndex(candidate_block_hash)};
            if (!block_index) {
                LogWarning("Block index entry %s not found for txid %s", candidate_block_hash.ToString(), tx_hash.ToString());
                continue;
            }
            if (!(block_index->nStatus & BLOCK_HAVE_DATA)) continue;
            const FlatFilePos tx_position{block_index->nFile, block_index->nDataPos + key.pos.tx_offset_in_block};
            candidates.emplace_back(tx_position, candidate_block_hash, key.pos.block_seq, m_chainstate->m_chain.Contains(*block_index));
        }
    }

    // Prefer active-chain matches, then later-connected blocks.
    std::ranges::sort(candidates, std::greater{}, [](const Candidate& c) {
        return std::pair{c.in_active_chain, c.block_seq};
    });

    for (const auto& candidate : candidates) {
        AutoFile file{m_chainstate->m_blockman.OpenBlockFile(candidate.tx_position, /*fReadOnly=*/true)};
        if (file.IsNull()) {
            LogWarning("OpenBlockFile failed for txid %s", tx_hash.ToString());
            continue;
        }
        CTransactionRef tx;
        try {
            file >> TX_WITH_WITNESS(tx);
        } catch (const std::exception& e) {
            LogWarning("Deserialize or I/O error - %s", e.what());
            continue;
        }
        if (tx->GetHash() == tx_hash) {
            return TxIndexResult{candidate.block_hash, std::move(tx)};
        }
    }
    // Fall back to legacy if no hashed entry matched. This makes misses pay an
    // extra lookup, but keeps existing full-txid entries readable after upgrade.
    return m_db->m_has_legacy ? FindLegacyTx(tx_hash) : std::nullopt;
}

std::optional<TxIndexResult> TxIndex::FindLegacyTx(const Txid& tx_hash) const
{
    CDiskTxPos postx;
    if (!m_db->Read(txindex::LegacyTxKey(tx_hash), postx)) {
        return std::nullopt;
    }

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(postx, /*fReadOnly=*/true)};
    if (file.IsNull()) {
        LogError("OpenBlockFile failed");
        return std::nullopt;
    }
    CBlockHeader header;
    CTransactionRef tx;
    try {
        file >> header;
        file.seek(postx.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(tx);
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s", e.what());
        return std::nullopt;
    }
    if (tx->GetHash() != tx_hash) {
        LogError("txid mismatch");
        return std::nullopt;
    }
    return TxIndexResult{header.GetHash(), std::move(tx)};
}
