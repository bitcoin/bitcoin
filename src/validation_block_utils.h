// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_BLOCK_UTILS_H
#define BITCOIN_VALIDATION_BLOCK_UTILS_H

#include <validation_globals.h>
#include <chain.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

inline CBlockIndex* LookupBlockIndex(const uint256& hash)
{
    AssertLockHeld(cs_main);
    BlockMap::const_iterator it = mapBlockIndex.find(hash);
    return it == mapBlockIndex.end() ? nullptr : it->second;
}

/** Find the last common block between the parameter chain and a locator. */
CBlockIndex* FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator);

#endif // BITCOIN_VALIDATION_BLOCK_UTILS_H
