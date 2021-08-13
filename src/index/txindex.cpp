// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>
#include <shutdown.h>
#include <ui_interface.h>
#include <util/system.h>
#include <validation.h>

#include <boost/thread.hpp>

constexpr char DB_BEST_BLOCK = 'B';
constexpr char DB_TXINDEX = 't';
constexpr char DB_TXINDEX_BLOCK = 'T';

std::unique_ptr<TxIndex> g_txindex;

struct CDiskTxPos : public FlatFilePos
{
    unsigned int nTxOffset; // after header

    SERIALIZE_METHODS(CDiskTxPos, obj)
    {
        READWRITEAS(FlatFilePos, obj);
        READWRITE(VARINT(obj.nTxOffset));
    }

    CDiskTxPos(const FlatFilePos &blockIn, unsigned int nTxOffsetIn) : FlatFilePos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        FlatFilePos::SetNull();
        nTxOffset = 0;
    }
};

/**
 * Access to the txindex database (indexes/txindex/)
 *
 * The database stores a block locator of the chain the database is synced to
 * so that the TxIndex can efficiently determine the point it last stopped at.
 * A locator is used instead of a simple hash of the chain tip because blocks
 * and block index entries may not be flushed to disk until after this database
 * is updated.
 */
class TxIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Read the disk location of the transaction data with the given hash. Returns false if the
    /// transaction hash is not indexed.
    bool ReadTxPos(const uint256& txid, CDiskTxPos& pos) const;

    /// Write a batch of transaction positions to the DB.
    bool WriteTxs(const std::vector<std::pair<uint256, CDiskTxPos>>& v_pos);

    /// Migrate txindex data from the block tree DB, where it may be for older nodes that have not
    /// been upgraded yet to the new database.
    bool MigrateData(CBlockTreeDB& block_tree_db, const CBlockLocator& best_locator);
};

TxIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(GetDataDir() / "indexes" / "txindex", n_cache_size, f_memory, f_wipe)
{}

bool TxIndex::DB::ReadTxPos(const uint256 &txid, CDiskTxPos& pos) const
{
    return Read(std::make_pair(DB_TXINDEX, txid), pos);
}

bool TxIndex::DB::WriteTxs(const std::vector<std::pair<uint256, CDiskTxPos>>& v_pos)
{
    CDBBatch batch(*this);
    for (const auto& tuple : v_pos) {
        batch.Write(std::make_pair(DB_TXINDEX, tuple.first), tuple.second);
    }
    return WriteBatch(batch);
}

/*
 * Safely persist a transfer of data from the old txindex database to the new one, and compact the
 * range of keys updated. This is used internally by MigrateData.
 */
static void WriteTxIndexMigrationBatches(CDBWrapper& newdb, CDBWrapper& olddb,
                                         CDBBatch& batch_newdb, CDBBatch& batch_olddb,
                                         const std::pair<unsigned char, uint256>& begin_key,
                                         const std::pair<unsigned char, uint256>& end_key)
{
    // Sync new DB changes to disk before deleting from old DB.
    newdb.WriteBatch(batch_newdb, /*fSync=*/ true);
    olddb.WriteBatch(batch_olddb);
    olddb.CompactRange(begin_key, end_key);

    batch_newdb.Clear();
    batch_olddb.Clear();
}

