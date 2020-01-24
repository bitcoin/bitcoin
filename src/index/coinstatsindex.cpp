// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coins.h>
#include <crypto/muhash.h>
#include <hash.h>
#include <index/coinstatsindex.h>
#include <node/coinstats.h>
#include <serialize.h>
#include <txdb.h>
#include <undo.h>
#include <validation.h>

constexpr char DB_BLOCK_HASH = 's';
constexpr char DB_BLOCK_HEIGHT = 't';
constexpr char DB_MUHASH = 'M';

namespace {

struct DBVal {
    uint256 muhash;
    uint64_t transaction_output_count;
    uint64_t bogo_size;
    CAmount total_amount;
    uint64_t disk_size;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(muhash);
        READWRITE(transaction_output_count);
        READWRITE(bogo_size);
        READWRITE(total_amount);
        READWRITE(disk_size);
    }
};

struct DBHeightKey {
    int height;

    DBHeightKey() : height(0) {}
    explicit DBHeightKey(int height_in) : height(height_in) {}

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        ser_writedata8(s, DB_BLOCK_HEIGHT);
        ser_writedata32be(s, height);
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        char prefix = ser_readdata8(s);
        if (prefix != DB_BLOCK_HEIGHT) {
            throw std::ios_base::failure("Invalid format for coin stats index DB height key");
        }
        height = ser_readdata32be(s);
    }
};

struct DBHashKey {
    uint256 block_hash;

    explicit DBHashKey(const uint256& hash_in) : block_hash(hash_in) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        char prefix = DB_BLOCK_HASH;
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for coin stats index DB hash key");
        }

        READWRITE(block_hash);
    }
};

struct DBMuhash {
    MuHash3072 hash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
    }
};

}; // namespace

std::unique_ptr<CoinStatsIndex> g_coin_stats_index;

CoinStatsIndex::CoinStatsIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
{
    fs::path path = GetDataDir() / "indexes" / "coinstats";
    fs::create_directories(path);

    m_db = MakeUnique<CoinStatsIndex::DB>(path / "db", n_cache_size, f_memory, f_wipe);
}

bool CoinStatsIndex::Init()
{
    if (!m_db->Read(DB_MUHASH, m_muhash)) {
        // Check that the cause of the read failure is that the key does not exist. Any other errors
        // indicate database corruption or a disk failure, and starting the index would cause
        // further corruption.
        if (m_db->Exists(DB_MUHASH)) {
            return error("%s: Cannot read current %s state; index may be corrupted",
                         __func__, GetName());
        }

        // If the DB_MUHASH is not set, initialize empty values
        m_muhash = MuHash3072();
        m_transaction_output_count = 0;
        m_bogo_size = 0;
        m_total_amount = 0;
        m_disk_size = 0;
    }

    return BaseIndex::Init();
}

