// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TESTS_H
#define BITCOIN_TEST_UTIL_TESTS_H

#include <fs.h>

namespace util_tests {
    fs::path unit_test_directory(const std::string& fileName);
}; //namespace util_tests

#endif // BITCOIN_TEST_UTIL_TESTS_H