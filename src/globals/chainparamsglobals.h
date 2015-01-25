// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GLOBALS_CHAINPARAMS_H
#define BITCOIN_GLOBALS_CHAINPARAMS_H

#include "templates.hpp"

#include <string>

class CChainParams;

extern Container<CChainParams> cGlobalChainParams;

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams& Params();

/**
 * @returns CChainParams for the given BIP70 chain name.
 */
const CChainParams& Params(const std::string& chain);

/**
 * Sets the params returned by Params() to those for the given BIP70 chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);

#endif // BITCOIN_GLOBALS_CHAINPARAMS_H
