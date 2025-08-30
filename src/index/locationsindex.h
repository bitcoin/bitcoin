// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_LOCATIONSINDEX_H
#define BITCOIN_INDEX_LOCATIONSINDEX_H

#include <attributes.h>
#include <chain.h>
#include <index/base.h>

static constexpr bool DEFAULT_LOCATIONSINDEX{false};

/**
 * LocationsIndex is used to store and retrieve the location of transactions within a block.
 *
 * This is done in a compressed manner, allowing the whole index to fit in RAM on decent hardware.
 * For example, the on-disk size was 2.8 GiB at block height 912371.
 */
class LocationsIndex final : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;

    bool AllowPrune() const override { return true; }

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const LIFETIMEBOUND override { return *m_db; }

public:
    // Constructs the index, which becomes available to be queried.
    explicit LocationsIndex(std::unique_ptr<interfaces::Chain> chain,
                            size_t cache_size, bool memory_only = false, bool wipe = false);

    // Read transaction #i from a given block.
    std::vector<std::byte> ReadRawTransaction(const uint256& block_hash, uint32_t i) const;
};

extern std::unique_ptr<LocationsIndex> g_locations_index;

#endif // BITCOIN_INDEX_LOCATIONSINDEX_H
