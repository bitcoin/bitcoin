// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
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
                               size_t cache_size, bool memory_only, bool wipe)
    : BaseIndex{std::move(chain), "locationsindex"}, m_db{std::make_unique<BaseIndex::DB>(gArgs.GetDataDirNet() / "indexes" / "locations" / "db", cache_size, memory_only, wipe)}
{
}

bool LocationsIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    static const uint32_t HEADER_SIZE = ::GetSerializeSize(CBlockHeader{});

    assert(block.data);
    assert(block.file_number >= 0);

    Assume(block.data->vtx.size() <= std::numeric_limits<uint32_t>::max());
    const uint32_t tx_count{static_cast<uint32_t>(block.data->vtx.size())};
    uint32_t tx_offset{HEADER_SIZE + GetSizeOfCompactSize(tx_count)};

    std::vector<uint32_t> block_offsets{};
    block_offsets.reserve(tx_count + 1);
    block_offsets.push_back(tx_offset); // first transaction offset within the block

    for (const auto& tx : block.data->vtx) {
        tx_offset += ::GetSerializeSize(TX_WITH_WITNESS(*tx));
        block_offsets.push_back(tx_offset);
    }

    CDBBatch batch{GetDB()};
    uint32_t copied = 0;
    for (uint32_t row = 0; copied < tx_count; ++row) {
        size_t row_size = std::min(tx_count - copied, TXS_PER_ROW);
        std::span<uint32_t> row_offsets{&block_offsets[copied], row_size + 1};

        DBKey key{block.hash, row};
        DBValue value{{block.file_number, block.data_pos}, {row_offsets.begin(), row_offsets.end()}};

        copied += row_size;
        batch.Write(key, value);
    }
    return GetDB().WriteBatch(batch);
}

std::vector<std::byte> LocationsIndex::ReadRawTransaction(const uint256& block_hash, uint32_t i) const
{
    const uint32_t row{i / TXS_PER_ROW};    // used to find the correct DB row
    const uint32_t column{i % TXS_PER_ROW}; // index within a single DB row

    DBValue value{};
    if (!m_db->Read(DBKey{block_hash, row}, value)) {
        return {};
    }

    if (value.offsets.size() <= 1) {
        LogError("%s: LocationsIndex entry for %s:%u must have >1 offsets\n", __func__, block_hash.ToString(), row);
        return {};
    }
    const size_t tx_count{value.offsets.size() - 1};
    if (column >= tx_count) {
        LogError("%s: LocationsIndex entry for %s:%u has %d transactions\n", __func__, block_hash.ToString(), row, tx_count);
        return {};
    }

    FlatFilePos tx_pos{value.block_pos};
    tx_pos.nPos += value.offsets[column];
    size_t tx_size = value.offsets[column + 1] - value.offsets[column];

    AutoFile file{m_chainstate->m_blockman.OpenBlockFile(tx_pos, true)};
    if (file.IsNull()) {
        LogError("%s: OpenBlockFile failed\n", __func__);
        return {};
    }

    std::vector<std::byte> out(tx_size);
    try {
        file.read(MakeWritableByteSpan(out));
        return out;
    } catch (const std::exception& e) {
        LogError("%s: Read %d bytes from %s failed: %s\n", __func__, tx_size, tx_pos.ToString(), e.what());
        return {};
    }
}
