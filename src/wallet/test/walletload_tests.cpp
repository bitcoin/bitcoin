// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

namespace wallet {

BOOST_AUTO_TEST_SUITE(walletload_tests)

class DummyDescriptor final : public Descriptor
{
private:
    std::string desc;

public:
    explicit DummyDescriptor(const std::string& descriptor) : desc(descriptor){};
    ~DummyDescriptor() = default;

    std::string ToString(bool compat_format) const override { return desc; }
    std::optional<OutputType> GetOutputType() const override { return OutputType::UNKNOWN; }

    bool IsRange() const override { return false; }
    bool IsSolvable() const override { return false; }
    bool IsSingleType() const override { return true; }
    bool ToPrivateString(const SigningProvider& provider, std::string& out) const override { return false; }
    bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const override { return false; }
    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const override { return false; };
    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override { return false; }
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const override {}
    std::optional<int64_t> ScriptSize() const override { return {}; }
    std::optional<int64_t> MaxSatisfactionWeight(bool) const override { return {}; }
    std::optional<int64_t> MaxSatisfactionElems() const override { return {}; }
};

BOOST_FIXTURE_TEST_CASE(wallet_load_descriptors, TestingSetup)
{
    std::unique_ptr<WalletDatabase> database = CreateMockableWalletDatabase();
    {
        // Write unknown active descriptor
        WalletBatch batch(*database, false);
        std::string unknown_desc = "trx(tpubD6NzVbkrYhZ4Y4S7m6Y5s9GD8FqEMBy56AGphZXuagajudVZEnYyBahZMgHNCTJc2at82YX6s8JiL1Lohu5A3v1Ur76qguNH4QVQ7qYrBQx/86'/1'/0'/0/*)#8pn8tzdt";
        WalletDescriptor wallet_descriptor(std::make_shared<DummyDescriptor>(unknown_desc), 0, 0, 0, 0);
        BOOST_CHECK(batch.WriteDescriptor(uint256(), wallet_descriptor));
        BOOST_CHECK(batch.WriteActiveScriptPubKeyMan(static_cast<uint8_t>(OutputType::UNKNOWN), uint256(), false));
    }

    {
        // Now try to load the wallet and verify the error.
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::UNKNOWN_DESCRIPTOR);
    }

    // Test 2
    // Now write a valid descriptor with an invalid ID.
    // As the software produces another ID for the descriptor, the loading process must be aborted.
    database = CreateMockableWalletDatabase();

    // Verify the error
    bool found = false;
    DebugLogHelper logHelper("The descriptor ID calculated by the wallet differs from the one in DB", [&](const std::string* s) {
        found = true;
        return false;
    });

    {
        // Write valid descriptor with invalid ID
        WalletBatch batch(*database, false);
        std::string desc = "wpkh([d34db33f/84h/0h/0h]xpub6DJ2dNUysrn5Vt36jH2KLBT2i1auw1tTSSomg8PhqNiUtx8QX2SvC9nrHu81fT41fvDUnhMjEzQgXnQjKEu3oaqMSzhSrHMxyyoEAmUHQbY/0/*)#cjjspncu";
        WalletDescriptor wallet_descriptor(std::make_shared<DummyDescriptor>(desc), 0, 0, 0, 0);
        BOOST_CHECK(batch.WriteDescriptor(uint256::ONE, wallet_descriptor));
    }

    {
        // Now try to load the wallet and verify the error.
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
        BOOST_CHECK(found); // The error must be logged
    }
}

bool HasAnyRecordOfType(WalletDatabase& db, const std::string& key)
{
    std::unique_ptr<DatabaseBatch> batch = db.MakeBatch(false);
    BOOST_CHECK(batch);
    std::unique_ptr<DatabaseCursor> cursor = batch->GetNewCursor();
    BOOST_CHECK(cursor);
    while (true) {
        DataStream ssKey{};
        DataStream ssValue{};
        DatabaseCursor::Status status = cursor->Next(ssKey, ssValue);
        assert(status != DatabaseCursor::Status::FAIL);
        if (status == DatabaseCursor::Status::DONE) break;
        std::string type;
        ssKey >> type;
        if (type == key) return true;
    }
    return false;
}

template<typename... Args>
SerializeData MakeSerializeData(const Args&... args)
{
    DataStream s{};
    SerializeMany(s, args...);
    return {s.begin(), s.end()};
}

