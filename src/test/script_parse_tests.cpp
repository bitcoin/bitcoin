// Copyright (c) 2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(script_parse_tests)
BOOST_AUTO_TEST_CASE(parse_script)
{
    const std::vector<std::pair<std::string,std::string>> IN_OUT{
        // {IN: script string , OUT: hex string }
        {"", ""},
        {"0", "00"},
        {"1", "51"},
        {"2", "52"},
        {"3", "53"},
        {"4", "54"},
        {"5", "55"},
        {"6", "56"},
        {"7", "57"},
        {"8", "58"},
        {"9", "59"},
        {"10", "5a"},
        {"11", "5b"},
        {"12", "5c"},
        {"13", "5d"},
        {"14", "5e"},
        {"15", "5f"},
        {"16", "60"},
        {"17", "0111"},
        {"-9", "0189"},
        {"0x17", "17"},
        {"'17'", "023137"},
        {"ELSE", "67"},
        {"NOP10", "b9"},
    };
    std::string all_in;
    std::string all_out;
    for (const auto& [in, out] : IN_OUT) {
        BOOST_CHECK_EQUAL(HexStr(ParseScript(in)), out);
        all_in += " " + in + " ";
        all_out += out;
    }
    BOOST_CHECK_EQUAL(HexStr(ParseScript(all_in)), all_out);

    BOOST_CHECK_EXCEPTION(ParseScript("11111111111111111111"), std::runtime_error, HasReason("script parse error: decimal numeric value only allowed in the range -0xFFFFFFFF...0xFFFFFFFF"));
    BOOST_CHECK_EXCEPTION(ParseScript("11111111111"), std::runtime_error, HasReason("script parse error: decimal numeric value only allowed in the range -0xFFFFFFFF...0xFFFFFFFF"));
    BOOST_CHECK_EXCEPTION(ParseScript("OP_CHECKSIGADD"), std::runtime_error, HasReason("script parse error: unknown opcode"));
}
BOOST_AUTO_TEST_SUITE_END()
