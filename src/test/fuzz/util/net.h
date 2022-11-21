// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_NET_H
#define BITCOIN_TEST_FUZZ_UTIL_NET_H

#include <netaddress.h>

class FuzzedDataProvider;

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept;

#endif // BITCOIN_TEST_FUZZ_UTIL_NET_H