bool TxIndex::DB::MigrateData(CBlockTreeDB& block_tree_db, const CBlockLocator& best_locator)
{
    // The prior implementation of txindex was always in sync with block index
    // and presence was indicated with a boolean DB flag. If the flag is set,
    // this means the txindex from a previous version is valid and in sync with
    // the chain tip. The first step of the migration is to unset the flag and
    // write the chain hash to a separate key, DB_TXINDEX_BLOCK. After that, the
    // index entries are copied over in batches to the new database. Finally,
    // DB_TXINDEX_BLOCK is erased from the old database and the block hash is
    // written to the new database.
    //
    // Unsetting the boolean flag ensures that if the node is downgraded to a
    // previous version, it will not see a corrupted, partially migrated index
    // -- it will see that the txindex is disabled. When the node is upgraded
    // again, the migration will pick up where it left off and sync to the block
    // with hash DB_TXINDEX_BLOCK.
    bool f_legacy_flag = false;
    block_tree_db.ReadFlag("txindex", f_legacy_flag);
    if (f_legacy_flag) {
        if (!block_tree_db.Write(DB_TXINDEX_BLOCK, best_locator)) {
            return error("%s: cannot write block indicator", __func__);
        }
        if (!block_tree_db.WriteFlag("txindex", false)) {
            return error("%s: cannot write block index db flag", __func__);
        }
    }

    CBlockLocator locator;
    if (!block_tree_db.Read(DB_TXINDEX_BLOCK, locator)) {
        return true;
    }

    int64_t count = 0;
    LogPrintf("Upgrading txindex database... [0%%]\n");
    uiInterface.ShowProgress(_("Upgrading txindex database"), 0, true);
    int report_done = 0;
    const size_t batch_size = 1 << 24; // 16 MiB

    CDBBatch batch_newdb(*this);
    CDBBatch batch_olddb(block_tree_db);

    std::pair<unsigned char, uint256> key;
    std::pair<unsigned char, uint256> begin_key{DB_TXINDEX, uint256()};
    std::pair<unsigned char, uint256> prev_key = begin_key;

    bool interrupted = false;
    std::unique_ptr<CDBIterator> cursor(block_tree_db.NewIterator());
    for (cursor->Seek(begin_key); cursor->Valid(); cursor->Next()) {
        boost::this_thread::interruption_point();
        if (ShutdownRequested()) {
            interrupted = true;
            break;
        }

        if (!cursor->GetKey(key)) {
            return error("%s: cannot get key from valid cursor", __func__);
        }
        if (key.first != DB_TXINDEX) {
            break;
        }

        // Log progress every 10%.
        if (++count % 256 == 0) {
            // Since txids are uniformly random and traversed in increasing order, the high 16 bits
            // of the hash can be used to estimate the current progress.
            const uint256& txid = key.second;
            uint32_t high_nibble =
                (static_cast<uint32_t>(*(txid.begin() + 0)) << 8) +
                (static_cast<uint32_t>(*(txid.begin() + 1)) << 0);
            int percentage_done = (int)(high_nibble * 100.0 / 65536.0 + 0.5);

            uiInterface.ShowProgress(_("Upgrading txindex database"), percentage_done, true);
            if (report_done < percentage_done/10) {
                LogPrintf("Upgrading txindex database... [%d%%]\n", percentage_done);
                report_done = percentage_done/10;
            }
        }

        CDiskTxPos value;
        if (!cursor->GetValue(value)) {
            return error("%s: cannot parse txindex record", __func__);
        }
        batch_newdb.Write(key, value);
        batch_olddb.Erase(key);

        if (batch_newdb.SizeEstimate() > batch_size || batch_olddb.SizeEstimate() > batch_size) {
            // NOTE: it's OK to delete the key pointed at by the current DB cursor while iterating
            // because LevelDB iterators are guaranteed to provide a consistent view of the
            // underlying data, like a lightweight snapshot.
            WriteTxIndexMigrationBatches(*this, block_tree_db,
                                         batch_newdb, batch_olddb,
                                         prev_key, key);
            prev_key = key;
        }
    }

    // If these final DB batches complete the migration, write the best block
    // hash marker to the new database and delete from the old one. This signals
    // that the former is fully caught up to that point in the blockchain and
    // that all txindex entries have been removed from the latter.
    if (!interrupted) {
        batch_olddb.Erase(DB_TXINDEX_BLOCK);
        batch_newdb.Write(DB_BEST_BLOCK, locator);
    }

    WriteTxIndexMigrationBatches(*this, block_tree_db,
                                 batch_newdb, batch_olddb,
                                 begin_key, key);

    if (interrupted) {
        LogPrintf("[CANCELLED].\n");
        return false;
    }

    uiInterface.ShowProgress("", 100, false);

    LogPrintf("[DONE].\n");
    return true;
}

TxIndex::TxIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : m_db(MakeUnique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

TxIndex::~TxIndex() {}

bool TxIndex::Init()
{
    LOCK(cs_main);

    // Attempt to migrate txindex from the old database to the new one. Even if
    // chain_tip is null, the node could be reindexing and we still want to
    // delete txindex records in the old database.
    if (!m_db->MigrateData(*pblocktree, chainActive.GetLocator())) {
        return false;
    }

    return BaseIndex::Init();
}

bool TxIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (pindex->nHeight == 0) return true;

    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos>> vPos;
    vPos.reserve(block.vtx.size());
    for (const auto& tx : block.vtx) {
        vPos.emplace_back(tx->GetHash(), pos);
        pos.nTxOffset += ::GetSerializeSize(*tx, SER_DISK, CLIENT_VERSION);
    }
    return m_db->WriteTxs(vPos);
}

BaseIndex::DB& TxIndex::GetDB() const { return *m_db; }

bool TxIndex::FindTx(const uint256& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
        return false;
    }

    CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
    if (file.IsNull()) {
        return error("%s: OpenBlockFile failed", __func__);
    }
    CBlockHeader header;
    try {
        file >> header;
        if (fseek(file.Get(), postx.nTxOffset, SEEK_CUR)) {
            return error("%s: fseek(...) failed", __func__);
        }
        file >> tx;
    } catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }
    if (tx->GetHash() != tx_hash) {
        return error("%s: txid mismatch", __func__);
    }
    block_hash = header.GetHash();
    return true;
}

bool TxIndex::HasTx(const uint256& tx_hash) const
{
    CDiskTxPos postx;
    return m_db->ReadTxPos(tx_hash, postx);
}
