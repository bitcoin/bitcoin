// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory_global.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blsct_txfactory_global_tests)

BOOST_FIXTURE_TEST_CASE(create_output_test, TestingSetup)
{
    std::string destAddr = "xNTWKN8kaGR5tR8qbyMLrqkHXM8Esui3PbMBbL95EtoVsiLWTsU72E3tayw8CF81AJbSgRyV1GqyAaPnYJabZgfQYu1A11kqFhqicGTgv9J3Yk8BXSpxZmTXPRuQnyZMMHZrUxhczke ";
    MclScalar blindingKey{ParseHex("42c0926471b3bd01ae130d9382c5fca2e2b5000abbf826a93132696ffa5f2c65")};

    auto out = blsct::CreateOutput(destAddr, 1, "", TokenId(), blindingKey);

    BOOST_CHECK(out.out.blsctData.viewTag == 20232);
    BOOST_CHECK(HexStr(out.out.blsctData.spendingKey.GetVch()) == "84ee3e9c40fe65f91776033b5ddb3bf280bbd549924028280dad0d6ea464bb728886910f53bd66bcc2b59e37f1b8f55e");
    BOOST_CHECK(HexStr(out.out.blsctData.blindingKey.GetVch()) == "80e845ca79807a66ab93121dd57971ee2acbb3ef20d94c0750eb33eaa64bf58fe0f5cd775dcf73b49117260e806e8708");
    BOOST_CHECK(HexStr(out.out.blsctData.ephemeralKey.GetVch()) == "935963399885ba1dd51dd272fb9be541896ac619570315e55f06c1e3a42d28ffb300fe6a3247d484bb491b25ecf7fb8a");
    BOOST_CHECK(out.gamma.GetString() == "77fb7a5ff31f1c28037942b94e23270338fa788e065855dffa6860be93073ee");
    BOOST_CHECK(out.blindingKey.GetString() == "42c0926471b3bd01ae130d9382c5fca2e2b5000abbf826a93132696ffa5f2c65");
}

BOOST_AUTO_TEST_SUITE_END()