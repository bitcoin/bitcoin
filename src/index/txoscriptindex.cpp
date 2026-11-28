// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txoscriptindex.h>

#include <chain.h>
#include <common/args.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/txoscriptindex_key.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/fs.h>
#include <validation.h>

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace {
SipHasher13UJ ReadOrCreateHasher(CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    if (!db.Read(txoscriptindex::DB_SPK_HASH_SALT, salt)) {
        if (!db.IsEmpty()) {
            throw dbwrapper_error("txoscriptindex database corrupted. Rebuild the index.");
        }
        FastRandomContext rng{};
        salt = {rng.rand64(), rng.rand64()};
        db.Write(txoscriptindex::DB_SPK_HASH_SALT, salt, /*fSync=*/true);
    }
    return SipHasher13UJ{salt.first, salt.second};
}

} // namespace

TxoScriptIndex::TxoScriptIndex(std::unique_ptr<interfaces::Chain> chain, size_t cache_size, bool memory, bool wipe)
    : BaseIndex(std::move(chain), "txoscriptindex", "txoscriptidx"),
      m_db{std::make_unique<BaseIndex::DB>(gArgs.GetDataDirNet() / "indexes" / "txoscriptindex", cache_size, memory, wipe, /*f_obfuscate=*/false, /*f_bloom=*/false)},
      m_hasher{ReadOrCreateHasher(*m_db)}
{}

bool TxoScriptIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    assert(block.data);
    if (block.height < 0 || static_cast<uint32_t>(block.height) > txoscriptindex::MAX_HEIGHT) {
        return false;
    }
    const uint32_t height{static_cast<uint32_t>(block.height)};
    uint32_t gen{0};
    uint256 existing_hash;
    while (m_db->Read(txoscriptindex::BlockKey{height, gen}, existing_hash)) {
        // Skip blocks already indexed (e.g. after an unclean shutdown or reorg back).
        if (existing_hash == block.hash) return true;
        ++gen;
    }

    std::map<txoscriptindex::HashPrefix, txoscriptindex::DBValue> entries;
    uint32_t tx_offset{txoscriptindex::BLOCK_HEADER_SIZE + GetSizeOfCompactSize(block.data->vtx.size())};
    for (const auto& tx : block.data->vtx) {
        for (const auto& output : tx->vout) {
            const auto prefix{txoscriptindex::CreateKeyPrefix(m_hasher, txoscriptindex::Sha256ScriptPubKey(output.scriptPubKey))};
            auto& offsets{entries[prefix].offsets};
            if (offsets.empty() || offsets.back() != tx_offset) offsets.push_back(tx_offset);
        }
        tx_offset += tx->ComputeTotalSize();
    }

    CDBBatch batch{*m_db};
    batch.Write(txoscriptindex::BlockKey{height, gen}, block.hash);
    for (const auto& [prefix, value] : entries) {
        batch.Write(txoscriptindex::DBKey{prefix, height, gen}, value);
    }
    m_db->WriteBatch(batch);
    return true;
}


