// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txospenderindex.h>

#include <chain.h>
#include <common/args.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/block_seq.h>
#include <index/disktxpos.h>
#include <index/txospenderindex_key.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/fs.h>
#include <validation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

/* The database is used to find the spending transaction of a given utxo.
 * For every input of every transaction it stores a key that is a pair(hash prefix of the input outpoint, transaction
 * position) and a zero-byte value. The position is the sequence number of the block and the offset of the transaction
 * in it. To find the spending transaction of an outpoint, we perform a range query on the hash prefix, and for each
 * returned key in the active chain load the transaction and return it if it does spend the provided outpoint.
 * See txospenderindex_key.h for the database layout.
 */

std::unique_ptr<TxoSpenderIndex> g_txospenderindex;

static fs::path TxoSpenderIndexDBPath() { return gArgs.GetDataDirNet() / "indexes" / "txospenderindex" / "db"; }

TxoSpenderIndex::TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txospenderindex", "txospenderidx"),
      m_has_legacy{!f_memory && !f_wipe && CDBWrapper::HasKeyStartingWith(TxoSpenderIndexDBPath(), txospenderindex::DB_LEGACY_SPENDER)},
      m_db{std::make_unique<DB>(TxoSpenderIndexDBPath(), n_cache_size, f_memory, f_wipe, /*f_obfuscate=*/false, /*f_bloom=*/false,
                                /*versioned_locator=*/true)},
      m_hasher{block_seq::ReadOrCreateHasher(*m_db, txospenderindex::DB_OUTPOINT_HASH_SALT)}
{
    if (m_has_legacy) {
        std::pair<uint64_t, uint64_t> legacy_key;
        if (m_db->Read(txospenderindex::DB_LEGACY_SIPHASH_KEY, legacy_key)) {
            m_legacy_hasher.emplace(legacy_key.first, legacy_key.second);
        }
        LogInfo("txospenderindex contains entries in the legacy format, which uses excessive disk space. "
                "To reclaim disk space, stop the node, delete %s and restart to rebuild the index.",
                fs::PathToString(TxoSpenderIndexDBPath()));
    }
}

bool TxoSpenderIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    CDBBatch batch(*m_db);
    // A block that was already indexed keeps its sequence number and entries,
    // so skip it to avoid duplicate entries.
    const auto seq{block_seq::AssignBlockSeq<txospenderindex::DB_BLOCK_SEQ, txospenderindex::DB_BLOCK_HASH>(*m_db, batch, block.hash)};
    if (!seq) return true;

    uint32_t tx_offset_in_block{block_seq::BLOCK_HEADER_SIZE + GetSizeOfCompactSize(block.data->vtx.size())};
    for (const auto& tx : block.data->vtx) {
        if (!tx->IsCoinBase()) {
            for (const auto& input : tx->vin) {
                const txospenderindex::DBKey key{txospenderindex::CreateKeyPrefix(m_hasher, input.prevout),
                                                 block_seq::BlockTxPosition{*seq, tx_offset_in_block}};
                batch.Write(key, block_seq::EMPTY_VALUE);
            }
        }
        tx_offset_in_block += tx->ComputeTotalSize();
    }
    m_db->WriteBatch(batch);
    return true;
}

util::Expected<CTransactionRef, std::string> TxoSpenderIndex::ReadTransaction(const FlatFilePos& pos) const
{
    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(pos, /*fReadOnly=*/true)};
    if (file.IsNull()) {
        return util::Unexpected("cannot open block");
    }
    CTransactionRef tx;
    try {
        file >> TX_WITH_WITNESS(tx);
        return tx;
    } catch (const std::exception& e) {
        return util::Unexpected(e.what());
    }
}

util::Expected<TxoSpender, std::string> TxoSpenderIndex::ReadLegacyTransaction(const CDiskTxPos& tx_pos) const
{
    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, /*fReadOnly=*/true)};
    if (file.IsNull()) {
        return util::Unexpected("cannot open block");
    }
    CBlockHeader header;
    TxoSpender spender;
    try {
        file >> header;
        file.seek(tx_pos.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(spender.tx);
        spender.block_hash = header.GetHash();
        return spender;
    } catch (const std::exception& e) {
        return util::Unexpected(e.what());
    }
}

