// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>
#include <cmath>

#include <dbwrapper.h>
#include <hash.h>
#include <index/walletfilterindex.h>
#include <node/blockstorage.h>
#include <util/system.h>
#include <validation.h>

std::unique_ptr<WalletFilterIndex> g_wallet_filter_index;

using node::UndoReadFromDisk;

constexpr uint8_t DB_BLOCK_HASH{'s'};
constexpr uint8_t DB_FILTER_POS{'P'};
constexpr uint8_t DB_SIPHASH_K0{'k'};
constexpr uint8_t DB_SIPHASH_K1{'K'};

constexpr unsigned int MAX_FLTR_FILE_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for fltr?????.dat files */
constexpr unsigned int FLTR_FILE_CHUNK_SIZE = 0x100000; // 1 MiB

// value for M explained https://github.com/sipa/writeups/tree/main/minimizing-golomb-filters
constexpr uint8_t WALLET_FILTER_P = 19;
constexpr uint32_t WALLET_FILTER_M = 784931; // 1.497137 * pow(2,WALLET_FILTER_P);

namespace {

struct DBVal {
    FlatFilePos pos;
    uint64_t filter_sip_hash;

    SERIALIZE_METHODS(DBVal, obj) { READWRITE(obj.pos, obj.filter_sip_hash); }
};

struct DBHashKey {
    uint256 hash;

    explicit DBHashKey(const uint256& hash_in) : hash(hash_in) {}

    SERIALIZE_METHODS(DBHashKey, obj) {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for wallet filter index DB hash key");
        }

        READWRITE(obj.hash);
    }
};

};

WalletFilterIndex::WalletFilterIndex(std::unique_ptr<interfaces::Chain> chain,
                                     size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "wallet filter index")
{
    fs::path path = gArgs.GetDataDirNet() / "indexes" / "walletfilter";
    fs::create_directories(path);

    m_db = std::make_unique<BaseIndex::DB>(path / "db", n_cache_size, f_memory, f_wipe);
    m_filter_fileseq = std::make_unique<FlatFileSeq>(std::move(path), "fltr", FLTR_FILE_CHUNK_SIZE);
}

bool WalletFilterIndex::CustomInit(const std::optional<interfaces::BlockKey>& block)
{
    m_params.m_P = WALLET_FILTER_P;
    m_params.m_M = WALLET_FILTER_M;

    if (m_db->Exists(DB_FILTER_POS) || m_db->Exists(DB_SIPHASH_K0) || m_db->Exists(DB_SIPHASH_K1)) {
        if (!m_db->Read(DB_FILTER_POS, m_next_filter_pos))
            return error("%s: Cannot read current %s state; index may be corrupted",
                         __func__, GetName());
        if (!m_db->Read(DB_SIPHASH_K0, m_params.m_siphash_k0))
            return error("%s: Cannot read current %s state; index may be corrupted",
                         __func__, GetName());
        if (!m_db->Read(DB_SIPHASH_K1, m_params.m_siphash_k1))
            return error("%s: Cannot read current %s state; index may be corrupted",
                         __func__, GetName());
    } else {
        m_next_filter_pos.nFile = 0;
        m_next_filter_pos.nPos = 0;

        m_params.m_siphash_k0 = GetRand(std::numeric_limits<uint64_t>::max());
        m_params.m_siphash_k1 = GetRand(std::numeric_limits<uint64_t>::max());

        CDBBatch batch(*m_db);
        batch.Write(DB_SIPHASH_K0, m_params.m_siphash_k0);
        batch.Write(DB_SIPHASH_K1, m_params.m_siphash_k1);
        batch.Write(DB_FILTER_POS, m_next_filter_pos);
        if (!m_db->WriteBatch(batch)) return false;
    }

    return true;
}

bool WalletFilterIndex::CustomCommit(CDBBatch& batch)
{
    const FlatFilePos& pos = m_next_filter_pos;

    // Flush current filter file to disk.
    AutoFile file{m_filter_fileseq->Open(pos)};
    if (file.IsNull()) {
        return error("%s: Failed to open filter file %d", __func__, pos.nFile);
    }
    if (!FileCommit(file.Get())) {
        return error("%s: Failed to commit filter file %d", __func__, pos.nFile);
    }

    batch.Write(DB_FILTER_POS, pos);
    return true;
}


