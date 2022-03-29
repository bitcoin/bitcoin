// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/bip32.h>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string keypath_str(buffer.begin(), buffer.end());
    std::vector<uint32_t> keypath;
    (void)ParseHDKeypath(keypath_str, keypath);
}
