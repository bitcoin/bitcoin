// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers

#ifndef BITCOIN_CONNMGR_H
#define BITCOIN_CONNMGR_H

#include <atomic>
#include <vector>

#include "net.h"

class CConnMgr
{
    // This critical section protects the vectors
    CCriticalSection cs_expedited;
    // We send expedited blocks to these nodes
    std::vector<CNode *> vSendExpeditedBlocks;
    // We send expedited txs to these nodes
    std::vector<CNode *> vSendExpeditedTxs;
    // We requested expedited blocks from these nodes
    std::vector<CNode *> vExpeditedUpstream;

    // Maximum number of nodes to send expedited blocks and txs to.
    uint32_t nExpeditedBlocksMax;
    uint32_t nExpeditedTxsMax;

    std::atomic<NodeId> next;

public:
    CConnMgr();

    /**
     * Call once the command line is parsed so the connection manager configures itself appropriately.
     */
    void HandleCommandLine();

    /**
     * Return and consume the next node identifier.  Guaranteed positive and monotonically increasing.
     * @return The node ID.
     */
    NodeId NextNodeId();

    /**
     * Given a node ID, return a node reference to the node.
     * @param[in] id     The node ID
     * @return A CNodeRef.  Will be a null CNodeRef if the node was not found.
     */
    CNodeRef FindNodeFromId(NodeId id);

    /**
     * Enable expedited sends to a node.  Ignored if already enabled.
     * @param[in] pNode         The node
     * @param[in] fBlocks       True to enable expedited block sends
     * @param[in] fTxs          True to enable expedited tx sends
     * @param[in] fForceIfFull  True to add the node even if the expedited send list is at its maximum
     */
    void EnableExpeditedSends(CNode *pNode, bool fBlocks, bool fTxs, bool fForceIfFull);

    /**
     * Disable expedited sends to a node.  Ignored if not enabled.
     * @param[in] pNode         The node
     * @param[in] fBlocks       True to disable expedited block sends
     * @param[in] fTxs          True to disnable expedited tx sends
     */
    void DisableExpeditedSends(CNode *pNode, bool fBlocks, bool fTxs);

    /**
     * Return expedited node counts.
     * @param[out] nBlocks       Number of nodes to which we send expedited blocks to
     * @param[out] nTxs          Number of nodes to which we send expedited txs to
     * @param[out] nUpstream     Number of nodes we requested expedited blocks from
     */
    void ExpeditedNodeCounts(uint32_t &nBlocks, uint32_t &nTxs, uint32_t &nUpstream);

    /**
     * Return a vector of node refs to which we send expedited blocks.  A lock is held only briefly.
     * @return A vector of node references.
     */
    VNodeRefs ExpeditedBlockNodes();

    /**
     * Must be called after a node is removed from the vNodes list.
     * @param[in] pNode         The node
     */
    void RemovedNode(CNode *pNode);

    /**
     * Call to request a node to send, or stop sending, expedited blocks or transactions to us.
     * @param[in] pNode         The node
     * @param[in] flags         EXPEDITED_STOP, EXPEDITED_BLOCKS, EXPEDITED_TXS
     * @return True if the request was sent.  False if thin blocks are disabled for us or the peer.
     */
    bool PushExpeditedRequest(CNode *pNode, uint64_t flags);

    /**
     * @param[in] pNode         The node
     * @return True if we have requested expedited blocks from the node.
     */
    bool IsExpeditedUpstream(CNode *pNode);
};

extern std::unique_ptr<CConnMgr> connmgr;

#endif
