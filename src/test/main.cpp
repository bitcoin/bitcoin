// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * See https://www.boost.org/doc/libs/1_71_0/libs/test/doc/html/boost_test/utf_reference/link_references/link_boost_test_module_macro.html
 */
#define BOOST_TEST_MODULE Bitcoin Core Test Suite

#include <boost/test/unit_test.hpp>

#include <test/util/setup_common.h>

/** Redirect debug log to boost log */
const std::function<void(const std::string&)> G_TEST_LOG_FUN = [](const std::string& s) {
    if (s.back() == '\n') {
        // boost will insert the new line
        BOOST_TEST_MESSAGE(s.substr(0, s.size() - 1));
    } else {
        BOOST_TEST_MESSAGE(s);
    }
};
