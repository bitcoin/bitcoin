// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <hash.h>
#include <index/addrindex.h>
#include <index/disktxpos.h>
#include <shutdown.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/standard.h>
#include <txdb.h>
#include <validation.h>
#include <vector>
#include <uint256.h>

std::unique_ptr<AddrIndex> g_addr_index;

/*
 * The address index stores two main types of objects that allow for
 * script-based/address-based look-ups of all created outputs and all spent
 * outputs in the Bitcoin blockchain. These are differentiated by their key_type
 * as either DBKeyType::SPENT or DBKekyType::CREATED. The address index also
 * stores one unique global value under the DBKeyType::SEED key that seeds the
 * MurmurHash3 hasher used to create AddrIds.
 *
 * The DB keys are structured as follows: <addr_id, key_type, outpoint>
 *
 * addr_id is the hash of the script_pub_key computed using MurmurHash3, a short
 * non-cryptographic hash function. It also functions as the search key, since
 * LevelDB keys are iterated through in lexicograpical order. Collisions are
 * resolved by storing the full script_pub_key in the DB value. This can be
 * checked against the script used to make a look-up in the index.
 *
 * key_type is SPENT when the outpoint stored in the key is spent, i.e. it is
 * used as a prevout in a transaction input.  It is CREATED when the outpoint is
 * created as a new  COutpoint in that transaction.
 *
 * outpoints are stored in the key as opposed to in the DB value to preserve
 * uniqueness and to support multiple values for a single addr_id and key_type
 * combination. LevelDB only allows one value stored for each key.
 *
 *
 * The DB values are simply: <CDiskTxPos, CScript>
 *
 * The tx_pos (CDiskTxPos) value is the transaction in which:
 *  - the outpoint in the key was spent (for SPENT keys)
 *  - OR where the outpoint was created (for CREATED keys)
 *
 * The CScript (the script_pub_key) is used for collision resolution.
 */
using AddrId = unsigned int;
// DBKeyType is used by the address index to distinguish between the
// different kinds of values stored.
enum class DBKeyType : uint8_t {
    SEED,    // Seed used for MurmurHash3 inside GetAddrId
    SPENT,   // Used for values in the index indicating a spend
    CREATED, // Used for values in the index indicating the creation of an input
};


namespace {

struct DBKey {
  AddrId m_addr_id;
  DBKeyType m_key_type;
  COutPoint m_outpoint;

  DBKey() {}
  explicit DBKey(DBKeyType key_type, AddrId addr_id, COutPoint outpoint) : m_addr_id(addr_id), m_key_type(key_type), m_outpoint(outpoint) {}

  SERIALIZE_METHODS(DBKey, obj) {
    uint8_t key_type;
    SER_WRITE(obj, key_type = static_cast<uint8_t>(obj.m_key_type));

    READWRITE(obj.m_addr_id, key_type, obj.m_outpoint);

    SER_READ(obj, obj.m_key_type  = static_cast<DBKeyType>(key_type));
    // Check if the key type is a valid key. SEED keys are excluded because they
    // are never created with this type.
    if ((obj.m_key_type != DBKeyType::SPENT) && (obj.m_key_type != DBKeyType::CREATED)) {
      throw std::ios_base::failure("Invalid key type for address index DB key");
    }
  }
};

}; // namespace

// The address index stores information needed to get relevant transactions,
// and a copy of the CScript to double check against in case of hash collisions.
using DBValue = std::pair<CDiskTxPos, CScript>;

/** Access to the addr_index database (indexes/addr_index/)*/
class AddrIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /** ReadAddrIndex returns the set of entries stored in the index for this addr_id. */
    std::vector<std::pair<DBKey, DBValue>> ReadAddrIndex(const int max_count, const int skip, const unsigned int addr_id, const CScript& script);

    /** WriteToIndex writes the input vector of database entries into the index.  */
    bool WriteToIndex(const std::vector<std::pair<DBKey, DBValue>> &entries);

    /** SetupHashSeed is used to create/backup/restore the seed used by the index for hashing. */
    unsigned int SetupHashSeed();
};

AddrIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(GetDataDir() / "indexes" / "addr_index", n_cache_size, f_memory, f_wipe)
{}

BaseIndex::DB& AddrIndex::GetDB() const { return *m_db; }

std::vector<std::pair<DBKey, DBValue>> AddrIndex::DB::ReadAddrIndex(const int max_count, const int skip, const unsigned int addr_id, const CScript& script)
{
    std::vector<std::pair<DBKey, DBValue>> result;
    AddrId search_key = addr_id;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    iter->Seek(search_key);
    int i = 0;
    while (iter->Valid() && i < max_count) {
        DBKey key;
        DBValue value;
        if (!iter->GetKey(key) || key.m_addr_id != addr_id) break;
        if (!iter->GetValue(value)) {
            LogPrintf("%s: Failed to read value stored under key with addr_id:  %d\n", __func__, addr_id);
            break;
        }

        // Check that the stored script matches the one we're searching for, in case of hash collisions.
        if (value.second == script && i >= skip) {
            result.emplace_back(std::make_pair(key, value));
        }
        iter->Next();
        i++;
    }

    return result;
}

