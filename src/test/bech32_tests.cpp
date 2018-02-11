// Copyright (c) 2017 Pieter Wuille
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <bech32.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(bech32_tests, BasicTestingSetup)

std::vector<std::pair<std::string, std::string> > testsPass = {
    std::make_pair("PZdYWHgyhuG7NHVCzEkkx3dcLKurTpvmo6", "ph1z2kuclaye2ktkndy7mdpw3zk0nck78a7u6h8hm"),
    std::make_pair("RJAPhgckEgRGVPZa9WoGSWW24spskSfLTQ", "pr1v9h7c4k0e3axk7jlejzyh8tnc5eryx6e4ys8vz"),
    std::make_pair("SPGxiYZ1Q5dhAJxJNMk56ZbxcsUBYqTCsdEPPHsJJ96Vcns889gHTqSrTZoyrCd5E9NSe9XxLivK6izETniNp1Gu1DtrhVwv3VuZ3e", "ps1qqpvvvphd2zkphxxckzef2lgag67gzpz85alcemkxzvpl5tkgc8p34qpqwg58jx532dpkk0qtysnyudzg4ajk0vvqp8w9zlgjamxxz2l9cpt7qqqflt7zu"),
    std::make_pair("PPARTKMMf4AUDYzRSBcXSJZALbUXgWKHi6qdpy95yBmABuznU3keHFyNHfjaMT33ehuYwjx3RXort1j8d9AYnqyhAvdN168J4GBsM2ZHuTb91rsj", "pep1qqqqqqqqqqqqqqqcpjvcv9trdnv8t2nscuw056mm74cps7jkmrrdq48xpdjy6ylf6vpru36q6zax883gjclngas40d7097mudl05y48ewzvulpnsk5z75kg24d5nf"),
};

std::vector<std::string> testsFail = {
    "ph1z2kuclaye2kthndy7mdpw3zk0nck78a7u6h8hm",
    "prIv9h7c4k0e3axk7jlejzyh8tnc5eryx6e4ys8vz",
    "ps1qqpvvvphd2zkphxxckzef2lgag67gzpz85alcemkxzvpl5tkgc7p34qpqwg58jx532dpkk0qtysnyudzg4ajk0vvqp8w9zlgjamxxz2l9cpt7qqqflt7zu",
    "pep1qqqqqqqqqqqqqqcpjvcv9trdnv8t2nscuw056mm74cps7jkmrrdq48xpdjy6ylf6vpru36q6zax883gjclngas40d7097mudl05y48ewzvulpnsk5z75kg24d5nf",
};

std::vector<std::pair<CChainParams::Base58Type, std::string>> testsType = {
    std::make_pair(CChainParams::PUBKEY_ADDRESS, "ph1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqx7sxra"),
    std::make_pair(CChainParams::SCRIPT_ADDRESS, "pr1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqwp6w94"),
    std::make_pair(CChainParams::PUBKEY_ADDRESS_256, "pl1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqra7r7c"),
    std::make_pair(CChainParams::SCRIPT_ADDRESS_256, "pj1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqsthset"),
    std::make_pair(CChainParams::SECRET_KEY, "px1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqjjpjvf"),
    std::make_pair(CChainParams::EXT_PUBLIC_KEY, "pep1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq5cjrsh"),
    std::make_pair(CChainParams::EXT_SECRET_KEY, "pex1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq77wfra"),
    std::make_pair(CChainParams::STEALTH_ADDRESS, "ps1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9ld9g7"),
    std::make_pair(CChainParams::EXT_KEY_HASH, "pek1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqhe0qm5"),
    std::make_pair(CChainParams::EXT_ACC_HASH, "pea1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqt25ujg"),
};


BOOST_AUTO_TEST_CASE(bech32_test)
{
    CBitcoinAddress addr_base58;
    CBitcoinAddress addr_bech32;

    for (auto &v : testsPass)
    {
        addr_base58.SetString(v.first);
        CTxDestination dest = addr_base58.Get();
        BOOST_CHECK(addr_bech32.Set(dest, true));
        BOOST_CHECK(addr_bech32.ToString() == v.second);

        CTxDestination dest2 = addr_bech32.Get();
        BOOST_CHECK(dest == dest2);
        CBitcoinAddress t58_back(dest2);
        BOOST_CHECK(t58_back.ToString() == v.first);

        CBitcoinAddress addr_bech32_2(v.second);
        BOOST_CHECK(addr_bech32_2.IsValid());
        CTxDestination dest3 = addr_bech32_2.Get();
        BOOST_CHECK(dest == dest3);
    };

    for (auto &v : testsFail)
    {
        BOOST_CHECK(!addr_bech32.SetString(v));
    };

    CKeyID knull;
    for (auto &v : testsType)
    {
        CBitcoinAddress addr;
        addr.Set(knull, v.first, true);
        BOOST_CHECK(addr.ToString() == v.second);
    };
}

bool CaseInsensitiveEqual(const std::string &s1, const std::string &s2)
{
    if (s1.size() != s2.size()) return false;
    for (size_t i = 0; i < s1.size(); ++i) {
        char c1 = s1[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 -= ('A' - 'a');
        char c2 = s2[i];
        if (c2 >= 'A' && c2 <= 'Z') c2 -= ('A' - 'a');
        if (c1 != c2) return false;
    }
    return true;
}

BOOST_AUTO_TEST_CASE(bip173_testvectors_valid)
{
    static const std::string CASES[] = {
        "A12UEL5L",
        "a12uel5l",
        "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs",
        "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
        "11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j",
        "split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w",
        "?1ezyfcl",
    };
    for (const std::string& str : CASES) {
        auto ret = bech32::Decode(str);
        BOOST_CHECK(!ret.first.empty());
        std::string recode = bech32::Encode(ret.first, ret.second);
        BOOST_CHECK(!recode.empty());
        BOOST_CHECK(CaseInsensitiveEqual(str, recode));
    }
}

BOOST_AUTO_TEST_CASE(bip173_testvectors_invalid)
{
    static const std::string CASES[] = {
        " 1nwldj5",
        "\x7f""1axkwrx",
        "\x80""1eym55h",
        "an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx",
        "pzry9x0s0muk",
        "1pzry9x0s0muk",
        "x1b4n0q5v",
        "li1dgmt3",
        "de1lg7wt\xff",
        "A1G7SGD8",
        "10a06t8",
        "1qzzfhee",
    };
    for (const std::string& str : CASES) {
        auto ret = bech32::Decode(str);
        BOOST_CHECK(ret.first.empty());
    }
}

BOOST_AUTO_TEST_SUITE_END()
