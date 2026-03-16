// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txospenderindex.h>

#include <common/args.h>
#include <crypto/siphash.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <index/base.h>
#include <index/disktxpos.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/fs.h>
#include <validation.h>

#include <cstdio>
#include <exception>
#include <ios>
#include <span>
#include <string>
#include <utility>
#include <vector>

/* The database is used to find the spending transaction of a given utxo.
 * For every input of every transaction it stores a key that is a pair(siphash(input outpoint), transaction location on disk) and an empty value.
 * To find the spending transaction of an outpoint, we perform a range query on siphash(outpoint), and for each returned key load the transaction
 * and return it if it does spend the provided outpoint.
 */

// LevelDB key prefix. We only have one key for now but it will make it easier to add others if needed.
constexpr uint8_t DB_TXOSPENDERINDEX{'s'};

std::unique_ptr<TxoSpenderIndex> g_txospenderindex;

struct DBKey {
    uint64_t hash;
    CDiskTxPos pos;

    explicit DBKey(const uint64_t& hash_in, const CDiskTxPos& pos_in) : hash(hash_in), pos(pos_in) {}

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
    : BaseIndex(std::move(chain), "txospenderindex"), m_db{std::make_unique<DB>(gArgs.GetDataDirNet() / "indexes" / "txospenderindex" / "db", n_cache_size, f_memory, f_wipe)}
{
    if (!m_db->Read("siphash_key", m_siphash_key)) {
        FastRandomContext rng(false);
        m_siphash_key = {rng.rand64(), rng.rand64()};
        m_db->Write("siphash_key", m_siphash_key, /*fSync=*/ true);
    }
}

interfaces::Chain::NotifyOptions TxoSpenderIndex::CustomOptions()
{
    interfaces::Chain::NotifyOptions options;
    options.disconnect_data = true;
    return options;
}

static uint64_t CreateKeyPrefix(std::pair<uint64_t, uint64_t> siphash_key, const COutPoint& vout)
{
    return PresaltedSipHasher(siphash_key.first, siphash_key.second)(vout.hash.ToUint256(), vout.n);
}

static DBKey CreateKey(std::pair<uint64_t, uint64_t> siphash_key, const COutPoint& vout, const CDiskTxPos& pos)
{
    return DBKey(CreateKeyPrefix(siphash_key, vout), pos);
}

void TxoSpenderIndex::WriteSpenderInfos(const std::vector<std::pair<COutPoint, CDiskTxPos>>& items)
{
    CDBBatch batch(*m_db);
    for (const auto& [outpoint, pos] : items) {
        DBKey key(CreateKey(m_siphash_key, outpoint, pos));
        // key is hash(spent outpoint) | disk pos, value is empty
        batch.Write(key, "");
    }
    m_db->WriteBatch(batch);
}


void TxoSpenderIndex::EraseSpenderInfos(const std::vector<std::pair<COutPoint, CDiskTxPos>>& items)
{
    CDBBatch batch(*m_db);
    for (const auto& [outpoint, pos] : items) {
        batch.Erase(CreateKey(m_siphash_key, outpoint, pos));
    }
    m_db->WriteBatch(batch);
}

static std::vector<std::pair<COutPoint, CDiskTxPos>> BuildSpenderPositions(const interfaces::BlockInfo& block)
{
    std::vector<std::pair<COutPoint, CDiskTxPos>> items;
    items.reserve(block.data->vtx.size());

    CDiskTxPos pos({block.file_number, block.data_pos}, GetSizeOfCompactSize(block.data->vtx.size()));
    for (const auto& tx : block.data->vtx) {
        if (!tx->IsCoinBase()) {
            for (const auto& input : tx->vin) {
                items.emplace_back(input.prevout, pos);
            }
        }
        pos.nTxOffset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
    }

    return items;
}


bool TxoSpenderIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    WriteSpenderInfos(BuildSpenderPositions(block));
    return true;
}

bool TxoSpenderIndex::CustomRemove(const interfaces::BlockInfo& block)
{
    EraseSpenderInfos(BuildSpenderPositions(block));
    return true;
}

util::Expected<TxoSpender, std::string> TxoSpenderIndex::ReadTransaction(const CDiskTxPos& tx_pos) const
{
    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, /*fReadOnly=*/true)};
    if (file.IsNull()) {
        return util::Unexpected("cannot open block");
    }
    CBlockHeader header;
    TxoSpender spender;
    try {
        file >> header;
        file.seek(tx_pos.nTxOffset, SEEK_CUR);
        file >> TX_WITH_WITNESS(spender.tx);
        spender.block_hash = header.GetHash();
        return spender;
    } catch (const std::exception& e) {
        return util::Unexpected(e.what());
    }
}

util::Expected<std::optional<TxoSpender>, std::string> TxoSpenderIndex::FindSpender(const COutPoint& txo) const
{
    const uint64_t prefix{CreateKeyPrefix(m_siphash_key, txo)};
    std::unique_ptr<CDBIterator> it(m_db->NewIterator());
    DBKey key(prefix, CDiskTxPos());

    // find all keys that start with the outpoint hash, load the transaction at the location specified in the key
    // and return it if it does spend the provided outpoint
    for (it->Seek(std::pair{DB_TXOSPENDERINDEX, prefix}); it->Valid() && it->GetKey(key) && key.hash == prefix; it->Next()) {
        if (const auto spender{ReadTransaction(key.pos)}) {
            for (const auto& input : spender->tx->vin) {
                if (input.prevout == txo) {
                    return std::optional{*spender};
                }
            }
        } else {
            LogError("Deserialize or I/O error - %s", spender.error());
            return util::Unexpected{strprintf("IO error finding spending tx for outpoint %s:%d.", txo.hash.GetHex(), txo.n)};
        }
    }
    return util::Expected<std::optional<TxoSpender>, std::string>(std::nullopt);
}

BaseIndex::DB& TxoSpenderIndex::GetDB() const { return *m_db; }