bool AddrIndex::Init() {
        m_hash_seed = m_db->SetupHashSeed();
        return BaseIndex::Init();
}

AddrIndex::AddrIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : m_db(MakeUnique<AddrIndex::DB>(n_cache_size, f_memory, f_wipe)) {}

unsigned int AddrIndex::DB::SetupHashSeed() {
    static const uint8_t seed_key =  static_cast<uint8_t>(DBKeyType::SEED);
    unsigned int seed;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    uint8_t key;

    // If key is in the index already, read it and return.
    iter->Seek(seed_key);
    if (iter->Valid() && iter->GetKey(key) && key == seed_key) {
        if (!iter->GetValue(seed)) {
            return error("%s: Cannot read current %s seed key; index may be corrupted", __func__);
        }
        return seed;
    }

    // Generate a random key and write it to the index.
    seed = GetRandInt(std::numeric_limits<int>::max());
    Write(seed_key, seed);
    return seed;
}

AddrIndex::~AddrIndex() {}

unsigned int AddrIndex::GetAddrId(const CScript& script) {
    std::vector<unsigned char> script_data;
    for (auto it = script.begin(); it != script.end(); ++it) {
        script_data.push_back(*it);
    }
    return MurmurHash3(m_hash_seed, script_data);
}

bool AddrIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    CBlockUndo block_undo;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<DBKey, DBValue>> entries;

    const bool genesis_block = (pindex->nHeight == 0);
    if (!genesis_block && !UndoReadFromDisk(block_undo, pindex)) {
      return false;
    }

    for (size_t i = 0; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *(block.vtx[i]);
        const uint256 tx_hash = tx.GetHash();
        for (size_t j = 0; j < tx.vout.size(); ++j) {
            CScript script_pub_key = tx.vout[j].scriptPubKey;
            DBKey key(DBKeyType::CREATED, GetAddrId(script_pub_key), COutPoint(tx_hash, j));
            entries.emplace_back(key, std::make_pair(pos, script_pub_key));
        }

        // Skip coinbase inputs.
        if (!genesis_block && i > 0) {
            const CTxUndo& tx_undo = block_undo.vtxundo[i-1];
            for (size_t k = 0; k < tx.vin.size(); ++k) {
                CScript spent_outputs_scriptpubkey = tx_undo.vprevout[k].out.scriptPubKey;
                DBKey key(DBKeyType::SPENT, GetAddrId(spent_outputs_scriptpubkey), tx.vin[k].prevout);
                entries.emplace_back(key, std::make_pair(pos, spent_outputs_scriptpubkey));
            }
        }
        pos.nTxOffset += ::GetSerializeSize(tx, CLIENT_VERSION);
    }

    return m_db->WriteToIndex(entries);
}

bool AddrIndex::DB::WriteToIndex(const std::vector<std::pair<DBKey, DBValue>> &entries)
{
    CDBBatch batch(*this);
    for (const auto& entry : entries) {
        batch.Write(entry.first, entry.second);
    }
    return WriteBatch(batch);
}

// FindTxsByScript fills the spends_result vector with outpoints corresponding
// to the output spent with the given script, and the transaction it was spent
// in. creations_result is filled with outpoints for outputs created with this
// script as their script pubkey, and the transactions they were created in.
// max_count determines the maximum number of results returned. The skip
// parameter sets the number of initial values skipped.
bool AddrIndex::FindTxsByScript(const int max_count,
                                const int skip,
                                const CScript& script,
                                std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> &spends_result,
                                std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> &creations_result)
{
    auto db_entries = m_db->ReadAddrIndex(max_count, skip, GetAddrId(script), script);
    if (db_entries.size() == 0) return false;

    for (const auto& entry : db_entries) {
        DBKey key = entry.first;
        CDiskTxPos pos = entry.second.first;

        CAutoFile file(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
        if (file.IsNull()) {
            return error("%s: OpenBlockFile failed", __func__);
        }
        CBlockHeader header;
        CTransactionRef tx;
        try {
            file >> header;
            if (fseek(file.Get(), pos.nTxOffset, SEEK_CUR)) {
                return error("%s: fseek(...) failed", __func__);
            }
            file >> tx;
        } catch (const std::exception& e) {
            return error("%s: Deserialize or I/O error - %s", __func__, e.what());
        }
        std::pair<CTransactionRef, uint256> result =  std::make_pair(tx, header.GetHash());

        // Place entry into correct vector depending on its type.
        switch (key.m_key_type) {
            case DBKeyType::SPENT:
                spends_result.emplace_back(std::make_pair(key.m_outpoint, result));
                break;
            case DBKeyType::CREATED:
                creations_result.emplace_back(std::make_pair(key.m_outpoint, result));
                break;
            default:
                LogPrintf("AddrIndex::DB returned value with unexpected key type.\n");
                return false;
        }
    }

    return true;
}
