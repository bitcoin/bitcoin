// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>

#include <clientversion.h>
#include <common/args.h>
#include <index/disktxpos.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/transaction_identifier.h>
#include <validation.h>

constexpr uint8_t DB_TXINDEX{'t'};

std::unique_ptr<TxIndex> g_txindex;


/** Access to the txindex database (indexes/txindex/) */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Read the disk location of the transaction data with the given hash. Returns false if the
    /// transaction hash is not indexed.
    bool ReadTxPos(const Txid& txid, CDiskTxPos& pos) const;

    /// Write a batch of transaction positions to the DB.
    [[nodiscard]] bool WriteTxs(const std::vector<std::pair<Txid, CDiskTxPos>>& v_pos);
};

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "txindex", n_cache_size, f_memory, f_wipe)
{}

bool TxIndex::DB::ReadTxPos(const Txid& txid, CDiskTxPos& pos) const
{
    return Read(std::make_pair(DB_TXINDEX, txid.ToUint256()), pos);
}

bool TxIndex::DB::WriteTxs(const std::vector<std::pair<Txid, CDiskTxPos>>& v_pos)
{
    CDBBatch batch(*this);
    for (const auto& [txid, pos] : v_pos) {
        batch.Write(std::make_pair(DB_TXINDEX, txid.ToUint256()), pos);
    }
    return WriteBatch(batch);
}

TxIndex::TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txindex"), m_db(std::make_unique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

TxIndex::~TxIndex() = default;

bool TxIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (block.height == 0) return true;

    assert(block.data);
    CDiskTxPos pos({block.file_number, block.data_pos}, GetSizeOfCompactSize(block.data->vtx.size()));
    std::vector<std::pair<Txid, CDiskTxPos>> vPos;
    vPos.reserve(block.data->vtx.size());
    for (const auto& tx : block.data->vtx) {
        vPos.emplace_back(tx->GetHash(), pos);
        pos.nTxOffset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
    }
    return m_db->WriteTxs(vPos);
}

BaseIndex::DB& TxIndex::GetDB() const { return *m_db; }

bool TxIndex::FindTx(const Txid& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
        return false;
    }

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(postx, true)};
    if (file.IsNull()) {
        LogError("OpenBlockFile failed");
        return false;
    }
    CBlockHeader header;
    try {
        file >> header;
        file.seek(postx.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(tx);
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s", e.what());
        return false;
    }
    if (tx->GetHash() != tx_hash) {
        LogError("txid mismatch");
        return false;
    }
    block_hash = header.GetHash();
    return true;
}
