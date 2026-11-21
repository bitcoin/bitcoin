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

std::unique_ptr<TxoScriptIndex> g_txoscriptindex;

namespace {
SipHasher13UJ ReadOrCreateHasher(CDBWrapper& db)
{
    std::pair<uint64_t, uint64_t> salt;
    if (!db.Read(txoscriptindex::DB_SPK_HASH_SALT, salt)) {
        FastRandomContext rng{};
        salt = {rng.rand64(), rng.rand64()};
        db.Write(txoscriptindex::DB_SPK_HASH_SALT, salt, /*fSync=*/true);
    }
    return SipHasher13UJ{salt.first, salt.second};
}

} // namespace

class TxoScriptIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t cache_size, bool memory, bool wipe)
        : BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "txoscriptindex", cache_size, memory, wipe, /*f_obfuscate=*/false, /*f_bloom=*/false),
          m_hasher{ReadOrCreateHasher(*this)}
    {}

    const SipHasher13UJ m_hasher;

    void WriteEntries(const interfaces::BlockInfo& block, bool erase = false)
    {
        assert(block.data);
        assert(block.height >= 0);
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

        CDBBatch batch{*this};
        for (const auto& [prefix, value] : entries) {
            const txoscriptindex::DBKey key{prefix, static_cast<uint32_t>(block.height)};
            if (erase) {
                batch.Erase(key);
            } else {
                batch.Write(key, value);
            }
        }
        WriteBatch(batch);
    }
};

TxoScriptIndex::TxoScriptIndex(std::unique_ptr<interfaces::Chain> chain, size_t cache_size, bool memory, bool wipe)
    : BaseIndex(std::move(chain), "txoscriptindex", "txoscriptidx"), m_db{std::make_unique<DB>(cache_size, memory, wipe)}
{}

TxoScriptIndex::~TxoScriptIndex() = default;

interfaces::Chain::NotifyOptions TxoScriptIndex::CustomOptions()
{
    interfaces::Chain::NotifyOptions options;
    options.disconnect_data = true;
    return options;
}

bool TxoScriptIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    m_db->WriteEntries(block);
    return true;
}

bool TxoScriptIndex::CustomRemove(const interfaces::BlockInfo& block)
{
    m_db->WriteEntries(block, /*erase=*/true);
    return true;
}

BaseIndex::DB& TxoScriptIndex::GetDB() const { return *m_db; }

util::Expected<std::vector<ScriptHistoryMatch>, std::string> TxoScriptIndex::Find(const uint256& script_hash, int32_t start_height, int32_t end_height, uint64_t& remaining_bytes) const
{
    if (start_height < 0 || end_height < start_height) return {};

    const txoscriptindex::HashPrefix prefix{txoscriptindex::CreateKeyPrefix(m_db->m_hasher, script_hash)};
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
        if (!it->GetKey(key)) return util::Unexpected("Invalid txoscriptindex key");
        const uint64_t key_bytes{GetSerializeSize(key)};
        if (key_bytes > remaining_bytes) return budget_error(key);
        remaining_bytes -= key_bytes;
        if (key.hash_prefix != prefix || key.height > static_cast<uint32_t>(end_height)) break;

        txoscriptindex::DBValue value;
        if (!it->GetValue(value)) return util::Unexpected("Invalid txoscriptindex value");
        const uint64_t value_bytes{GetSerializeSize(value)};
        if (value_bytes > remaining_bytes) return budget_error(key);
        remaining_bytes -= value_bytes;

        FlatFilePos block_pos;
        uint256 block_hash;
        const int32_t height{static_cast<int32_t>(key.height)};
        const CBlockIndex* block_index;
        {
            LOCK(cs_main);
            block_index = m_chainstate->m_chain[key.height];
            if (!block_index) continue;
            block_hash = block_index->GetBlockHash();
            if (block_index->nStatus & BLOCK_HAVE_DATA) {
                block_pos = FlatFilePos{block_index->nFile, block_index->nDataPos};
            } else {
                matches.emplace_back(/*tx=*/nullptr, block_hash, height, std::vector<uint32_t>{});
                continue;
            }
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
                matches.emplace_back(/*tx=*/nullptr, block_hash, height, std::vector<uint32_t>{});
                continue;
            }
            return util::Unexpected("Unable to read txoscriptindex transaction in block " + block_hash.ToString() + ": OpenBlockFile failed");
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
                matches.emplace_back(std::move(tx), block_hash, height, std::move(outputs), offset);
            }
        } catch (const std::exception& e) {
            return util::Unexpected("Unable to read txoscriptindex transaction in block " + block_hash.ToString() + ": " + e.what());
        }
    }
    return matches;
}