bool CoinStatsIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    CBlockUndo block_undo;

    // Ignore genesis block
    if (pindex->nHeight > 0) {
        if (!UndoReadFromDisk(block_undo, pindex)) {
            return false;
        }

        std::pair<uint256, DBVal> read_out;
        if (!m_db->Read(DBHeightKey(pindex->nHeight - 1), read_out)) {
            return false;
        }

        uint256 expected_block_hash = pindex->pprev->GetBlockHash();
        if (read_out.first != expected_block_hash) {
            return error("%s: previous block header belongs to unexpected block %s; expected %s",
                         __func__, read_out.first.ToString(), expected_block_hash.ToString());
        }

        bool is_bip30_block = (pindex->nHeight==91722 && pindex->GetBlockHash() == uint256S("0x00000000000271a2dc26e7667f8419f2e15416dc6955e5a6c6cdf3f2574dd08e")) ||
                              (pindex->nHeight==91812 && pindex->GetBlockHash() == uint256S("0x00000000000af0aed4792b1acee3d966af36cf5def14935db8de83d6f9306f2f"));
        MuHash3072 undo_muhash;

        // Add the new utxos created from the block
        for (size_t i = 0; i < block.vtx.size(); ++i) {
            const auto& tx = block.vtx.at(i);

            // Skip duplicate txid coinbase transactions (BIP30).
            if (is_bip30_block && tx->IsCoinBase()) {
                continue;
            }

            for (size_t j = 0; j < tx->vout.size(); ++j) {
                const CTxOut& out = tx->vout[j];
                COutPoint outpoint = COutPoint(tx->GetHash(), j);
                Coin coin = Coin(out, pindex->nHeight, tx->IsCoinBase());

                // Skip unspendable coins
                if (coin.out.scriptPubKey.IsUnspendable()) continue;

                m_muhash *= MuHash3072(GetTruncatedSHA512Hash(outpoint, coin).begin());

                m_transaction_output_count++;
                m_total_amount += coin.out.nValue;
                m_bogo_size += 32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ + 8 /* amount */ +
                               2 /* scriptPubKey len */ + coin.out.scriptPubKey.size() /* scriptPubKey */;
            }

            // The coinbase tx has no undo data since no former output is spent
            if (i > 0) {
                const auto& tx_undo = block_undo.vtxundo.at(i-1);

                for (size_t j = 0; j < tx_undo.vprevout.size(); ++j) {
                    Coin coin = tx_undo.vprevout[j];
                    COutPoint outpoint = COutPoint(tx->vin[j].prevout.hash, tx->vin[j].prevout.n);

                    undo_muhash *= MuHash3072(GetTruncatedSHA512Hash(outpoint, coin).begin());

                    m_transaction_output_count--;
                    m_total_amount -= coin.out.nValue;
                    m_bogo_size -= 32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ + 8 /* amount */ +
                                   2 /* scriptPubKey len */ + coin.out.scriptPubKey.size() /* scriptPubKey */;
                }
            }
        }

        m_muhash /= undo_muhash;
    }

    CCoinsView* coins_view = WITH_LOCK(cs_main, return &ChainstateActive().CoinsDB());
    m_disk_size = coins_view->EstimateSize();

    std::pair<uint256, DBVal> value;
    value.first = pindex->GetBlockHash();
    value.second.muhash = currentHashInternal();
    value.second.disk_size = m_disk_size;
    value.second.transaction_output_count = m_transaction_output_count;
    value.second.bogo_size = m_bogo_size;
    value.second.total_amount = m_total_amount;

    if (!m_db->Write(DBHeightKey(pindex->nHeight), value)) {
        return false;
    }

    if (!m_db->Write(DB_MUHASH, m_muhash)) {
        return false;
    }

    return true;
}

static bool CopyHeightIndexToHashIndex(CDBIterator& db_it, CDBBatch& batch,
                                       const std::string& index_name,
                                       int start_height, int stop_height)
{
    DBHeightKey key(start_height);
    db_it.Seek(key);

    for (int height = start_height; height <= stop_height; ++height) {
        if (!db_it.GetKey(key) || key.height != height) {
            return error("%s: unexpected key in %s: expected (%c, %d)",
                         __func__, index_name, DB_BLOCK_HEIGHT, height);
        }

        std::pair<uint256, DBVal> value;
        if (!db_it.GetValue(value)) {
            return error("%s: unable to read value in %s at key (%c, %d)",
                         __func__, index_name, DB_BLOCK_HEIGHT, height);
        }

        batch.Write(DBHashKey(value.first), std::move(value.second));

        db_it.Next();
    }
    return true;
}

bool CoinStatsIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    CDBBatch batch(*m_db);
    std::unique_ptr<CDBIterator> db_it(m_db->NewIterator());

    {
        LOCK(cs_main);
        CBlockIndex* iter_tip = LookupBlockIndex(current_tip->GetBlockHash());
        auto& consensus_params = Params().GetConsensus();

        do {
            CBlock block;

            if (!ReadBlockFromDisk(block, iter_tip, consensus_params)) {
                return error("%s: Failed to read block %s from disk",
                               __func__, iter_tip->GetBlockHash().ToString());
            }

            ReverseBlock(block, iter_tip);

            iter_tip = iter_tip->GetAncestor(iter_tip->nHeight - 1);
        } while (new_tip != iter_tip);
    }

    // During a reorg, we need to copy all hash digests for blocks that are getting disconnected from the
    // height index to the hash index so we can still find them when the height index entries are
    // overwritten.
    if (!CopyHeightIndexToHashIndex(*db_it, batch, m_name, new_tip->nHeight, current_tip->nHeight)) {
        return false;
    }

    if (!m_db->WriteBatch(batch)) return false;

    return BaseIndex::Rewind(current_tip, new_tip);
}

