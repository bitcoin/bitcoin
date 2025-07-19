// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <crypto/siphash.h>
#include <index/disktxpos.h>
#include <index/txospenderindex.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <random.h>
#include <uint256.h>
#include <validation.h>

// LevelDB key prefix. We only have one key for now but it will make it easier to add others if needed.
constexpr uint8_t DB_TXOSPENDERINDEX{'s'};

std::unique_ptr<TxoSpenderIndex> g_txospenderindex;

struct DBKey {
    uint64_t hash;
    FlatFilePos pos;

    explicit DBKey(const uint64_t& hash_in, const FlatFilePos& pos_in) : hash(hash_in), pos(pos_in) {}

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_TXOSPENDERINDEX};
        READWRITE(prefix);
        if (prefix != DB_TXOSPENDERINDEX) {
            throw std::ios_base::failure("Invalid format for spender index DB key");
        }
        READWRITE(obj.hash);
        READWRITE(obj.pos);
    }
};

TxoSpenderIndex::TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "txospenderindex")
{
    fs::path path{gArgs.GetDataDirNet() / "indexes" / "txospenderindex"};
    fs::create_directories(path);

    m_db = std::make_unique<TxoSpenderIndex::DB>(path / "db", n_cache_size, f_memory, f_wipe);
    if (!m_db->Read("siphash_key", m_siphash_key)) {
        FastRandomContext rng(false);
        m_siphash_key = {rng.rand64(), rng.rand64()};
        assert(m_db->Write("siphash_key", m_siphash_key));
    }
}

TxoSpenderIndex::~TxoSpenderIndex() = default;

interfaces::Chain::NotifyOptions TxoSpenderIndex::CustomOptions()
{
    interfaces::Chain::NotifyOptions options;
    options.disconnect_data = true;
    return options;
}

uint64_t CreateKeyPrefix(std::pair<uint64_t, uint64_t> siphash_key, const COutPoint& vout)
{
    return SipHashUint256Extra(siphash_key.first, siphash_key.second, vout.hash.ToUint256(), vout.n);
}

DBKey CreateKey(std::pair<uint64_t, uint64_t> siphash_key, const COutPoint& vout, const FlatFilePos& pos)
{
    return DBKey(CreateKeyPrefix(siphash_key, vout), pos);
}

bool TxoSpenderIndex::WriteSpenderInfos(const std::vector<std::pair<COutPoint, FlatFilePos>>& items)
{
    CDBBatch batch(*m_db);
    for (const auto& [outpoint, pos] : items) {
        DBKey key(CreateKey(m_siphash_key, outpoint, pos));
        // key is hash(spent outpoint) | disk pos, value is empty
        batch.Write(key, "");
    }
    return m_db->WriteBatch(batch);
}


bool TxoSpenderIndex::EraseSpenderInfos(const std::vector<std::pair<COutPoint, FlatFilePos>>& items)
{
    CDBBatch batch(*m_db);
    for (const auto& [outpoint, pos] : items) {
        batch.Erase(CreateKey(m_siphash_key, outpoint, pos));
    }
    return m_db->WriteBatch(batch);
}

std::vector<std::pair<COutPoint, FlatFilePos>> BuildSpenderPositions(const interfaces::BlockInfo& block)
{
    std::vector<std::pair<COutPoint, FlatFilePos>> items;
    items.reserve(block.data->vtx.size());

    FlatFilePos pos{block.file_number, block.data_pos};
    pos.nPos += 80; // skip block header
    pos.nPos += GetSizeOfCompactSize(block.data->vtx.size());
    for (const auto& tx : block.data->vtx) {
        if (!tx->IsCoinBase()) {
            for (const auto& input : tx->vin) {
                items.emplace_back(input.prevout, pos);
            }
        }
        pos.nPos += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
    }

    return items;
}


bool TxoSpenderIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    return WriteSpenderInfos(BuildSpenderPositions(block));
}

bool TxoSpenderIndex::CustomRemove(const interfaces::BlockInfo& block)
{
    return EraseSpenderInfos(BuildSpenderPositions(block));
}

bool TxoSpenderIndex::ReadTransaction(const FlatFilePos& tx_pos, CTransactionRef& tx) const
{
    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, true)};
    if (file.IsNull()) {
        return false;
    }
    try {
        file >> TX_WITH_WITNESS(tx);
        return true;
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s\n", e.what());
        return false;
    }
}

CTransactionRef TxoSpenderIndex::FindSpender(const COutPoint& txo) const
{
    uint64_t prefix = CreateKeyPrefix(m_siphash_key, txo);
    std::unique_ptr<CDBIterator> it(m_db->NewIterator());
    DBKey key(prefix, FlatFilePos());
    CTransactionRef tx;

    // find all keys that start with the outpoint hash, load the transaction at the location specified in the key
    // and return it if it does spend the provided outpoint
    for (it->Seek(std::pair{DB_TXOSPENDERINDEX, prefix}); it->Valid() && it->GetKey(key) && key.hash == prefix; it->Next()) {
        if (ReadTransaction(key.pos, tx)) {
            for (const auto& input : tx->vin) {
                if (input.prevout == txo) {
                    return tx;
                }
            }
        }
    }
    return nullptr;
}

BaseIndex::DB& TxoSpenderIndex::GetDB() const { return *m_db; }