util::Expected<std::vector<ScriptHistoryMatch>, std::string> TxoScriptIndex::Find(const uint256& script_hash, int32_t start_height, int32_t end_height, uint64_t& remaining_bytes) const
{
    if (start_height < 0 || static_cast<uint32_t>(start_height) > txoscriptindex::MAX_HEIGHT || end_height < start_height) return {};

    const txoscriptindex::HashPrefix prefix{txoscriptindex::CreateKeyPrefix(m_hasher, script_hash)};
    const auto budget_error{[&](const txoscriptindex::DBKey& key) {
        std::string error{"Script history index lookup limit exceeded"};
        if (key.hash_prefix == prefix && key.height <= static_cast<uint32_t>(end_height)) {
            error += strprintf(" while scanning block %u", key.height);
        }
        return util::Unexpected(error + ". Narrow the range with start_height/end_height or the descriptor range.");
    }};
    std::vector<ScriptHistoryMatch> matches;
    std::unique_ptr<AutoFile> file;
    int32_t file_num{-1};
    const std::unique_ptr<CDBIterator> it{m_db->NewIterator()};
    txoscriptindex::DBKey key{prefix, static_cast<uint32_t>(start_height)};
    for (it->Seek(key); it->Valid(); it->Next()) {
        uint8_t key_prefix;
        if (!it->GetKey(key_prefix) || key_prefix != txoscriptindex::DB_TXOSCRIPTINDEX) break;
        if (!it->GetKey(key)) return util::Unexpected("Invalid txoscriptindex key");
        const uint64_t key_bytes{GetSerializeSize(key)};
        if (key_bytes > remaining_bytes) return budget_error(key);
        remaining_bytes -= key_bytes;
        if (key.hash_prefix != prefix || key.height > static_cast<uint32_t>(end_height)) break;

        const txoscriptindex::BlockKey block_key{key.height, key.gen};
        uint256 row_block_hash;
        if (!m_db->Read(block_key, row_block_hash)) {
            return util::Unexpected(strprintf("Corrupt txoscriptindex: block entry missing at height %u", key.height));
        }
        const uint64_t block_entry_bytes{GetSerializeSize(block_key) + GetSerializeSize(row_block_hash)};
        if (block_entry_bytes > remaining_bytes) return budget_error(key);
        remaining_bytes -= block_entry_bytes;

        FlatFilePos block_pos;
        bool pruned{false};
        const int32_t height{static_cast<int32_t>(key.height)};
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_chainstate->m_chain[key.height];
            if (!block_index || block_index->GetBlockHash() != row_block_hash) continue;
            if (block_index->nStatus & BLOCK_HAVE_DATA) {
                block_pos = FlatFilePos{block_index->nFile, block_index->nDataPos};
            } else {
                pruned = true;
            }
        }

        txoscriptindex::DBValue value;
        if (!it->GetValue(value)) return util::Unexpected("Invalid txoscriptindex value");
        const uint64_t value_bytes{GetSerializeSize(value)};
        if (value_bytes > remaining_bytes) return budget_error(key);
        remaining_bytes -= value_bytes;

        if (pruned) {
            matches.emplace_back(/*tx=*/nullptr, row_block_hash, height, std::vector<uint32_t>{});
            continue;
        }
        if (!file || file_num != block_pos.nFile) {
            file.reset(new AutoFile{m_chainstate->m_blockman.OpenBlockFile(block_pos, /*fReadOnly=*/true)});
            file_num = block_pos.nFile;
        }
        if (file->IsNull()) {
            file.reset();
            // Pruning may have removed the file after the availability check.
            LOCK(cs_main);
            if (m_chainstate->m_chain.Contains(*block_index) && m_chainstate->m_blockman.IsBlockPruned(*block_index)) {
                matches.emplace_back(/*tx=*/nullptr, row_block_hash, height, std::vector<uint32_t>{});
                continue;
            }
            return util::Unexpected("Unable to read txoscriptindex transaction in block " + row_block_hash.ToString() + ": OpenBlockFile failed");
        }
        try {
            for (const uint32_t offset : value.offsets) {
                file->seek(uint64_t{block_pos.nPos} + offset, SEEK_SET);
                auto tx{std::make_shared<const CTransaction>(deserialize, TX_WITH_WITNESS, *file)};
                const uint64_t tx_bytes{tx->ComputeTotalSize()};
                if (tx_bytes > remaining_bytes) return budget_error(key);
                remaining_bytes -= tx_bytes;
                std::vector<uint32_t> outputs;
                for (const uint32_t index : std::views::iota(uint32_t{0}, tx->vout.size())) {
                    if (txoscriptindex::Sha256ScriptPubKey(tx->vout[index].scriptPubKey) == script_hash) {
                        outputs.push_back(index);
                    }
                }
                if (outputs.empty()) continue;
                matches.emplace_back(std::move(tx), row_block_hash, height, std::move(outputs), offset);
            }
        } catch (const std::exception& e) {
            return util::Unexpected("Unable to read txoscriptindex transaction in block " + row_block_hash.ToString() + ": " + e.what());
        }
    }
    return matches;
}
