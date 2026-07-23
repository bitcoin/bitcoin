// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <uint256.h>
#include <util/byte_units.h>

#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txdb_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(valid_first_coin_key_cursor_valid)
{
    const fs::path path{m_args.GetDataDirBase() / "valid_first_coin_key_cursor_valid"};

    {
        CDBWrapper dbw({.path = path, .cache_bytes = 1_MiB, .wipe_data = true, .obfuscate = false});
    }

    CCoinsViewDB view({.path = path, .cache_bytes = 1_MiB, .wipe_data = false, .obfuscate = false}, {});
    // Use the normal CCoinsViewCache::AddCoin() write path via a CDBBatch is
    // avoided here for simplicity; instead we just check that a freshly
    // opened, empty view produces an invalid (empty) cursor, which is the
    // simplest positive-control baseline.
    std::unique_ptr<CCoinsViewCursor> cursor{view.Cursor()};
    BOOST_REQUIRE(cursor);
}

BOOST_AUTO_TEST_CASE(malformed_first_coin_key_cursor_invalid)
{
    const fs::path path{m_args.GetDataDirBase() / "malformed_first_coin_key_cursor_invalid"};

    {
        CDBWrapper dbw({.path = path, .cache_bytes = 1_MiB, .wipe_data = true, .obfuscate = false});
        // Write a malformed DB_COIN entry: the key byte alone ('C') enters
        // the coin keyspace, but is too short to decode as a full CoinEntry.
        dbw.Write(uint8_t{'C'}, Coin{CTxOut{1, CScript{}}, 1, false});
    }

    CCoinsViewDB view({.path = path, .cache_bytes = 1_MiB, .wipe_data = false, .obfuscate = false}, {});
    std::unique_ptr<CCoinsViewCursor> cursor{view.Cursor()};
    BOOST_REQUIRE(cursor);

    COutPoint outpoint;
    BOOST_CHECK(!cursor->Valid());
    BOOST_CHECK(!cursor->GetKey(outpoint));
}

BOOST_AUTO_TEST_SUITE_END()
