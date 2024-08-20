// Copyright (c) 2014-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <wallet/crypter.h>

#include <vector>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(wallet_crypto_tests, BasicTestingSetup)

class TestCrypter
{
public:
static void TestPassphraseSingle(const std::vector<unsigned char>& salt, const SecureString& passphrase, uint32_t rounds,
                                 const std::vector<unsigned char>& correct_key = {},
                                 const std::vector<unsigned char>& correct_iv = {})
{
    CCrypter crypt;
    crypt.SetKeyFromPassphrase(passphrase, salt, rounds, 0);

    if (!correct_key.empty()) {
        BOOST_CHECK_MESSAGE(memcmp(crypt.vchKey.data(), correct_key.data(), crypt.vchKey.size()) == 0,
            HexStr(crypt.vchKey) + std::string(" != ") + HexStr(correct_key));
    }
    if (!correct_iv.empty()) {
        BOOST_CHECK_MESSAGE(memcmp(crypt.vchIV.data(), correct_iv.data(), crypt.vchIV.size()) == 0,
            HexStr(crypt.vchIV) + std::string(" != ") + HexStr(correct_iv));
    }
}

static void TestPassphrase(const std::vector<unsigned char>& salt, const SecureString& passphrase, uint32_t rounds,
                           const std::vector<unsigned char>& correct_key = {},
                           const std::vector<unsigned char>& correct_iv = {})
{
    TestPassphraseSingle(salt, passphrase, rounds, correct_key, correct_iv);
    for (SecureString::const_iterator it{passphrase.begin()}; it != passphrase.end(); ++it) {
        TestPassphraseSingle(salt, SecureString{it, passphrase.end()}, rounds);
    }
}

static void TestDecrypt(const CCrypter& crypt, const std::vector<unsigned char>& ciphertext,
                        const std::vector<unsigned char>& correct_plaintext = {})
{
    CKeyingMaterial decrypted;
    crypt.Decrypt(ciphertext, decrypted);
    if (!correct_plaintext.empty()) {
        BOOST_CHECK_EQUAL_COLLECTIONS(decrypted.begin(), decrypted.end(), correct_plaintext.begin(), correct_plaintext.end());
    }
}

static void TestEncryptSingle(const CCrypter& crypt, const CKeyingMaterial& plaintext,
                              const std::vector<unsigned char>& correct_ciphertext = {})
{
    std::vector<unsigned char> ciphertext;
    crypt.Encrypt(plaintext, ciphertext);

    if (!correct_ciphertext.empty()) {
        BOOST_CHECK_EQUAL_COLLECTIONS(ciphertext.begin(), ciphertext.end(), correct_ciphertext.begin(), correct_ciphertext.end());
    }

    const std::vector<unsigned char> plaintext2(plaintext.begin(), plaintext.end());
    TestDecrypt(crypt, ciphertext, /*correct_plaintext=*/plaintext2);
}

static void TestEncrypt(const CCrypter& crypt, const std::vector<unsigned char>& plaintext,
                        const std::vector<unsigned char>& correct_ciphertext = {})
{
    TestEncryptSingle(crypt, CKeyingMaterial{plaintext.begin(), plaintext.end()}, correct_ciphertext);
    for (auto it{plaintext.begin()}; it != plaintext.end(); ++it) {
        TestEncryptSingle(crypt, CKeyingMaterial{it, plaintext.end()});
    }
}

};

BOOST_AUTO_TEST_CASE(passphrase) {
    // These are expensive.

    TestCrypter::TestPassphrase(ParseHex("0000deadbeef0000"), "test", 25000,
                                ParseHex("fc7aba077ad5f4c3a0988d8daa4810d0d4a0e3bcb53af662998898f33df0556a"),
                                ParseHex("cf2f2691526dd1aa220896fb8bf7c369"));

    std::string hash(GetRandHash().ToString());
    std::vector<unsigned char> vchSalt(8);
    GetRandBytes(vchSalt);
    uint32_t rounds = m_rng.rand32();
    if (rounds > 30000)
        rounds = 30000;
    TestCrypter::TestPassphrase(vchSalt, SecureString(hash.begin(), hash.end()), rounds);
}

BOOST_AUTO_TEST_CASE(encrypt) {
    std::vector<unsigned char> vchSalt = ParseHex("0000deadbeef0000");
    BOOST_CHECK(vchSalt.size() == WALLET_CRYPTO_SALT_SIZE);
    CCrypter crypt;
    crypt.SetKeyFromPassphrase("passphrase", vchSalt, 25000, 0);
    TestCrypter::TestEncrypt(crypt, ParseHex("22bcade09ac03ff6386914359cfe885cfeb5f77ff0d670f102f619687453b29d"));

    for (int i = 0; i != 100; i++)
    {
        uint256 hash(GetRandHash());
        TestCrypter::TestEncrypt(crypt, std::vector<unsigned char>(hash.begin(), hash.end()));
    }

}

BOOST_AUTO_TEST_CASE(decrypt) {
    std::vector<unsigned char> vchSalt = ParseHex("0000deadbeef0000");
    BOOST_CHECK(vchSalt.size() == WALLET_CRYPTO_SALT_SIZE);
    CCrypter crypt;
    crypt.SetKeyFromPassphrase("passphrase", vchSalt, 25000, 0);

    // Some corner cases the came up while testing
    TestCrypter::TestDecrypt(crypt,ParseHex("795643ce39d736088367822cdc50535ec6f103715e3e48f4f3b1a60a08ef59ca"));
    TestCrypter::TestDecrypt(crypt,ParseHex("de096f4a8f9bd97db012aa9d90d74de8cdea779c3ee8bc7633d8b5d6da703486"));
    TestCrypter::TestDecrypt(crypt,ParseHex("32d0a8974e3afd9c6c3ebf4d66aa4e6419f8c173de25947f98cf8b7ace49449c"));
    TestCrypter::TestDecrypt(crypt,ParseHex("e7c055cca2faa78cb9ac22c9357a90b4778ded9b2cc220a14cea49f931e596ea"));
    TestCrypter::TestDecrypt(crypt,ParseHex("b88efddd668a6801d19516d6830da4ae9811988ccbaf40df8fbb72f3f4d335fd"));
    TestCrypter::TestDecrypt(crypt,ParseHex("8cae76aa6a43694e961ebcb28c8ca8f8540b84153d72865e8561ddd93fa7bfa9"));

    for (int i = 0; i != 100; i++)
    {
        uint256 hash(GetRandHash());
        TestCrypter::TestDecrypt(crypt, std::vector<unsigned char>(hash.begin(), hash.end()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
