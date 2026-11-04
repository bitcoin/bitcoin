// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_STALETIPRELAYMAN_H
#define BITCOIN_NODE_STALETIPRELAYMAN_H

#include <kernel/cs_main.h>
#include <node/staletiptracker.h>
#include <staletipmessage.h>
#include <sync.h>
#include <util/chaintype.h>

#include <vector>

class CBlockIndex;
class CChain;

namespace node {
class BlockManager;

struct StaleTipRelayOptions {
    ChainType chain_type{ChainType::MAIN};
};

/** Build the `staletip` message announcing `fork`, compressing the headers
 *  between `fork.fork_point` (exclusive) and `fork.tip` (inclusive). `fork`
 *  must be one the tracker returned, so it fits in one message. */
StaleTipMessage MakeStaleTipMessage(const StaleFork& fork, bool have_block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

/**
 * Class responsible for deciding which stale tips to track for BIP 332
 * stale-tip relay.
 *
 * This class is not thread-safe. Callers must hold cs_main.
 */
class StaleTipRelayManager
{
public:
    explicit StaleTipRelayManager(const StaleTipRelayOptions& opts);

    /** Seed the tracker with the eligible stale tips in the block index. */
    void Initialize(const BlockManager& blockman, const CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    // Responses to chain events. StaleTipRelayManager is not an actual client
    // of ValidationInterface, these are called through PeerManager.
    void BlockDisconnected(const CChain& chain, const CBlockIndex& block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void AcceptedStaleTip(const CChain& chain, const CBlockIndex& tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the tracked tips that are eligible for relay, with their fork
     *  points, in the order they were added. */
    std::vector<StaleFork> GetStaleTips(const CChain& chain) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Whether to ask peers to announce stale tips once they can serve the
     *  blocks. Always on signet, where a tip is only tracked once its block
     *  is validated. */
    bool PrefersBlocks() const;

private:
    const StaleTipRelayOptions m_opts;
    StaleTipTracker m_tracker GUARDED_BY(::cs_main);
};

} // namespace node

#endif // BITCOIN_NODE_STALETIPRELAYMAN_H
