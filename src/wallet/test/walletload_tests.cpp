// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <script/descriptor.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>

namespace wallet {

BOOST_AUTO_TEST_SUITE(walletload_tests)

class DummyDescriptor final : public Descriptor {
private:
    std::string desc;
public:
    explicit DummyDescriptor(const std::string& descriptor) : desc(descriptor) {};
    ~DummyDescriptor() = default;

    std::string ToString(bool compat_format) const override { return desc; }
    std::string ToCanonicalString() const override { return desc; }
    std::optional<OutputType> GetOutputType() const override { return OutputType::UNKNOWN; }

    bool IsRange() const override { return true; }
    bool IsSolvable() const override { return false; }
    bool IsSingleType() const override { return true; }
    bool HavePrivateKeys(const SigningProvider&) const override { return false; }
    bool ToPrivateString(const SigningProvider& provider, std::string& out) const override { return false; }
    bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const override { return false; }
    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const override { return false; };
    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const override { return false; }
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const override {}
    std::optional<int64_t> ScriptSize() const override { return {}; }
    std::optional<int64_t> MaxSatisfactionWeight(bool) const override { return {}; }
    std::optional<int64_t> MaxSatisfactionElems() const override { return {}; }
    void GetPubKeys(std::set<CPubKey>& pubkeys, std::set<CExtPubKey>& ext_pubs) const override {}
    bool HasScripts() const override { return true; }
    std::vector<std::string> Warnings() const override { return {}; }
    uint32_t GetMaxKeyExpr() const override { return 0; }
    size_t GetKeyCount() const override { return 0; }
    bool CanSelfExpand() const final { return false; }
};

BOOST_FIXTURE_TEST_CASE(wallet_load_descriptors, TestingSetup)
{
    bilingual_str _error;
    std::vector<bilingual_str> _warnings;
    std::unique_ptr<WalletDatabase> database = CreateMockableWalletDatabase();
    {
        // Write unknown active descriptor
        WalletBatch batch(*database);
        std::string unknown_desc = "trx(tpubD6NzVbkrYhZ4Y4S7m6Y5s9GD8FqEMBy56AGphZXuagajudVZEnYyBahZMgHNCTJc2at82YX6s8JiL1Lohu5A3v1Ur76qguNH4QVQ7qYrBQx/86'/1'/0'/0/*)#8pn8tzdt";
        WalletDescriptor wallet_descriptor(std::make_shared<DummyDescriptor>(unknown_desc), 0, 0, 0, 0);
        BOOST_CHECK(batch.WriteDescriptor(uint256(), wallet_descriptor));
        BOOST_CHECK(batch.WriteActiveScriptPubKeyMan(static_cast<uint8_t>(OutputType::UNKNOWN), uint256(), false));
    }

    {
        // Now try to load the wallet and verify the error.
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(_error, _warnings), DBErrors::UNKNOWN_DESCRIPTOR);
    }

    // Test 2
    // Now write a valid descriptor with a different ID which must be accepted
    database = CreateMockableWalletDatabase();

    {
        // Write valid descriptor with arbitrary ID
        WalletBatch batch(*database);
        std::string desc = "wpkh([d34db33f/84h/0h/0h]xpub6DJ2dNUysrn5Vt36jH2KLBT2i1auw1tTSSomg8PhqNiUtx8QX2SvC9nrHu81fT41fvDUnhMjEzQgXnQjKEu3oaqMSzhSrHMxyyoEAmUHQbY/0/*)#cjjspncu";
        WalletDescriptor wallet_descriptor(std::make_shared<DummyDescriptor>(desc), 0, 0, 0, 0);
        BOOST_CHECK(batch.WriteDescriptor(uint256::ONE, wallet_descriptor));
    }

    {
        // Now try to load the wallet and verify the result.
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(_error, _warnings), DBErrors::LOAD_OK);
    }
}

