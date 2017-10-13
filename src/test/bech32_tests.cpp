// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_particl.h"

#include "base58.h"

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
}

BOOST_AUTO_TEST_SUITE_END()

