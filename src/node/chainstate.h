// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <cstdint> // for int64_t

class ArgsManager;
struct bilingual_str;
class CChainParams;
class ChainstateManager;
struct NodeContext;

/** This sequence can have 4 types of outcomes:
 *
 *  1. Success
 *  2. Shutdown requested
 *    - nothing failed but a shutdown was triggered in the middle of the
 *      sequence
 *  3. Soft failure
 *    - a failure that might be recovered from with a reindex
 *  4. Hard failure
 *    - a failure that definitively cannot be recovered from with a reindex
 *
 *  Currently, LoadChainstate returns a bool which:
 *      - if false
 *          - Definitely a "Hard failure"
 *      - if true
 *          - if fLoaded -> "Success"
 *          - if ShutdownRequested() -> "Shutdown requested"
 *          - else -> "Soft failure"
 */
bool LoadChainstate(bool& fLoaded,
                    bilingual_str& strLoadError,
                    bool fReset,
                    ChainstateManager& chainman,
                    NodeContext& node,
                    bool fPruneMode,
                    const CChainParams& chainparams,
                    const ArgsManager& args,
                    bool fReindexChainState,
                    int64_t nBlockTreeDBCache,
                    int64_t nCoinDBCache,
                    int64_t nCoinCacheUsage);

#endif // BITCOIN_NODE_CHAINSTATE_H
