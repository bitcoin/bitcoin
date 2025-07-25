// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_DB_KEY_H
#define BITCOIN_INDEX_DB_KEY_H

#include <dbwrapper.h>
#include <interfaces/types.h>
#include <serialize.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

/*
 * This file includes the logic for the db keys used by blockfilterindex and coinstatsindex.
 * Index data is usually indexed by height, but in case of a reorg, entries of blocks no
 * longer in the main chain will be copied to a hash index by which they can still be queried.
 * Keys for the height index have the type [DB_BLOCK_HEIGHT, uint32 (BE)]. The height is represented
 * as big-endian so that sequential reads of filters by height are fast.
 * Keys for the hash index have the type [DB_BLOCK_HASH, uint256].
 */

static constexpr uint8_t DB_BLOCK_HASH{'s'};
static constexpr uint8_t DB_BLOCK_HEIGHT{'t'};

struct DBHeightKey {
    int height;

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
        const uint8_t prefix{ser_readdata8(s)};
        if (prefix != DB_BLOCK_HEIGHT) {
            throw std::ios_base::failure("Invalid format for index DB height key");
        }
        height = ser_readdata32be(s);
    }
};

struct DBHashKey {
    uint256 hash;

    explicit DBHashKey(const uint256& hash_in) : hash(hash_in) {}

    SERIALIZE_METHODS(DBHashKey, obj) {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for index DB hash key");
        }

        READWRITE(obj.hash);
    }
};

template <typename DBVal>
[[nodiscard]] static bool CopyHeightIndexToHashIndex(CDBIterator& db_it, CDBBatch& batch,
                                                     const std::string& index_name, int height)
{
    DBHeightKey key(height);
    db_it.Seek(key);

    if (!db_it.GetKey(key) || key.height != height) {
        LogError("unexpected key in %s: expected (%c, %d)",
                  index_name, DB_BLOCK_HEIGHT, height);
        return false;
    }

    std::pair<uint256, DBVal> value;
    if (!db_it.GetValue(value)) {
        LogError("unable to read value in %s at key (%c, %d)",
                 index_name, DB_BLOCK_HEIGHT, height);
        return false;
    }

    batch.Write(DBHashKey(value.first), std::move(value.second));
    return true;
}

#endif // BITCOIN_INDEX_DB_KEY_H
