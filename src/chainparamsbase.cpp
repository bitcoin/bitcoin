// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";

std::string GetChainName(const Chain chain)
{
    switch (chain) {
    case Chain::MAIN:    return "main";
    case Chain::TESTNET: return "test";
    case Chain::REGTEST: return "regtest";
    default: throw std::runtime_error(strprintf("%s: Unknown chain %d.", __func__, static_cast<int>(chain)));
    }
}

std::string GetDataDir(const Chain chain)
{
    switch (chain) {
    case Chain::MAIN:    return "";
    case Chain::TESTNET: return "testnet3";
    case Chain::REGTEST: return "regtest";
    default: throw std::runtime_error(strprintf("%s: Unknown chain %d.", __func__, static_cast<int>(chain)));
    }
}

int GetRPCPort(const Chain chain)
{
    switch (chain) {
    case Chain::MAIN:    return 8332;
    case Chain::TESTNET: return 18332;
    case Chain::REGTEST: return 18443;
    default: throw std::runtime_error(strprintf("%s: Unknown chain %d.", __func__, static_cast<int>(chain)));
    }
}
