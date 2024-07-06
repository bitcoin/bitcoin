// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include <blockencodings.h>
#include <clientversion.h>
#include <common/args.h>
#include <dbwrapper.h>
#include <hash.h>
#include <index/locationsindex.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <serialize.h>
#include <util/fs_helpers.h>
#include <validation.h>

constexpr uint8_t DB_BLOCK_HASH{'s'};

constexpr uint32_t TXS_PER_ROW{128};

namespace {

struct DBKey {
    uint256 hash;
    uint32_t part; // allow splitting one block's transactions into multiple DB rows

    explicit DBKey(const uint256& hash_in, uint32_t part_in) : hash(hash_in), part(part_in) {}

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for location index DB hash key");
        }

        READWRITE(obj.hash);
        READWRITE(COMPACTSIZE(obj.part));
    }
};

struct DBValue {
    FlatFilePos block_pos;
    std::vector<uint32_t> offsets{};

    SERIALIZE_METHODS(DBValue, obj)
    {
        READWRITE(obj.block_pos);
        READWRITE(Using<VectorFormatter<DifferenceFormatter>>(obj.offsets));
    }
};

}; // namespace

std::unique_ptr<LocationsIndex> g_locations_index;

LocationsIndex::LocationsIndex(std::unique_ptr<interfaces::Chain> chain,
                               size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "locationsindex")
{
    fs::path path = gArgs.GetDataDirNet() / "indexes" / "locations";
    fs::create_directories(path);

    m_db = std::make_unique<BaseIndex::DB>(path / "db", n_cache_size, f_memory, f_wipe);
}

static const uint32_t HEADER_SIZE = ::GetSerializeSize(CBlockHeader{});

bool LocationsIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    assert(block.data);
    assert(block.file_number >= 0);

    const uint32_t nTx = block.data->vtx.size();
    uint32_t nTxOffset = HEADER_SIZE + GetSizeOfCompactSize(nTx);

    std::vector<uint32_t> block_offsets{};
    block_offsets.reserve(nTx + 1);
    block_offsets.push_back(nTxOffset); // first transaction offset within the block

    for (const auto& tx : block.data->vtx) {
        nTxOffset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
        block_offsets.push_back(nTxOffset);
    }

    CDBBatch batch{GetDB()};
    uint32_t copied = 0;
    for (uint32_t part = 0; copied < nTx; ++part) {
        size_t part_size = std::min(nTx - copied, TXS_PER_ROW);
        std::span<uint32_t> part_offsets{&block_offsets[copied], part_size + 1};

        DBKey key{block.hash, part};
        DBValue value{{block.file_number, block.data_pos}, {part_offsets.begin(), part_offsets.end()}};

        copied += part_size;
        batch.Write(key, value);
    }
    return GetDB().WriteBatch(batch);
}

bool LocationsIndex::ReadRawTransaction(const uint256& block_hash, size_t i, std::vector<uint8_t>& out) const
{
    uint32_t part = i / TXS_PER_ROW; // used to find the correct DB row
    i = i % TXS_PER_ROW;             // index within a single DB row

    DBValue value{};
    if (!m_db->Read(DBKey{block_hash, part}, value)) {
        return false;
    }

    if (value.offsets.empty()) {
        LogError("%s: LocationsIndex entry for %s:%u must have >1 offsets\n", __func__, block_hash.ToString(), part);
        return false;
    }
    size_t nTx = value.offsets.size() - 1;
    if (i >= nTx) {
        LogError("%s: LocationsIndex entry for %s:%u has %d transactions\n", __func__, block_hash.ToString(), part, nTx);
        return false;
    }

    FlatFilePos tx_pos{value.block_pos};
    tx_pos.nPos += value.offsets[i];
    size_t tx_size = value.offsets[i + 1] - value.offsets[i];

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, true)};
    if (file.IsNull()) {
        LogError("%s: OpenBlockFile failed\n", __func__);
        return false;
    }

    out.resize(tx_size); // Zeroing of memory is intentional here
    try {
        file.read(MakeWritableByteSpan(out));
    } catch (const std::exception& e) {
        LogError("%s: Read %d bytes from %s failed: %s\n", __func__, tx_size, tx_pos.ToString(), e.what());
        return false;
    }

    return true;
}
