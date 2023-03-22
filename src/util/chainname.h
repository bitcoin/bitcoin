// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHAINNAME_H
#define BITCOIN_UTIL_CHAINNAME_H

#include <string>

namespace chainname {
extern const std::string MAIN;
extern const std::string TESTNET;
extern const std::string SIGNET;
extern const std::string REGTEST;
} // namespace chainname

#endif // BITCOIN_UTIL_CHAINNAME_H
