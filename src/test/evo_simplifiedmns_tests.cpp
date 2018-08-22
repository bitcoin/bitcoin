// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_dash.h"

#include "evo/simplifiedmns.h"
#include "netbase.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_simplifiedmns_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(simplifiedmns_merkleroots)
{
    std::vector<CSimplifiedMNListEntry> entries;
    for (size_t i = 0; i < 15; i++) {
        CSimplifiedMNListEntry smle;
        smle.proRegTxHash.SetHex(strprintf("%064x", i));

        std::string ip = strprintf("%d.%d.%d.%d", 0, 0, 0, i);
        Lookup(ip.c_str(), smle.service, i, false);

        smle.keyIDOperator.SetHex(strprintf("%040x", i));
        smle.keyIDVoting.SetHex(strprintf("%040x", i));
        smle.isValid = true;

        entries.emplace_back(smle);
    }

    std::vector<std::string> expectedHashes = {
        "aa8bfb825f433bcd6f1039f27c77ed269386e05577b0fe9afc4e16b1af0076b2",
        "686a19dba9b515f77f11027cd1e92e6a8c650448bf4616101fd5ddbe6e2629e7",
        "c2efc1b08daa791c71e1d5887be3eaa136381f783fcc5b7efdc5909db38701bb",
        "ce394197d6e1684467fbf2e1619f71ae9d1a6cf6548b2235e4289f95d4bccbbd",
        "aeeaf7b498aa7d5fa92ee0028499b4f165c31662f5e9b0a80e6e13b38fd61f8d",
        "0c1c8dc9dc82eb5432a557580e5d3d930943ce0d0db5daebc51267afb46b6d48",
        "1c4add10ea844a46734473e48c2f781059b35382219d0cf67d6432b540e0bbbe",
        "1ae1ad5ff4dd4c09469d21d569a025d467dca1e407581a2815175528e139b7da",
        "d59b231cdc80ce7eda3a3f37608abda818659c189d31a7ef42024d496e290cbc",
        "2d5e6c87e3d4e5b3fdd600f561e8dec1ea720560569398006050480232f1257c",
        "3d6af35f08efeea22f3c8fcb78038e56dac221f3173ca4e2230ea8ae3cbd3c60",
        "ecf547077c37b79da954c4ef46a3c4fb136746366bfb81192ed01de96fd66348",
        "626af5fb8192ead7bbd79ad7bfe2c3ea82714fdfd9ac49b88d7a411aa6956853",
        "6c84a4485fb2ba35b4dcd4d89cbdd3d813446514bb7a2046b6b1b9813beaac0f",
        "453ca2a83140da73a37794fe6fddd701ea5066f21c2f1df8a33b6ff6134043c3",
    };
    std::vector<std::string> calculatedHashes;

    for (auto& smle : entries) {
        calculatedHashes.emplace_back(smle.CalcHash().ToString());
        //printf("\"%s\",\n", calculatedHashes.back().c_str());
    }

    BOOST_CHECK(expectedHashes == calculatedHashes);

    CSimplifiedMNList sml(entries);

    std::string expectedMerkleRoot = "926efc8dc7b5b060254b102670b918133fea67c5e1bc2703d596e49672878c22";
    std::string calculatedMerkleRoot = sml.CalcMerkleRoot(nullptr).ToString();

    BOOST_CHECK(expectedMerkleRoot == calculatedMerkleRoot);
}
BOOST_AUTO_TEST_SUITE_END()
