// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/spentindex.h>

#include <chain.h>
#include <chainparams.h>
#include <index/addressindex_util.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <tinyformat.h>
#include <undo.h>
#include <util/system.h>
#include <validation.h>

constexpr uint8_t DB_SPENTINDEX{'p'};

SpentIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "spentindex", n_cache_size, f_memory, f_wipe)
{
}

bool SpentIndex::DB::WriteBatch(const std::vector<CSpentIndexEntry>& entries)
{
    CDBBatch batch(*this);
    for (const auto& [key, value] : entries) {
        if (value.IsNull()) {
            // Null value means delete entry (used during disconnect)
            batch.Erase(std::make_pair(DB_SPENTINDEX, key));
        } else {
            batch.Write(std::make_pair(DB_SPENTINDEX, key), value);
        }
    }
    return CDBWrapper::WriteBatch(batch);
}

bool SpentIndex::DB::ReadSpentIndex(const CSpentIndexKey& key, CSpentIndexValue& value)
{
    return Read(std::make_pair(DB_SPENTINDEX, key), value);
}

bool SpentIndex::DB::EraseSpentIndex(const std::vector<CSpentIndexKey>& keys)
{
    CDBBatch batch(*this);
    for (const auto& key : keys) {
        batch.Erase(std::make_pair(DB_SPENTINDEX, key));
    }
    return CDBWrapper::WriteBatch(batch);
}

SpentIndex::SpentIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex(std::move(chain), "spentindex"),
    m_db(std::make_unique<SpentIndex::DB>(n_cache_size, f_memory, f_wipe))
{
}

SpentIndex::~SpentIndex() = default;

bool SpentIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Skip genesis block (no inputs to index)
    if (block.height == 0) {
        return true;
    }

    // pindex variable gives indexing code access to node internals. It
    // will be removed in upcoming commit
    const CBlockIndex* pindex = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(block.hash));

    // Read undo data for this block to get information about spent outputs
    CBlockUndo blockundo;
    if (!node::UndoReadFromDisk(blockundo, pindex)) {
        LogError("%s: Failed to read undo data for block %s at height %d\n", __func__,
                 block.hash.ToString(), block.height);
        return false;
    }

    std::vector<CSpentIndexEntry> entries;

    // Process each non-coinbase transaction
    // blockundo.vtxundo[i] corresponds to block.vtx[i+1] (coinbase is skipped in undo data)
    if (blockundo.vtxundo.size() != block.data->vtx.size() - 1) {
        LogError("%s: Undo data size mismatch for block %s (expected %zu, got %zu)\n", __func__,
                 block.hash.ToString(), block.data->vtx.size() - 1, blockundo.vtxundo.size());
        return false;
    }

    for (size_t i = 0; i < blockundo.vtxundo.size(); i++) {
        const CTransactionRef& tx = block.data->vtx[i + 1]; // +1 to skip coinbase
        const CTxUndo& txundo = blockundo.vtxundo[i];
        const uint256 txhash = tx->GetHash();

        // Process each input
        if (tx->vin.size() != txundo.vprevout.size()) {
            LogError("%s: Undo data mismatch for tx %s\n", __func__, txhash.ToString());
            return false;
        }

        for (size_t j = 0; j < tx->vin.size(); j++) {
            const CTxIn& input = tx->vin[j];
            const Coin& coin = txundo.vprevout[j];
            const CTxOut& prevout = coin.out;

            AddressType address_type{AddressType::UNKNOWN};
            uint160 address_bytes;
            AddressBytesFromScript(prevout.scriptPubKey, address_type, address_bytes);

            // Create spent index entry: spent output -> spending tx info
            CSpentIndexKey key(input.prevout.hash, input.prevout.n);
            CSpentIndexValue value(txhash, j, block.height, prevout.nValue, address_type, address_bytes);

            entries.emplace_back(key, value);
        }
    }

    return m_db->WriteBatch(entries);
}

bool SpentIndex::CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip)
{
    const CBlockIndex* current_tip_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(current_tip.hash));
    const CBlockIndex* new_tip_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(new_tip.hash));
    assert(current_tip_index->GetAncestor(new_tip_index->nHeight) == new_tip_index);

    // Erase spent index entries for blocks being rewound
    for (const CBlockIndex* pindex = current_tip_index; pindex != new_tip_index; pindex = pindex->pprev) {
        // Skip genesis block
        if (pindex->nHeight == 0) continue;

        // Read block to get transactions
        CBlock block;
        if (!node::ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
            LogError("%s: Failed to read block %s from disk during rewind\n", __func__,
                     pindex->GetBlockHash().ToString());
            return false;
        }

        std::vector<CSpentIndexKey> keys_to_erase;

        // Process each non-coinbase transaction
        for (size_t i = 1; i < block.vtx.size(); i++) {
            const CTransactionRef& tx = block.vtx[i];

            // Erase spent index entries for each input
            for (const CTxIn& input : tx->vin) {
                CSpentIndexKey key(input.prevout.hash, input.prevout.n);
                keys_to_erase.push_back(key);
            }
        }

        if (!keys_to_erase.empty() && !m_db->EraseSpentIndex(keys_to_erase)) {
            LogError("%s: Failed to erase spent index during rewind\n", __func__);
            return false;
        }
    }

    CDBBatch locator_batch(GetDB());
    WriteRewindLocator(locator_batch, new_tip_index);
    return GetDB().WriteBatch(locator_batch);
}

BaseIndex::DB& SpentIndex::GetDB() const { return *m_db; }

bool SpentIndex::GetSpentInfo(const CSpentIndexKey& key, CSpentIndexValue& value) const
{
    return m_db->ReadSpentIndex(key, value);
}