static bool LookupOne(const CDBWrapper& db, const CBlockIndex* block_index, DBVal& result)
{
    // First check if the result is stored under the height index and the value there matches the
    // block hash. This should be the case if the block is on the active chain.
    std::pair<uint256, DBVal> read_out;
    if (!db.Read(DBHeightKey(block_index->nHeight), read_out)) {
        return false;
    }
    if (read_out.first == block_index->GetBlockHash()) {
        result = std::move(read_out.second);
        return true;
    }

    // If value at the height index corresponds to an different block, the result will be stored in
    // the hash index.
    return db.Read(DBHashKey(block_index->GetBlockHash()), result);
}

bool CoinStatsIndex::LookupStats(const CBlockIndex* block_index, CCoinsStats& coins_stats) const
{
    DBVal entry;
    if (!LookupOne(*m_db, block_index, entry)) {
        return false;
    }

    coins_stats.hashSerialized = entry.muhash;
    coins_stats.nTransactionOutputs = entry.transaction_output_count;
    coins_stats.nBogoSize = entry.bogo_size;
    coins_stats.nTotalAmount = entry.total_amount;
    coins_stats.nDiskSize = entry.disk_size;

    return true;
}

uint256 CoinStatsIndex::currentHashInternal()
{
    unsigned char out[384];
    m_muhash.Finalize(out);
    return (TruncatedSHA512Writer(SER_DISK, 0) << out).GetHash();
}

// Reverse Block in case of reorg
bool CoinStatsIndex::ReverseBlock(const CBlock& block, const CBlockIndex* pindex)
{
    CBlockUndo block_undo;
    std::pair<uint256, DBVal> read_out;

    if (pindex->nHeight > 0) {
        if (!UndoReadFromDisk(block_undo, pindex)) {
            return false;
        }

        if (!m_db->Read(DBHeightKey(pindex->nHeight - 1), read_out)) {
            return false;
        }

        uint256 expected_block_hash = pindex->pprev->GetBlockHash();
        if (read_out.first != expected_block_hash) {
            return error("%s: previous block header belongs to unexpected block %s; expected %s",
                         __func__, read_out.first.ToString(), expected_block_hash.ToString());
        }
    }

    MuHash3072 do_muhash;

    // Remove the new utxos that were created from the block
    for (size_t i = 0; i < block.vtx.size(); ++i) {
        const auto& tx = block.vtx.at(i);

        for (size_t j = 0; j < tx->vout.size(); ++j) {
            const CTxOut& out = tx->vout[j];
            COutPoint outpoint = COutPoint(tx->GetHash(), j);
            Coin coin = Coin(out, pindex->nHeight, tx->IsCoinBase());

            // Skip unspendable coins
            if (coin.out.scriptPubKey.IsUnspendable()) continue;

            do_muhash *= MuHash3072(GetTruncatedSHA512Hash(outpoint, coin).begin());
        }

        // The coinbase tx has no undo data since no former output is spent
        if (i > 0) {
            const auto& tx_undo = block_undo.vtxundo.at(i-1);

            for (size_t j = 0; j < tx_undo.vprevout.size(); ++j) {
                Coin coin = tx_undo.vprevout[j];
                COutPoint outpoint = COutPoint(tx->vin[j].prevout.hash, tx->vin[j].prevout.n);

                m_muhash *= MuHash3072(GetTruncatedSHA512Hash(outpoint, coin).begin());
            }
        }
    }

    m_muhash /= do_muhash;

    m_transaction_output_count = read_out.second.transaction_output_count;
    m_total_amount = read_out.second.total_amount;
    m_bogo_size = read_out.second.bogo_size;
    m_disk_size = read_out.second.disk_size;

    if (!m_db->Write(DB_MUHASH, m_muhash)) {
        return false;
    }

    return true;
}
