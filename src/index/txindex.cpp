// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>

#include <common/args.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/disktxpos.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/log.h>
#include <validation.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

constexpr uint8_t DB_TXINDEX{'t'};
const std::string DB_BEST_BLOCK_V2{"best_block_v2"};

std::unique_ptr<TxIndex> g_txindex;


/** Access to the txindex database (indexes/txindex/) */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Read the disk location of the transaction data with the given hash. Returns false if the
    /// transaction hash is not indexed.
    bool ReadTxPos(const Txid& txid, CDiskTxPos& pos) const;

    /// Write a block of transaction positions to the DB.
    void WriteTxs(const interfaces::BlockInfo& block);

    CBlockLocator ReadBestBlock() const override;
    void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator) override;
};

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "txindex", n_cache_size, f_memory, f_wipe)
{}

bool TxIndex::DB::ReadTxPos(const Txid& txid, CDiskTxPos& pos) const
{
    return Read(std::make_pair(DB_TXINDEX, txid.ToUint256()), pos);
}

CBlockLocator TxIndex::DB::ReadBestBlock() const
{
    CBlockLocator locator;
    if (Read(DB_BEST_BLOCK_V2, locator)) {
        return locator;
    }
    // If we don't have a locator yet, start from the legacy best block.
    return BaseIndex::DB::ReadBestBlock();
}

void TxIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK_V2, locator);
}

void TxIndex::DB::WriteTxs(const interfaces::BlockInfo& block)
{
    CDBBatch batch(*this);
    CDiskTxPos pos({block.file_number, block.data_pos}, GetSizeOfCompactSize(block.data->vtx.size()));
    for (const auto& tx : block.data->vtx) {
        batch.Write(std::make_pair(DB_TXINDEX, tx->GetHash().ToUint256()), pos);
        pos.nTxOffset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
    }
    WriteBatch(batch);
}

TxIndex::TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txindex", "txidx"), m_db(std::make_unique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

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
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
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
