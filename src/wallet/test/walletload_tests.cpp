// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

namespace wallet {

BOOST_AUTO_TEST_SUITE(walletload_tests)

class DummyDescriptor final : public Descriptor {
private:
    std::string desc;
public:
    explicit DummyDescriptor(const std::string& descriptor) : desc(descriptor) {};
    ~DummyDescriptor() = default;

    std::string ToString() const override { return desc; }
    std::optional<OutputType> GetOutputType() const override { return OutputType::UNKNOWN; }

    bool IsRange() const override { return false; }
    bool IsSolvable() const override { return false; }
    bool IsSingleType() const override { return true; }
    bool ToPrivateString(const SigningProvider& provider, std::string& out) const override { return false; }
    bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const override { return false; }
    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const override { return false; };
    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override { return false; }
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const override {}
};

BOOST_FIXTURE_TEST_CASE(wallet_load_unknown_descriptor, TestingSetup)
{
    std::unique_ptr<WalletDatabase> database = CreateMockWalletDatabase();
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

BOOST_FIXTURE_TEST_CASE(wallet_load_verif_crypted_key_checksum, TestingSetup)
{
    // The test duplicates the db so each case has its own db instance.
    int NUMBER_OF_TESTS = 4;
    std::vector<std::unique_ptr<WalletDatabase>> dbs;
    CKey first_key;
    auto get_db = [](std::vector<std::unique_ptr<WalletDatabase>>& dbs) {
        std::unique_ptr<WalletDatabase> db = std::move(dbs.back());
        dbs.pop_back();
        return db;
    };

    {   // Context setup.
        // Create and encrypt legacy wallet
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", CreateMockWalletDatabase()));
        LOCK(wallet->cs_wallet);
        auto legacy_spkm = wallet->GetOrCreateLegacyScriptPubKeyMan();
        BOOST_CHECK(legacy_spkm->SetupGeneration(true));

        // Get the first key in the wallet
        CTxDestination dest = *Assert(legacy_spkm->GetNewDestination(OutputType::LEGACY));
        CKeyID key_id = GetKeyForDestination(*legacy_spkm, dest);
        BOOST_CHECK(legacy_spkm->GetKey(key_id, first_key));

        // Encrypt the wallet and duplicate database
        BOOST_CHECK(wallet->EncryptWallet("encrypt"));
        wallet->Flush();

        DatabaseOptions options;
        for (int i=0; i < NUMBER_OF_TESTS; i++) {
            dbs.emplace_back(DuplicateMockDatabase(wallet->GetDatabase(), options));
        }
    }

    {
        // First test case:
        // Erase all the crypted keys from db and unlock the wallet.
        // The wallet will only re-write the crypted keys to db if any checksum is missing at load time.
        // So, if any 'ckey' record re-appears on db, then the checksums were not properly calculated, and we are re-writing
        // the records every time that 'CWallet::Unlock' gets called, which is not good.

        // Load the wallet and check that is encrypted
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", get_db(dbs)));
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
        std::unique_ptr<WalletDatabase> db = get_db(dbs);
        {
            std::unique_ptr<DatabaseBatch> batch = db->MakeBatch(false);
            std::pair<std::vector<unsigned char>, uint256> value;
            BOOST_CHECK(batch->Read(std::make_pair(DBKeys::CRYPTED_KEY, first_key.GetPubKey()), value));

            const auto key = std::make_pair(DBKeys::CRYPTED_KEY, first_key.GetPubKey());
            BOOST_CHECK(batch->Write(key, value.first, /*fOverwrite=*/true));
        }

        // Load the wallet and check that is encrypted
        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
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
        std::unique_ptr<WalletDatabase> db = get_db(dbs);
        {
            std::unique_ptr<DatabaseBatch> batch = db->MakeBatch(false);
            std::vector<unsigned char> crypted_data;
            BOOST_CHECK(batch->Read(std::make_pair(DBKeys::CRYPTED_KEY, first_key.GetPubKey()), crypted_data));

            // Write an invalid checksum
            std::pair<std::vector<unsigned char>, uint256> value = std::make_pair(crypted_data, uint256::ONE);
            const auto key = std::make_pair(DBKeys::CRYPTED_KEY, first_key.GetPubKey());
            BOOST_CHECK(batch->Write(key, value, /*fOverwrite=*/true));
        }

        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
    }

    {
        // Fourth test case:
        // Verify that loading up a 'ckey' with an invalid pubkey throws an error
        std::unique_ptr<WalletDatabase> db = get_db(dbs);
        {
            CPubKey invalid_key;
            BOOST_ASSERT(!invalid_key.IsValid());
            const auto key = std::make_pair(DBKeys::CRYPTED_KEY, invalid_key);
            std::pair<std::vector<unsigned char>, uint256> value;
            BOOST_CHECK(db->MakeBatch(false)->Write(key, value, /*fOverwrite=*/true));
        }

        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
        BOOST_CHECK_EQUAL(wallet->LoadWallet(), DBErrors::CORRUPT);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
