// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <index/txospenderindex.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <validation.h>

// LeveLDB key prefix. We only have one key for now but it will make it easier to add others if needed.
constexpr uint8_t DB_TXOSPENDERINDEX{'s'};

std::unique_ptr<TxoSpenderIndex> g_txospenderindex;

/** Access to the txo spender index database (indexes/txospenderindex/) */
class TxoSpenderIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    bool WriteSpenderInfos(const std::vector<std::pair<COutPoint, uint256>>& items);
    bool EraseSpenderInfos(const std::vector<COutPoint>& items);
};

TxoSpenderIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "txospenderindex", n_cache_size, f_memory, f_wipe)
{
}

TxoSpenderIndex::TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txospenderindex")
    , m_db(std::make_unique<TxoSpenderIndex::DB>(n_cache_size, f_memory, f_wipe))
{
}

TxoSpenderIndex::~TxoSpenderIndex() = default;

bool TxoSpenderIndex::DB::WriteSpenderInfos(const std::vector<std::pair<COutPoint, uint256>>& items)
{
    CDBBatch batch(*this);
    for (const auto& [outpoint, hash] : items) {
        batch.Write(std::pair{DB_TXOSPENDERINDEX, outpoint}, hash);
    }
    return WriteBatch(batch);
}

bool TxoSpenderIndex::DB::EraseSpenderInfos(const std::vector<COutPoint>& items)
{
    CDBBatch batch(*this);
    for (const auto& outpoint : items) {
        batch.Erase(std::pair{DB_TXOSPENDERINDEX, outpoint});
    }
    return WriteBatch(batch);
}

bool TxoSpenderIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    std::vector<std::pair<COutPoint, uint256>> items;
    items.reserve(block.data->vtx.size());

    for (const auto& tx : block.data->vtx) {
        if (tx->IsCoinBase()) {
            continue;
        }
        for (const auto& input : tx->vin) {
            items.emplace_back(input.prevout, tx->GetHash());
        }
    }
    return m_db->WriteSpenderInfos(items);
}

bool TxoSpenderIndex::CustomRewind(const interfaces::BlockRef& current_tip, const interfaces::BlockRef& new_tip)
{
    LOCK(cs_main);
    const CBlockIndex* iter_tip{m_chainstate->m_blockman.LookupBlockIndex(current_tip.hash)};
    const CBlockIndex* new_tip_index{m_chainstate->m_blockman.LookupBlockIndex(new_tip.hash)};

    do {
        CBlock block;
        if (!m_chainstate->m_blockman.ReadBlock(block, *iter_tip)) {
            LogError("Failed to read block %s from disk\n", iter_tip->GetBlockHash().ToString());
            return false;
        }
        std::vector<COutPoint> items;
        items.reserve(block.vtx.size());
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                continue;
            }
            for (const auto& input : tx->vin) {
                items.emplace_back(input.prevout);
            }
        }
        if (!m_db->EraseSpenderInfos(items)) {
            LogError("Failed to erase indexed data for disconnected block %s from disk\n", iter_tip->GetBlockHash().ToString());
            return false;
        }

        iter_tip = iter_tip->GetAncestor(iter_tip->nHeight - 1);
    } while (new_tip_index != iter_tip);

    return true;
}

std::optional<Txid> TxoSpenderIndex::FindSpender(const COutPoint& txo) const
{
    uint256 tx_hash_out;
    if (m_db->Read(std::pair{DB_TXOSPENDERINDEX, txo}, tx_hash_out)) {
        return Txid::FromUint256(tx_hash_out);
    }
    return std::nullopt;
}

BaseIndex::DB& TxoSpenderIndex::GetDB() const { return *m_db; }
