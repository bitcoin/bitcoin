// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/chainname.h>

#include <chainparamsbase.h>

#include <string>

const std::string chainname::MAIN{CBaseChainParams::MAIN};
const std::string chainname::TESTNET{CBaseChainParams::TESTNET};
const std::string chainname::SIGNET{CBaseChainParams::SIGNET};
const std::string chainname::REGTEST{CBaseChainParams::REGTEST};
