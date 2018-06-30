// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>
#include <init.h>
#include <ui_interface.h>
#include <util.h>
#include <validation.h>

#include <insight/csindex.h>
#include <script/script.h>
#include <script/interpreter.h>
#include <script/ismine.h>
#include <key_io.h>

#include <boost/thread.hpp>

constexpr char DB_BEST_BLOCK = 'B';
constexpr char DB_TXINDEX = 't';
constexpr char DB_TXINDEX_BLOCK = 'T';

//constexpr char DB_TXINDEX_CSOUTPUT = 'O';
//constexpr char DB_TXINDEX_CSLINK = 'L';

std::unique_ptr<TxIndex> g_txindex;

struct CDiskTxPos : public CDiskBlockPos
{
    unsigned int nTxOffset; // after header

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITEAS(CDiskBlockPos, *this);
        READWRITE(VARINT(nTxOffset));
    }

    CDiskTxPos(const CDiskBlockPos &blockIn, unsigned int nTxOffsetIn) : CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
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

    if (!BaseIndex::Init()) {
        return false;
    }

    // Set m_best_block_index to the last cs_indexed block if lower
    if (m_cs_index) {
        CBlockLocator locator;
        if (!GetDB().Read(DB_TXINDEX_CSBESTBLOCK, locator)) {
            locator.SetNull();
        }
        CBlockIndex *best_cs_block_index = FindForkInGlobalIndex(chainActive, locator);

        if (best_cs_block_index != chainActive.Tip()) {
            m_synced = false;
            if (m_best_block_index.load()->nHeight > best_cs_block_index->nHeight) {
                LogPrintf("Setting txindex best block back to %d to sync csindex.\n", best_cs_block_index->nHeight);
                m_best_block_index = best_cs_block_index;
            }
        }
    }

    return true;
}

bool TxIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    if (m_cs_index) {
        IndexCSOutputs(block, pindex);
    }
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos>> vPos;
    vPos.reserve(block.vtx.size());
    for (const auto& tx : block.vtx) {
        vPos.emplace_back(tx->GetHash(), pos);
        pos.nTxOffset += ::GetSerializeSize(*tx, SER_DISK, CLIENT_VERSION);
    }
    return m_db->WriteTxs(vPos);
}

bool TxIndex::EraseBlock(const CBlock& block)
{
    if (!m_cs_index) {
        return true;
    }

    std::set<COutPoint> erasedCSOuts;
    CDBBatch batch(*m_db);
    for (const auto& tx : block.vtx) {
        int n = -1;
        for (const auto &o : tx->vpout) {
            n++;
            if (!o->IsType(OUTPUT_STANDARD)) {
                continue;
            }
            const CScript *ps = o->GetPScriptPubKey();
            if (!ps->StartsWithICS()) {
                continue;
            }

            ColdStakeIndexOutputKey ok(tx->GetHash(), n);
            batch.Erase(ok);
            erasedCSOuts.insert(COutPoint(ok.m_txnid, ok.m_n));
        }
        for (const auto &in : tx->vin) {
            ColdStakeIndexOutputKey ok(in.prevout.hash, in.prevout.n);
            ColdStakeIndexOutputValue ov;

            if (erasedCSOuts.count(in.prevout)) {
                continue;
            }
            if (m_db->Read(ok, ov)) {
                ov.m_spend_height = -1;
                ov.m_spend_txid.SetNull();
                batch.Write(std::make_pair(DB_TXINDEX_CSOUTPUT, ok), ov);
            }
        }
    }

    if (!m_db->WriteBatch(batch)) {
        return error("%s: WriteBatch failed.", __func__);
    }

    return true;
}

