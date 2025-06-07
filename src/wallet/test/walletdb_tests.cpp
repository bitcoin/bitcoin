// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <clientversion.h>
#include <streams.h>
#include <uint256.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(walletdb_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(walletdb_readkeyvalue)
{
    /**
     * When ReadKeyValue() reads from either a "key" or "wkey" it first reads the DataStream into a
     * CPrivKey or CWalletKey respectively and then reads a hash of the pubkey and privkey into a uint256.
     * Wallets from 0.8 or before do not store the pubkey/privkey hash, trying to read the hash from old
     * wallets throws an exception, for backwards compatibility this read is wrapped in a try block to
     * silently fail. The test here makes sure the type of exception thrown from DataStream::read()
     * matches the type we expect, otherwise we need to update the "key"/"wkey" exception type caught.
     */
    DataStream ssValue{};
    uint256 dummy;
    BOOST_CHECK_THROW(ssValue >> dummy, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(walletdb_readonly_loadwallet)
{
    // Test that LoadWallet works correctly with read-only databases
    // and skips write operations appropriately

    // Create a regular wallet first
    auto database = CreateMockableWalletDatabase();
    BOOST_CHECK(!database->IsReadOnly());

    // Create a minimal wallet and load it with write access
    std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(nullptr, "", std::move(database));
    wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    {
        LOCK(wallet->cs_wallet);
        WalletBatch batch(wallet->GetDatabase());

        // Write some basic wallet data
        batch.WriteWalletFlags(WALLET_FLAG_DESCRIPTORS);
        BOOST_CHECK(batch.TxnBegin());
        BOOST_CHECK(batch.TxnCommit());

        // Load the wallet to establish baseline
        DBErrors load_result = batch.LoadWallet(wallet.get());
        BOOST_CHECK(load_result == DBErrors::LOAD_OK);
    }

    // Now test with read-only database
    MockableData wallet_data = dynamic_cast<MockableDatabase&>(wallet->GetDatabase()).m_records;
    auto readonly_database = CreateMockableWalletDatabase(wallet_data, true);
    BOOST_CHECK(readonly_database->IsReadOnly());

    std::shared_ptr<CWallet> readonly_wallet = std::make_shared<CWallet>(nullptr, "", std::move(readonly_database));
    readonly_wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);

    {
        LOCK(readonly_wallet->cs_wallet);
        WalletBatch readonly_batch(readonly_wallet->GetDatabase());

        // LoadWallet should succeed on read-only database
        DBErrors load_result = readonly_batch.LoadWallet(readonly_wallet.get());
        BOOST_CHECK(load_result == DBErrors::LOAD_OK);

        // The readonly wallet should have the same flags as the original
        BOOST_CHECK(readonly_wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    }
}

BOOST_AUTO_TEST_CASE(walletdb_readonly_batch_operations)
{
    // Test WalletBatch operations with read-only database
    MockableData initial_data;

    // Pre-populate with some wallet-like data
    DataStream key_stream, value_stream;
    key_stream << std::make_pair(DBKeys::VERSION, std::string{});
    value_stream << CLIENT_VERSION;
    initial_data[SerializeData(key_stream.begin(), key_stream.end())] = SerializeData(value_stream.begin(), value_stream.end());

    key_stream.clear();
    value_stream.clear();
    key_stream << std::make_pair(DBKeys::FLAGS, std::string{});
    value_stream << uint64_t{WALLET_FLAG_DESCRIPTORS};
    initial_data[SerializeData(key_stream.begin(), key_stream.end())] = SerializeData(value_stream.begin(), value_stream.end());

    auto readonly_database = CreateMockableWalletDatabase(initial_data, true);

    // Test that we can create a batch and perform read operations
    std::unique_ptr<DatabaseBatch> batch = readonly_database->MakeBatch();

    // Reading should work
    int version;
    BOOST_CHECK(batch->Read(std::make_pair(DBKeys::VERSION, std::string{}), version));
    BOOST_CHECK_EQUAL(version, CLIENT_VERSION);

    uint64_t flags;
    BOOST_CHECK(batch->Read(std::make_pair(DBKeys::FLAGS, std::string{}), flags));
    BOOST_CHECK_EQUAL(flags, WALLET_FLAG_DESCRIPTORS);
}

BOOST_AUTO_TEST_CASE(walletdb_readonly_database_property)
{
    // Test that WalletDatabase.IsReadOnly() property is correctly exposed

    // Regular database should not be read-only
    auto regular_db = CreateMockableWalletDatabase();
    BOOST_CHECK(!regular_db->IsReadOnly());

    // Read-only database should report as read-only
    auto readonly_db = CreateMockableWalletDatabase({}, true);
    BOOST_CHECK(readonly_db->IsReadOnly());

    // Test with wallet instances
    std::shared_ptr<CWallet> regular_wallet = std::make_shared<CWallet>(nullptr, "", std::move(regular_db));
    BOOST_CHECK(!regular_wallet->GetDatabase().IsReadOnly());

    std::shared_ptr<CWallet> readonly_wallet = std::make_shared<CWallet>(nullptr, "", std::move(readonly_db));
    BOOST_CHECK(readonly_wallet->GetDatabase().IsReadOnly());
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
