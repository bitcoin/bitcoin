// Copyright (c) 2012-present The Bitcoin Core developers
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

BOOST_AUTO_TEST_CASE(walletdb_readonly_database)
{
    // Test read-only database functionality including property detection,
    // batch operations, and LoadWallet behavior

    // Test 1: IsReadOnly() property detection
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

    // Test 2: WalletBatch operations with read-only database
    // First create a writable database and populate it with wallet data
    auto writable_database = CreateMockableWalletDatabase();
    {
        WalletBatch write_batch(*writable_database);

        // Write some wallet data using WalletBatch methods
        BOOST_CHECK(write_batch.WriteWalletFlags(WALLET_FLAG_DESCRIPTORS));
        BOOST_CHECK(write_batch.WriteName("address1", "Alice"));
        BOOST_CHECK(write_batch.WritePurpose("address1", "receive"));
    }

    // Copy the data to create a read-only database
    MockableData wallet_data = dynamic_cast<MockableDatabase&>(*writable_database).m_records;
    auto readonly_database = CreateMockableWalletDatabase(wallet_data, true);
    BOOST_CHECK(readonly_database->IsReadOnly());

    // Test WalletBatch with read-only database
    WalletBatch readonly_batch(*readonly_database);

    // Reading should work through WalletBatch
    std::string read_name, read_purpose;
    BOOST_CHECK(readonly_batch.ReadName("address1", read_name));
    BOOST_CHECK_EQUAL(read_name, "Alice");
    BOOST_CHECK(readonly_batch.ReadPurpose("address1", read_purpose));
    BOOST_CHECK_EQUAL(read_purpose, "receive");

    // Write operations should fail on read-only database
    BOOST_CHECK(!readonly_batch.WriteName("address2", "Bob"));
    BOOST_CHECK(!readonly_batch.WritePurpose("address2", "send"));
    BOOST_CHECK(!readonly_batch.EraseName("address1"));

    // Verify original data is unchanged
    BOOST_CHECK(readonly_batch.ReadName("address1", read_name));
    BOOST_CHECK_EQUAL(read_name, "Alice");

    // Test 3: LoadWallet with read-only databases
    // Create a regular wallet first to populate data
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
    MockableData loadwallet_data = dynamic_cast<MockableDatabase&>(wallet->GetDatabase()).m_records;
    auto readonly_loadwallet_database = CreateMockableWalletDatabase(loadwallet_data, true);
    BOOST_CHECK(readonly_loadwallet_database->IsReadOnly());

    std::shared_ptr<CWallet> readonly_loadwallet_wallet = std::make_shared<CWallet>(nullptr, "", std::move(readonly_loadwallet_database));
    // Don't set wallet flags on read-only database as it's a write operation

    {
        LOCK(readonly_loadwallet_wallet->cs_wallet);
        WalletBatch readonly_batch(readonly_loadwallet_wallet->GetDatabase());

        // LoadWallet should succeed on read-only database
        DBErrors load_result = readonly_batch.LoadWallet(readonly_loadwallet_wallet.get());
        BOOST_CHECK(load_result == DBErrors::LOAD_OK);

        // The readonly wallet should have loaded the flags from the data
        BOOST_CHECK(readonly_loadwallet_wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
