// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

BOOST_AUTO_TEST_SUITE(mock_process, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(valid_json, *boost::unit_test::disabled())
{
    std::cout << R"({"success": true})" << std::endl;
}

BOOST_AUTO_TEST_CASE(nonzeroexit_nooutput, *boost::unit_test::disabled())
{
    BOOST_FAIL("Test unconditionally fails.");
}

BOOST_AUTO_TEST_CASE(nonzeroexit_stderroutput, *boost::unit_test::disabled())
{
    std::cerr << "err\n";
    BOOST_FAIL("Test unconditionally fails.");
}

BOOST_AUTO_TEST_CASE(invalid_json, *boost::unit_test::disabled())
{
    std::cout << "{\n";
}

BOOST_AUTO_TEST_CASE(pass_stdin_to_stdout, *boost::unit_test::disabled())
{
    std::string s;
    std::getline(std::cin, s);
    std::cout << s << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
