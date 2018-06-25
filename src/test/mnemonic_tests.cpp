// Copyright (c) 2017-2018 The Particl developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key/mnemonic.h>

#include <test/data/bip39_vectors_english.json.h>
#include <test/data/bip39_vectors_japanese.json.h>

#include <test/test_bitcoin.h>

#include <key/extkey.h>
#include <key_io.h>
#include <util.h>
#include <utilstrencodings.h>

#include <string>
#include <boost/test/unit_test.hpp>

#include <univalue.h>

extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(mnemonic_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(mnemonic_test)
{
    std::string words = "deer clever bitter bonus unable menu satoshi chaos dwarf inmate robot drama exist nuclear raise";
    std::string expect_seed = "1da563986981b82c17a76160934f4b532eac77e14b632c6adcf31ba4166913e063ce158164c512cdce0672cbc9256dd81e7be23a8d8eb331de1a497493c382b1";

    std::vector<uint8_t> vSeed;
    std::string password = "";
    BOOST_CHECK(0 == MnemonicToSeed(words, password, vSeed));

    BOOST_CHECK(HexStr(vSeed.begin(), vSeed.end()) == expect_seed);
}

BOOST_AUTO_TEST_CASE(mnemonic_test_fails)
{
    // Fail tests

    int nLanguage = -1;
    std::string sError;
    std::vector<uint8_t> vEntropy;
    std::string sWords = "legals winner thank year wave sausage worth useful legal winner thank yellow";
    BOOST_CHECK_MESSAGE(3 == MnemonicDecode(nLanguage, sWords, vEntropy, sError), "MnemonicDecode: " << sError);

    sWords = "winner legal thank year wave sausage worth useful legal winner thank yellow";
    BOOST_CHECK_MESSAGE(5 == MnemonicDecode(nLanguage, sWords, vEntropy, sError), "MnemonicDecode: " << sError);
}

BOOST_AUTO_TEST_CASE(mnemonic_addchecksum)
{
    std::string sError;
    std::string sWordsIn = "abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab";
    std::string sWordsOut;

    BOOST_CHECK_MESSAGE(0 == MnemonicAddChecksum(-1, sWordsIn, sWordsOut, sError), "MnemonicAddChecksum: " << sError);

    BOOST_CHECK_MESSAGE(sWordsOut == "abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb", "sWordsOut: " << sWordsOut);

    // Must fail, len % 3 != 0
    std::string sWordsInFail = "abandon baby cabbage dad eager fabric gadget habit ice kangaroo";
    BOOST_CHECK_MESSAGE(4 == MnemonicAddChecksum(-1, sWordsInFail, sWordsOut, sError), "MnemonicAddChecksum: " << sError);


    std::string sWordsInFrench = "zoologie ficeler xénon voyelle village viande vignette sécréter séduire torpille remède";

    BOOST_CHECK(0 == MnemonicAddChecksum(-1, sWordsInFrench, sWordsOut, sError));

    BOOST_CHECK(sWordsOut == "zoologie ficeler xénon voyelle village viande vignette sécréter séduire torpille remède abolir");
}


void runTests(int nLanguage, UniValue &tests)
{
    std::string sError;
    for (unsigned int idx = 0; idx < tests.size(); idx++)
    {
        UniValue test = tests[idx];

        assert(test.size() > 2);

        std::string sEntropy = test[0].get_str();
        std::string sWords = test[1].get_str();
        std::string sSeed;
        std::string sPassphrase;
        if (test.size() > 3)
        {
            sPassphrase = test[2].get_str();
            sSeed = test[3].get_str();
        } else
        {
            sPassphrase = "TREZOR";
            sSeed = test[2].get_str();
        };

        std::vector<uint8_t> vEntropy = ParseHex(sEntropy);
        std::vector<uint8_t> vEntropyTest;

        std::string sWordsTest;
        BOOST_CHECK_MESSAGE(0 == MnemonicEncode(nLanguage, vEntropy, sWordsTest, sError), "MnemonicEncode: " << sError);

        BOOST_CHECK(sWords == sWordsTest);

        int nLanguage = -1;
        BOOST_CHECK_MESSAGE(0 == MnemonicDecode(nLanguage, sWords, vEntropyTest, sError), "MnemonicDecode: " << sError);
        BOOST_CHECK(vEntropy == vEntropyTest);

        std::vector<uint8_t> vSeed = ParseHex(sSeed);
        std::vector<uint8_t> vSeedTest;

        BOOST_CHECK(0 == MnemonicToSeed(sWords, sPassphrase, vSeedTest));
        BOOST_CHECK(vSeed == vSeedTest);

        if (test.size() > 4)
        {
            CExtKey58 eKey58;
            std::string sExtKey = test[4].get_str();

            CExtKey ekTest;
            ekTest.SetSeed(&vSeed[0], vSeed.size());

            eKey58.SetKey(ekTest, CChainParams::EXT_SECRET_KEY_BTC);
            BOOST_CHECK(eKey58.ToString() == sExtKey);
        };
    };
};

BOOST_AUTO_TEST_CASE(mnemonic_test_json)
{
    UniValue tests_english = read_json(
        std::string(json_tests::bip39_vectors_english,
        json_tests::bip39_vectors_english + sizeof(json_tests::bip39_vectors_english)));

    runTests(WLL_ENGLISH, tests_english);

    UniValue tests_japanese = read_json(
        std::string(json_tests::bip39_vectors_japanese,
        json_tests::bip39_vectors_japanese + sizeof(json_tests::bip39_vectors_japanese)));

    runTests(WLL_JAPANESE, tests_japanese);
}

BOOST_AUTO_TEST_SUITE_END()