bool TxIndex::IndexCSOutputs(const CBlock& block, const CBlockIndex* pindex)
{
    CDBBatch batch(*m_db);
    std::map<ColdStakeIndexOutputKey, ColdStakeIndexOutputValue> newCSOuts;
    std::map<ColdStakeIndexLinkKey, std::vector<ColdStakeIndexOutputKey> > newCSLinks;
    for (const auto& tx : block.vtx) {
        int n = -1;
        for (const auto &o : tx->vpout) {
            n++;
            if (!o->IsType(OUTPUT_STANDARD)) {
                continue;
            }
            const CScript *ps = o->GetPScriptPubKey();
            if (!ps->StartsWithICS()) {
                continue;
            }

            CScript scriptStake, scriptSpend;
            if (!SplitConditionalCoinstakeScript(*ps, scriptStake, scriptSpend)) {
                continue;
            }

            std::vector<valtype> vSolutions;

            ColdStakeIndexOutputKey ok;
            ColdStakeIndexOutputValue ov;
            ColdStakeIndexLinkKey lk;
            lk.m_height = pindex->nHeight;

            if (!Solver(scriptStake, lk.m_stake_type, vSolutions)) {
                LogPrint(BCLog::COINDB, "%s: Failed to parse scriptStake.\n", __func__);
                continue;
            }

            if (m_cs_index_whitelist.size() > 0
                && !m_cs_index_whitelist.count(vSolutions[0])) {
                continue;
            }

            if (lk.m_stake_type == TX_PUBKEYHASH) {
                memcpy(lk.m_stake_id.begin(), vSolutions[0].data(), 20);
            } else
            if (lk.m_stake_type == TX_PUBKEYHASH256) {
                lk.m_stake_id = CKeyID256(uint256(vSolutions[0]));
            } else {
                LogPrint(BCLog::COINDB, "%s: Ignoring unexpected stakescript type=%d.\n", __func__, lk.m_stake_type);
                continue;
            };

            if (!Solver(scriptSpend, lk.m_spend_type, vSolutions)) {
                LogPrint(BCLog::COINDB, "%s: Failed to parse spendscript.\n", __func__);
                continue;
            }

            if (lk.m_spend_type == TX_PUBKEYHASH || lk.m_spend_type == TX_SCRIPTHASH)
            {
                memcpy(lk.m_spend_id.begin(), vSolutions[0].data(), 20);
            } else
            if (lk.m_spend_type == TX_PUBKEYHASH256 || lk.m_spend_type == TX_SCRIPTHASH256) {
                lk.m_spend_id = CKeyID256(uint256(vSolutions[0]));
            } else {
                LogPrint(BCLog::COINDB, "%s: Ignoring unexpected spendscript type=%d.\n", __func__, lk.m_spend_type);
                continue;
            }

            ok.m_txnid = tx->GetHash();
            ok.m_n = n;
            ov.m_value = o->GetValue();

            if (tx->IsCoinStake()) {
                ov.m_flags |= CSI_FROM_STAKE;
            }

            newCSOuts[ok] = ov;
            newCSLinks[lk].push_back(ok);
        }

        for (const auto &in : tx->vin) {
            if (in.IsAnonInput()) {
                continue;
            }
            ColdStakeIndexOutputKey ok(in.prevout.hash, in.prevout.n);
            ColdStakeIndexOutputValue ov;

            auto it = newCSOuts.find(ok);
            if (it != newCSOuts.end()) {
                it->second.m_spend_height = pindex->nHeight;
                it->second.m_spend_txid = tx->GetHash();
            } else
            if (m_db->Read(std::make_pair(DB_TXINDEX_CSOUTPUT, ok), ov)) {
                ov.m_spend_height = pindex->nHeight;
                ov.m_spend_txid = tx->GetHash();
                batch.Write(std::make_pair(DB_TXINDEX_CSOUTPUT, ok), ov);
            }
        }
    }

    for (const auto &it : newCSOuts) {
        batch.Write(std::make_pair(DB_TXINDEX_CSOUTPUT, it.first), it.second);
    }
    for (const auto &it : newCSLinks) {
        batch.Write(std::make_pair(DB_TXINDEX_CSLINK, it.first), it.second);
    }

    batch.Write(DB_TXINDEX_CSBESTBLOCK, chainActive.GetLocator(pindex));

    if (!m_db->WriteBatch(batch)) {
        return error("%s: WriteBatch failed.", __func__);
    }

    return true;
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

bool TxIndex::FindTx(const uint256& tx_hash, CBlockHeader& header, CTransactionRef& tx) const
{
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
        return false;
    }

    CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
    if (file.IsNull()) {
        return error("%s: OpenBlockFile failed", __func__);
    }
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
    return true;
}

bool TxIndex::AppendCSAddress(std::string addr)
{
    CTxDestination dest = DecodeDestination(addr);

    if (dest.type() == typeid(CKeyID)) {
        CKeyID id = boost::get<CKeyID>(dest);
        valtype vSolution;
        vSolution.resize(20);
        memcpy(vSolution.data(), id.begin(), 20);
        m_cs_index_whitelist.insert(vSolution);
        return true;
    }

    if (dest.type() == typeid(CKeyID256)) {
        CKeyID256 id256 = boost::get<CKeyID256>(dest);
        valtype vSolution;
        vSolution.resize(32);
        memcpy(vSolution.data(), id256.begin(), 32);
        m_cs_index_whitelist.insert(vSolution);
        return true;
    }

    return error("%s: Failed to parse address %s.", __func__, addr);
}