BOOST_FIXTURE_TEST_CASE(wallet_load_ckey, TestingSetup)
{
    SerializeData ckey_record_key;
    SerializeData ckey_record_value;
    MockableData records;

    {
        // Context setup.
        // Create and encrypt legacy wallet
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase()));
        LOCK(wallet->cs_wallet);
        auto legacy_spkm = wallet->GetOrCreateLegacyScriptPubKeyMan();
        BOOST_CHECK(legacy_spkm->SetupGeneration(true));

        // Retrieve a key
        CTxDestination dest = *Assert(legacy_spkm->GetNewDestination(OutputType::LEGACY));
        CKeyID key_id = GetKeyForDestination(*legacy_spkm, dest);
        CKey first_key;
        BOOST_CHECK(legacy_spkm->GetKey(key_id, first_key));

        // Encrypt the wallet
        BOOST_CHECK(wallet->EncryptWallet("encrypt"));
        wallet->Flush();

        // Store a copy of all the records
        records = GetMockableDatabase(*wallet).m_records;

        // Get the record for the retrieved key
        ckey_record_key = MakeSerializeData(DBKeys::CRYPTED_KEY, first_key.GetPubKey());
        ckey_record_value = records.at(ckey_record_key);
    }

    {
        // First test case:
        // Erase all the crypted keys from db and unlock the wallet.
        // The wallet will only re-write the crypted keys to db if any checksum is missing at load time.
        // So, if any 'ckey' record re-appears on db, then the checksums were not properly calculated, and we are re-writing
        // the records every time that 'CWallet::Unlock' gets called, which is not good.

        // Load the wallet and check that is encrypted
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase(records)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::LOAD_OK);
        BOOST_CHECK(wallet->IsCrypted());
        BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));

        // Now delete all records and check that the 'Unlock' function doesn't re-write them
        BOOST_CHECK(wallet->GetLegacyScriptPubKeyMan()->DeleteRecords());
        BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));
        BOOST_CHECK(wallet->Unlock("encrypt"));
        BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));
    }

    {
        // Second test case:
        // Verify that loading up a 'ckey' with no checksum triggers a complete re-write of the crypted keys.

        // Cut off the 32 byte checksum from a ckey record
        records[ckey_record_key].resize(ckey_record_value.size() - 32);

        // Load the wallet and check that is encrypted
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase(records)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::LOAD_OK);
        BOOST_CHECK(wallet->IsCrypted());
        BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));

        // Now delete all ckey records and check that the 'Unlock' function re-writes them
        // (this is because the wallet, at load time, found a ckey record with no checksum)
        BOOST_CHECK(wallet->GetLegacyScriptPubKeyMan()->DeleteRecords());
        BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));
        BOOST_CHECK(wallet->Unlock("encrypt"));
        BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_KEY));
    }

    {
        // Third test case:
        // Verify that loading up a 'ckey' with an invalid checksum throws an error.

        // Cut off the 32 byte checksum from a ckey record
        records[ckey_record_key].resize(ckey_record_value.size() - 32);
        // Fill in the checksum space with 0s
        records[ckey_record_key].resize(ckey_record_value.size());

        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase(records)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
    }

    {
        // Fourth test case:
        // Verify that loading up a 'ckey' with an invalid pubkey throws an error
        CPubKey invalid_key;
        BOOST_CHECK(!invalid_key.IsValid());
        SerializeData key = MakeSerializeData(DBKeys::CRYPTED_KEY, invalid_key);
        records[key] = ckey_record_value;

        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase(records)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
    }
}