size_t WalletFilterIndex::WriteFilterToDisk(FlatFilePos& pos, const GCSFilter& filter)
{
    size_t data_size = GetSerializeSize(filter.GetEncoded(), CLIENT_VERSION);

    // If writing the filter would overflow the file, flush and move to the next one.
    if (pos.nPos + data_size > MAX_FLTR_FILE_SIZE) {
        AutoFile last_file{m_filter_fileseq->Open(pos)};
        if (last_file.IsNull()) {
            LogPrintf("%s: Failed to open filter file %d\n", __func__, pos.nFile);
            return 0;
        }
        if (!TruncateFile(last_file.Get(), pos.nPos)) {
            LogPrintf("%s: Failed to truncate filter file %d\n", __func__, pos.nFile);
            return 0;
        }
        if (!FileCommit(last_file.Get())) {
            LogPrintf("%s: Failed to commit filter file %d\n", __func__, pos.nFile);
            return 0;
        }

        pos.nFile++;
        pos.nPos = 0;
    }

    // Pre-allocate sufficient space for filter data.
    bool out_of_space;
    m_filter_fileseq->Allocate(pos, data_size, out_of_space);
    if (out_of_space) {
        LogPrintf("%s: out of disk space\n", __func__);
        return 0;
    }

    AutoFile fileout{m_filter_fileseq->Open(pos)};
    if (fileout.IsNull()) {
        LogPrintf("%s: Failed to open filter file %d\n", __func__, pos.nFile);
        return 0;
    }

    fileout << filter.GetEncoded();
    return data_size;
}

bool WalletFilterIndex::ReadFilterFromDisk(const FlatFilePos& pos, GCSFilter& filter) const
{
    AutoFile filein{m_filter_fileseq->Open(pos, true)};
    if (filein.IsNull()) {
        return false;
    }

    // Check that the hash of the encoded_filter matches the one stored in the db.
    std::vector<uint8_t> encoded_filter;
    try {
        filein >> encoded_filter;
        filter = GCSFilter(m_params, std::move(encoded_filter), /*skip_decode_check=*/true);
    }
    catch (const std::exception& e) {
        return error("%s: Failed to deserialize block filter from disk: %s", __func__, e.what());
    }

    return true;
}

static GCSFilter::ElementSet WalletFilterElements(const CBlock& block,
                                                 const CBlockUndo& block_undo)
{
    GCSFilter::ElementSet elements;

    for (const CTransactionRef& tx : block.vtx) {
        for (const CTxOut& txout : tx->vout) {
            const CScript& script = txout.scriptPubKey;
            if (script.empty() || script[0] == OP_RETURN) continue;
            elements.emplace(script.begin(), script.end());
        }
    }

    for (const CTxUndo& tx_undo : block_undo.vtxundo) {
        for (const Coin& prevout : tx_undo.vprevout) {
            const CScript& script = prevout.out.scriptPubKey;
            if (script.empty()) continue;
            elements.emplace(script.begin(), script.end());
        }
    }

    return elements;
}

bool WalletFilterIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    CBlockUndo block_undo;

    if (block.height > 0) {
        // pindex variable gives indexing code access to node internals. It
        // will be removed in upcoming commit
        const CBlockIndex* pindex = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(block.hash));
        if (!UndoReadFromDisk(block_undo, pindex)) {
            return false;
        }
    }

    GCSFilter::ElementSet elements = WalletFilterElements(*Assert(block.data), block_undo);
    GCSFilter filter(m_params, elements);

    size_t bytes_written = WriteFilterToDisk(m_next_filter_pos, filter);
    if (bytes_written == 0) return false;

    // filter hash uses the siphash parameters and includes the block hash
    // this implicitly prevents mixups between indexes and blocks
    uint64_t filter_sip_hash = CSipHasher(m_params.m_siphash_k0, m_params.m_siphash_k1)
        .Write(block.hash.GetUint64(0))
        .Write(block.hash.GetUint64(1))
        .Write(block.hash.GetUint64(2))
        .Write(block.hash.GetUint64(3))
        .Write(filter.GetEncoded().data(), filter.GetEncoded().size())
        .Finalize();

    DBVal entry;
    entry.pos = m_next_filter_pos;
    entry.filter_sip_hash = filter_sip_hash;

    if (!m_db->Write(DBHashKey(block.hash), entry)) {
        return false;
    }

    m_next_filter_pos.nPos += bytes_written;
    return true;
}

bool WalletFilterIndex::LookupFilter(const CBlockIndex* block_index, GCSFilter& filter_out) const
{
    DBVal entry;
    if (!m_db->Read(DBHashKey(block_index->GetBlockHash()), entry))
        return false;

    if (!ReadFilterFromDisk(entry.pos, filter_out))
        return false;

    uint64_t filter_sip_hash = CSipHasher(m_params.m_siphash_k0, m_params.m_siphash_k1)
        .Write(block_index->GetBlockHash().GetUint64(0))
        .Write(block_index->GetBlockHash().GetUint64(1))
        .Write(block_index->GetBlockHash().GetUint64(2))
        .Write(block_index->GetBlockHash().GetUint64(3))
        .Write(filter_out.GetEncoded().data(), filter_out.GetEncoded().size())
        .Finalize();

    return filter_sip_hash == entry.filter_sip_hash;
}
