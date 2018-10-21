// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_dash.h"

#include "bls/bls.h"
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

        uint8_t skBuf[CBLSSecretKey::SerSize];
        memset(skBuf, 0, sizeof(skBuf));
        skBuf[0] = (uint8_t)i;
        CBLSSecretKey sk;
        sk.SetBuf(skBuf, sizeof(skBuf));

        smle.pubKeyOperator = sk.GetPublicKey();
        smle.keyIDVoting.SetHex(strprintf("%040x", i));
        smle.isValid = true;

        entries.emplace_back(smle);
    }

    std::vector<std::string> expectedHashes = {
        "1465924a81df1d0fb46d2571e65296c0fddab30d1b4d104f182f80a6e45362fa",
        "cb466084403067b699a50bd46e53145ebfd57af9a076a13432f524e41ec2d899",
        "0f6a720d0119f5b83469d884ec2a7d739c1d653142e5d36b569cddaf735a6149",
        "8794379559c77d67902a5e894739bd574f25ff6cd48612f7156c5fafaefefd4e",
        "4738d4f17c76b4e61f028d9426f40242eb56977fe805704d980ff57ad3f60b90",
        "6342b7ce87a299a0fb070ec040a45613f082b0c99395698a6f712e85be87ad06",
        "0be7143f4fb357333350a7e28606713933e9819c83c71009b9ea97476d86b78e",
        "d13dfedc920490a8c6d9b555d740f2944ac83eeb8c3923a51261371a8118ffe0",
        "c0c4638fcefe09380adff61d59c064be03a18690245b89be20223df737590d46",
        "4cce41032bb341adb9b9f3f6ba1de3c812f0d1630e3a561c7aae00f39e49c6d4",
        "3e8c9ad9e2cf0520a96b6bc58aafb7009365a1d8f357047a40430b782142ed69",
        "bde7a1b61a263a7e0dfe474c850c4f9d642d9a03da501a366800e03093e48cd9",
        "e221a3e14868251083eea718e698cc81739b016665a9024d798a14c0e9417c35",
        "b1be038e40bc8ee6a19dbbe70c92d7bc0880ccc54c4bd54eab6b7a30d0650ab1",
        "0607e1d850e27c336e8d65722fc82eae7ab5331fd88a4fbfcff6c3a1bb6364da",
    };
    std::vector<std::string> calculatedHashes;

    for (auto& smle : entries) {
        calculatedHashes.emplace_back(smle.CalcHash().ToString());
        //printf("\"%s\",\n", calculatedHashes.back().c_str());
    }

    BOOST_CHECK(expectedHashes == calculatedHashes);

    CSimplifiedMNList sml(entries);

    std::string expectedMerkleRoot = "a7fae13820022ddba6cf54ba9b234dfed54ccfa86c2635c5627287f1ffa5497f";
    std::string calculatedMerkleRoot = sml.CalcMerkleRoot(nullptr).ToString();
    //printf("merkleRoot=\"%s\",\n", calculatedMerkleRoot.c_str());

    BOOST_CHECK(expectedMerkleRoot == calculatedMerkleRoot);
}
BOOST_AUTO_TEST_SUITE_END()