// TODO: Fix this test need some help from @aguycalled
// BOOST_FIXTURE_TEST_CASE(wallet_load_verif_crypted_blsct, TestingSetup)
// {
//     // The test duplicates the db so each case has its own db instance.
//     int NUMBER_OF_TESTS = 5;
//     std::vector<std::unique_ptr<WalletDatabase>> dbs;
//     blsct::PrivateKey viewKey, spendKey, tokenKey;
//     blsct::DoublePublicKey dest;
//
//     DatabaseOptions options;
//     options.create_flags |= WALLET_FLAG_BLSCT;
//
//     auto get_db = [](std::vector<std::unique_ptr<WalletDatabase>>& dbs) {
//         std::unique_ptr<WalletDatabase> db = std::move(dbs.back());
//         dbs.pop_back();
//         return db;
//     };
//
//     { // Context setup.
//         // Create and encrypt blsct wallet
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockableWalletDatabase(options)));
//         LOCK(wallet->cs_wallet);
//         auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
//         BOOST_CHECK(blsct_km->SetupGeneration(true));
//
//         // Get the keys in the wallet before encryption
//         auto masterKeysMetadata = blsct_km->GetHDChain();
//         blsct::SubAddress recvAddress = blsct_km->GetSubAddress();
//         dest = recvAddress.GetKeys();
//         viewKey = blsct_km->viewKey;
//         BOOST_CHECK(viewKey.IsValid());
//         BOOST_CHECK(blsct_km->GetKey(masterKeysMetadata.spend_id, spendKey));
//         BOOST_CHECK(blsct_km->GetKey(masterKeysMetadata.token_id, tokenKey));
//
//         // Encrypt the wallet and duplicate database
//         BOOST_CHECK(wallet->EncryptWallet("encrypt"));
//         wallet->Flush();
//
//         for (int i = 0; i < NUMBER_OF_TESTS; i++) {
//             dbs.emplace_back(DuplicateMockDatabase(wallet->GetDatabase(), options));
//         }
//     }
//
//     {
//         // First test case:
//         // Erase all the crypted keys from db and unlock the wallet.
//         // The wallet will only re-write the crypted keys to db if any checksum is missing at load time.
//         // So, if any 'cblsctkey' record re-appears on db, then the checksums were not properly calculated, and we are re-writing
//         // the records every time that 'CWallet::Unlock' gets called, which is not good.
//
//         // Load the wallet and check that is encrypted
//
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", get_db(dbs)));
//
//         BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::LOAD_OK);
//         BOOST_CHECK(wallet->IsCrypted());
//         BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//
//         // Now delete all records and check that the 'Unlock' function doesn't re-write them
//         BOOST_CHECK(wallet->GetBLSCTKeyMan()->DeleteRecords());
//         BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//         BOOST_CHECK(wallet->Unlock("encrypt"));
//         BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//     }
//
//     {
//         // Second test case:
//         // Verify that loading up a 'cblsctkey' with no checksum triggers a complete re-write of the crypted keys.
//         std::unique_ptr<WalletDatabase> db = get_db(dbs);
//         {
//             std::unique_ptr<DatabaseBatch> batch = db->MakeBatch(false);
//             std::pair<std::vector<unsigned char>, uint256> value;
//             BOOST_CHECK(batch->Read(std::make_pair(DBKeys::CRYPTED_BLSCTKEY, spendKey.GetPublicKey()), value));
//
//             const auto key = std::make_pair(DBKeys::CRYPTED_BLSCTKEY, spendKey.GetPublicKey());
//             BOOST_CHECK(batch->Write(key, value.first, /*fOverwrite=*/true));
//         }
//
//         // Load the wallet and check that is encrypted
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
//         BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::LOAD_OK);
//         BOOST_CHECK(wallet->IsCrypted());
//         BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//
//         // Now delete all cblsctkey records and check that the 'Unlock' function re-writes them
//         // (this is because the wallet, at load time, found a cblsctkey record with no checksum)
//         BOOST_CHECK(wallet->GetBLSCTKeyMan()->DeleteKeys());
//         BOOST_CHECK(!HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//         BOOST_CHECK(wallet->Unlock("encrypt"));
//         BOOST_CHECK(HasAnyRecordOfType(wallet->GetDatabase(), DBKeys::CRYPTED_BLSCTKEY));
//     }
//
//     {
//         // Third test case:
//         // Verify that loading up a 'cblsctkey' with an invalid checksum throws an error.
//         std::unique_ptr<WalletDatabase> db = get_db(dbs);
//         {
//             std::unique_ptr<DatabaseBatch> batch = db->MakeBatch(false);
//             std::vector<unsigned char> crypted_data;
//             BOOST_CHECK(batch->Read(std::make_pair(DBKeys::CRYPTED_BLSCTKEY, spendKey.GetPublicKey()), crypted_data));
//
//             // Write an invalid checksum
//             std::pair<std::vector<unsigned char>, uint256> value = std::make_pair(crypted_data, uint256::ONE);
//             const auto key = std::make_pair(DBKeys::CRYPTED_BLSCTKEY, spendKey.GetPublicKey());
//             BOOST_CHECK(batch->Write(key, value, /*fOverwrite=*/true));
//         }
//
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
//         BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
//     }
//
//     {
//         // Fourth test case:
//         // Verify that loading up a 'cblsctkey' with an invalid pubkey throws an error
//         std::unique_ptr<WalletDatabase> db = get_db(dbs);
//         {
//             CPubKey invalid_key;
//             BOOST_CHECK(!invalid_key.IsValid());
//             const auto key = std::make_pair(DBKeys::CRYPTED_KEY, invalid_key);
//             std::pair<std::vector<unsigned char>, uint256> value;
//             BOOST_CHECK(db->MakeBatch(false)->Write(key, value, /*fOverwrite=*/true));
//         }
//
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
//         BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
//     }
//
//     {
//         // Fifth test case:
//         // Verify that keys and addresses are not re-generated after encryption
//         std::unique_ptr<WalletDatabase> db = get_db(dbs);
//         std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
//         BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::LOAD_OK);
//
//         blsct::PrivateKey viewKey2, spendKey2, tokenKey2;
//         auto blsct_km = wallet->GetBLSCTKeyMan();
//         BOOST_CHECK(blsct_km != nullptr);
//
//         // Get the keys in the wallet before encryption
//         auto masterKeysMetadata = blsct_km->GetHDChain();
//         blsct::SubAddress recvAddress = blsct_km->GetSubAddress();
//         blsct::DoublePublicKey dest2 = recvAddress.GetKeys();
//         viewKey2 = blsct_km->viewKey;
//         BOOST_CHECK(viewKey.IsValid());
//         BOOST_CHECK(!blsct_km->GetKey(masterKeysMetadata.spend_id, spendKey2));
//         BOOST_CHECK(!blsct_km->GetKey(masterKeysMetadata.token_id, tokenKey2));
//         BOOST_CHECK(wallet->Unlock("encrypt"));
//         BOOST_CHECK(blsct_km->GetKey(masterKeysMetadata.spend_id, spendKey2));
//         BOOST_CHECK(blsct_km->GetKey(masterKeysMetadata.token_id, tokenKey2));
//
//         BOOST_CHECK(dest == dest2);
//         BOOST_CHECK(viewKey == viewKey2);
//         BOOST_CHECK(spendKey == spendKey2);
//         BOOST_CHECK(tokenKey == tokenKey2);
//     }
// }

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