static bool Spends(const CTransaction& tx, const COutPoint& txo)
{
    return std::ranges::any_of(tx.vin, [&](const CTxIn& input) { return input.prevout == txo; });
}

util::Expected<std::optional<TxoSpender>, std::string> TxoSpenderIndex::FindSpender(const COutPoint& txo) const
{
    const auto prefix{txospenderindex::CreateKeyPrefix(m_hasher, txo)};
    std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
    txospenderindex::DBKey key{prefix, {}};

    // Find all keys that start with the outpoint hash prefix, load the transaction at the location specified in the
    // key if its block is in the active chain, and return it if it does spend the provided outpoint.
    for (it->Seek(key); it->Valid() && it->GetKey(key) && key.hash_prefix == prefix; it->Next()) {
        uint256 block_hash;
        if (!m_db->Read(txospenderindex::BlockSeqKey{key.pos.block_seq}, block_hash)) {
            LogWarning("Block sequence %u not found for outpoint %s:%d", key.pos.block_seq, txo.hash.GetHex(), txo.n);
            continue;
        }
        FlatFilePos tx_pos;
        {
            LOCK(cs_main);
            const CBlockIndex* block_index{m_chainstate->m_blockman.LookupBlockIndex(block_hash)};
            if (!block_index) {
                LogWarning("Block index entry %s not found for outpoint %s:%d", block_hash.ToString(), txo.hash.GetHex(), txo.n);
                continue;
            }
            // Entries of disconnected blocks are kept, so only spends in the active chain count.
            if (!m_chainstate->m_chain.Contains(*block_index)) continue;
            if (!(block_index->nStatus & BLOCK_HAVE_DATA)) {
                return util::Unexpected{strprintf("Block %s with a candidate spending tx for outpoint %s:%d is not available.",
                                                  block_hash.ToString(), txo.hash.GetHex(), txo.n)};
            }
            tx_pos = FlatFilePos{block_index->nFile, block_index->nDataPos + key.pos.tx_offset_in_block};
        }
        const auto tx{ReadTransaction(tx_pos)};
        if (!tx) {
            LogError("Deserialize or I/O error - %s", tx.error());
            return util::Unexpected{strprintf("IO error finding spending tx for outpoint %s:%d.", txo.hash.GetHex(), txo.n)};
        }
        if (Spends(**tx, txo)) return std::optional{TxoSpender{*tx, block_hash}};
    }
    // Fall back to legacy if no hashed entry matched.
    if (m_has_legacy) return FindLegacySpender(txo);
    return std::optional<TxoSpender>{};
}

util::Expected<std::optional<TxoSpender>, std::string> TxoSpenderIndex::FindLegacySpender(const COutPoint& txo) const
{
    if (!m_legacy_hasher) return std::optional<TxoSpender>{};
    const uint64_t prefix{txospenderindex::CreateLegacyKeyPrefix(*m_legacy_hasher, txo)};
    std::unique_ptr<CDBIterator> it(m_db->NewIterator());
    txospenderindex::LegacyDBKey key{prefix, {}};

    for (it->Seek(std::pair{txospenderindex::DB_LEGACY_SPENDER, prefix}); it->Valid() && it->GetKey(key) && key.hash == prefix; it->Next()) {
        if (const auto spender{ReadLegacyTransaction(key.pos)}) {
            if (!Spends(*spender->tx, txo)) continue;
            // As for hashed entries, only spends in the active chain count.
            LOCK(cs_main);
            const CBlockIndex* block_index{m_chainstate->m_blockman.LookupBlockIndex(spender->block_hash)};
            if (block_index && m_chainstate->m_chain.Contains(*block_index)) return std::optional{*spender};
        } else {
            LogError("Deserialize or I/O error - %s", spender.error());
            return util::Unexpected{strprintf("IO error finding spending tx for outpoint %s:%d.", txo.hash.GetHex(), txo.n)};
        }
    }
    return std::optional<TxoSpender>{};
}

BaseIndex::DB& TxoSpenderIndex::GetDB() const { return *m_db; }
