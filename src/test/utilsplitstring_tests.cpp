// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/splitstring.h>
#include <util/strencodings.h>

#include <string>
#include <vector>

#include <test/setup_common.h>
#include <boost/test/unit_test.hpp>

//
// To validate the implementation of Split against `boost::split`:
//
// [1] Comment out `#include <util/splitstring.h>` above and use the includes below:
//
// #include <boost/algorithm/string/classification.hpp>
// #include <boost/algorithm/string/split.hpp>
//
// [2] Replace the Split implementation in `util/splitstring.h` with the implementation below:
//
// template<typename ContainerT>
// void Split(ContainerT& tokens, const std::string& str, const std::function<bool(char)>& predicate, bool merge_empty = false)
// {
//     boost::split(tokens, str, predicate, merge_empty ? boost::token_compress_on : boost::token_compress_off);
// }
//
// The unit tests should yield the same result.
//

BOOST_FIXTURE_TEST_SUITE(utilsplitstring_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(urlsplitstring_test)
{
    {
        std::vector<std::string> result;
        Split(result, "", IsAnyOf(","));
        std::vector<std::string> expected = {""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, "", IsAnyOf(","), true);
        std::vector<std::string> expected = {""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",", IsAnyOf(","));
        std::vector<std::string> expected = {"", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",", IsAnyOf(","), true);
        std::vector<std::string> expected = {"", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,", IsAnyOf(","));
        std::vector<std::string> expected = {"", "", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,", IsAnyOf(","), true);
        std::vector<std::string> expected = {"", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,,", IsAnyOf(","));
        std::vector<std::string> expected = {"", "", "", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,,", IsAnyOf(","), true);
        std::vector<std::string> expected = {"", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, "Satoshi,Nakamoto", IsAnyOf(","));
        std::vector<std::string> expected = {"Satoshi", "Nakamoto"};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, "Satoshi,Nakamoto", IsAnyOf(","), true);
        std::vector<std::string> expected = {"Satoshi", "Nakamoto"};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,Satoshi,,,,,,Nakamoto,,", IsAnyOf(","));
        std::vector<std::string> expected = {"", "", "Satoshi", "", "", "", "", "", "Nakamoto", "", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, ",,Satoshi,,,,,,Nakamoto,,", IsAnyOf(","), true);
        std::vector<std::string> expected = {"", "Satoshi", "Nakamoto", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::set<std::string> result;
        Split(result, "#reckless", IsAnyOf(""), false);
        BOOST_CHECK_EQUAL(result.count("#reckless"), 1);
    }

    {
        std::set<std::string> result;
        Split(result, "#reckless", IsAnyOf(""), true);
        BOOST_CHECK_EQUAL(result.count("#reckless"), 1);
    }

    {
        std::set<std::string> result;
        Split(result, "#reckless", IsAnyOf(",#$"), false);
        BOOST_CHECK_EQUAL(result.count(""), 1);
        BOOST_CHECK_EQUAL(result.count("reckless"), 1);
    }

    {
        std::set<std::string> result;
        Split(result, "#reckless|hodl bitcoin ", IsAnyOf(" |"), true);
        BOOST_CHECK_EQUAL(result.count("#reckless"), 1);
        BOOST_CHECK_EQUAL(result.count("hodl"), 1);
        BOOST_CHECK_EQUAL(result.count("bitcoin"), 1);
        BOOST_CHECK_EQUAL(result.count(""), 1);
    }

    {
        std::vector<std::string> result;
        Split(result, "  Satoshi \f\n\r\t\vNakamoto  ", IsSpace);
        std::vector<std::string> expected = {"", "", "Satoshi", "", "", "", "", "", "Nakamoto", "", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

    {
        std::vector<std::string> result;
        Split(result, "  Satoshi \f\n\r\t\vNakamoto  ", IsSpace, true);
        std::vector<std::string> expected = {"", "Satoshi", "Nakamoto", ""};
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }

}

BOOST_AUTO_TEST_SUITE_END()