BOOST_FIXTURE_TEST_CASE(wallet_load_descriptor_cache_invalid_xpub, TestingSetup)
{
    // A cache record whose serialized xpub is not exactly BIP32_EXTKEY_SIZE bytes, or is not
    // a valid public key, must be rejected as corruption.
    bilingual_str error;
    std::vector<bilingual_str> warnings;

    const std::string desc_str = "wpkh([d34db33f/84h/0h/0h]xpub6DJ2dNUysrn5Vt36jH2KLBT2i1auw1tTSSomg8PhqNiUtx8QX2SvC9nrHu81fT41fvDUnhMjEzQgXnQjKEu3oaqMSzhSrHMxyyoEAmUHQbY/0/*)#cjjspncu";
    FlatSigningProvider keys;
    std::string parse_error;
    std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, parse_error, /*require_checksum=*/true);
    BOOST_REQUIRE_MESSAGE(descs.size() == 1, parse_error);
    std::shared_ptr<Descriptor> descriptor = std::move(descs.at(0));
    const uint256 desc_id = CompatDescriptorHash(*descriptor);

    auto make_db_with_cache_xpub = [&](const std::string& cache_type, const std::vector<unsigned char>& xpub) {
        std::unique_ptr<WalletDatabase> database = CreateMockableWalletDatabase();
        {
            WalletBatch batch(*database);
            WalletDescriptor wallet_descriptor(descriptor, 0, 0, 0, 0);
            BOOST_CHECK(batch.WriteDescriptor(desc_id, wallet_descriptor));
        }
        std::unique_ptr<DatabaseBatch> raw = database->MakeBatch();
        BOOST_CHECK(raw->Write(std::make_pair(std::make_pair(cache_type, desc_id), uint32_t{0}), xpub));
        return database;
    };

    const std::vector<unsigned char> short_xpub(BIP32_EXTKEY_SIZE - 1, 0);
    {
        std::unique_ptr<WalletDatabase> database = make_db_with_cache_xpub("walletdescriptorcache", short_xpub);
        ASSERT_DEBUG_LOG("descriptor cache xpub has invalid size");
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(error, warnings), DBErrors::CORRUPT);
    }

    {
        std::unique_ptr<WalletDatabase> database = make_db_with_cache_xpub("walletdescriptorlhcache", short_xpub);
        ASSERT_DEBUG_LOG("descriptor last hardened cache xpub has invalid size");
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(error, warnings), DBErrors::CORRUPT);
    }

    // A record of the right size whose public key is not valid must be rejected as well.
    // The public key starts after the depth (1), fingerprint (4), child number (4) and chain code (32).
    constexpr size_t pubkey_pos{41};
    std::vector<unsigned char> bad_prefix(BIP32_EXTKEY_SIZE, 0);
    bad_prefix[pubkey_pos] = 0x05;
    std::vector<unsigned char> off_curve(BIP32_EXTKEY_SIZE, 0);
    off_curve[pubkey_pos] = 0x02;
    off_curve.back() = 0x05; // x = 5 is not the x coordinate of a point on the curve
    // A valid public key with depth 0 and a non-zero child number is rejected by deserialization.
    std::vector<unsigned char> depth0_child(BIP32_EXTKEY_SIZE, 0);
    depth0_child[8] = 0x01;
    const auto generator{ParseHex("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")};
    std::copy(generator.begin(), generator.end(), depth0_child.begin() + pubkey_pos);
    for (const auto& xpub : {bad_prefix, off_curve, depth0_child}) {
        std::unique_ptr<WalletDatabase> database = make_db_with_cache_xpub("walletdescriptorcache", xpub);
        ASSERT_DEBUG_LOG("descriptor cache xpub is invalid");
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(error, warnings), DBErrors::CORRUPT);
    }
    for (const auto& xpub : {bad_prefix, off_curve, depth0_child}) {
        std::unique_ptr<WalletDatabase> database = make_db_with_cache_xpub("walletdescriptorlhcache", xpub);
        ASSERT_DEBUG_LOG("descriptor last hardened cache xpub is invalid");
        const std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(database)));
        BOOST_CHECK_EQUAL(wallet->PopulateWalletFromDB(error, warnings), DBErrors::CORRUPT);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
