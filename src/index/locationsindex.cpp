// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
    uint32_t row; // allow splitting one block's transactions into multiple DB rows

    explicit DBKey(const uint256& hash_in, uint32_t row_in) : hash(hash_in), row(row_in) {}

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for location index DB hash key");
        }

        READWRITE(obj.hash);
        READWRITE(COMPACTSIZE(obj.row));
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

bool LocationsIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    static const uint32_t HEADER_SIZE = ::GetSerializeSize(CBlockHeader{});

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
    for (uint32_t row = 0; copied < nTx; ++row) {
        size_t row_size = std::min(nTx - copied, TXS_PER_ROW);
        std::span<uint32_t> row_offsets{&block_offsets[copied], row_size + 1};

        DBKey key{block.hash, row};
        DBValue value{{block.file_number, block.data_pos}, {row_offsets.begin(), row_offsets.end()}};

        copied += row_size;
        batch.Write(key, value);
    }
    return GetDB().WriteBatch(batch);
}

bool LocationsIndex::ReadRawTransaction(const uint256& block_hash, size_t i, std::vector<std::byte>& out) const
{
    uint32_t row = i / TXS_PER_ROW;      // used to find the correct DB row
    const auto column = i % TXS_PER_ROW; // index within a single DB row

    DBValue value{};
    if (!m_db->Read(DBKey{block_hash, row}, value)) {
        return false;
    }

    if (value.offsets.size() <= 1) {
        LogError("%s: LocationsIndex entry for %s:%u must have >1 offsets\n", __func__, block_hash.ToString(), row);
        return false;
    }
    size_t tx_count = value.offsets.size() - 1;
    if (column >= tx_count) {
        LogError("%s: LocationsIndex entry for %s:%u has %d transactions\n", __func__, block_hash.ToString(), row, tx_count);
        return false;
    }

    FlatFilePos tx_pos{value.block_pos};
    tx_pos.nPos += value.offsets[column];
    size_t tx_size = value.offsets[column + 1] - value.offsets[column];

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
