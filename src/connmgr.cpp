// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers

#include "connmgr.h"
#include "expedited.h"

std::unique_ptr<CConnMgr> connmgr(new CConnMgr);

/**
 * Find a node in a vector.  Just calls std::find.  Split out for cleanliness and also because std:find won't be
 * enough if we switch to vectors of noderefs.  Requires any lock for vector traversal to be held.
 */
static std::vector<CNode *>::iterator FindNode(std::vector<CNode *> &vNodes, CNode *pNode)
{
    return std::find(vNodes.begin(), vNodes.end(), pNode);
}

/**
 * Add a node to a vector, and take a reference.  This function does NOT prevent adding duplicates.
 * Requires any lock for vector traversal to be held.
 * @param[in] vNodes      The vector of nodes.
 * @param[in] pNode       The node to add.
 */
static void AddNode(std::vector<CNode *> &vNodes, CNode *pNode)
{
    pNode->AddRef();
    vNodes.push_back(pNode);
}

/**
 * If a node is present in a vector, remove it and drop a reference.
 * Requires any lock for vector traversal to be held.
 * @param[in] vNodes      The vector of nodes.
 * @param[in] pNode       The node to remove.
 * @return  True if the node was originally present, false if not.
 */
static bool RemoveNode(std::vector<CNode *> &vNodes, CNode *pNode)
{
    auto Node = FindNode(vNodes, pNode);

    if (Node != vNodes.end())
    {
        pNode->Release();
        vNodes.erase(Node);
        return true;
    }

    return false;
}

CConnMgr::CConnMgr() : nExpeditedBlocksMax(32), nExpeditedTxsMax(32), next(0)
{
    vSendExpeditedBlocks.reserve(256);
    vSendExpeditedTxs.reserve(256);
    vExpeditedUpstream.reserve(256);
}

NodeId CConnMgr::NextNodeId()
{
    // Pre-increment; do not use zero
    return ++next;
}

/**
 * Given a node ID, return a node reference to the node.
 */
CNodeRef CConnMgr::FindNodeFromId(NodeId id)
{
    LOCK(cs_vNodes);

    for (CNode *pNode : vNodes)
    {
        if (pNode->GetId() == id)
            return CNodeRef(pNode);
    }

    return CNodeRef();
}

void CConnMgr::EnableExpeditedSends(CNode *pNode, bool fBlocks, bool fTxs, bool fForceIfFull)
{
    LOCK(cs_expedited);

    if (fBlocks && FindNode(vSendExpeditedBlocks, pNode) == vSendExpeditedBlocks.end())
    {
        if (fForceIfFull || vSendExpeditedBlocks.size() < nExpeditedBlocksMax)
        {
            AddNode(vSendExpeditedBlocks, pNode);
            LogPrint("thin", "Enabled expedited blocks to peer %s (%u peers total)\n", pNode->GetLogName(),
                vSendExpeditedBlocks.size());
        }
        else
        {
            LogPrint("thin", "Cannot enable expedited blocks to peer %s, I am full (%u peers total)\n",
                pNode->GetLogName(), vSendExpeditedBlocks.size());
        }
    }

    if (fTxs && FindNode(vSendExpeditedTxs, pNode) == vSendExpeditedTxs.end())
    {
        if (fForceIfFull || vSendExpeditedTxs.size() < nExpeditedTxsMax)
        {
            AddNode(vSendExpeditedTxs, pNode);
            LogPrint("thin", "Enabled expedited txs to peer %s (%u peers total)\n", pNode->GetLogName(),
                vSendExpeditedTxs.size());
        }
        else
        {
            LogPrint("thin", "Cannot enable expedited txs to peer %s, I am full (%u peers total)\n",
                pNode->GetLogName(), vSendExpeditedTxs.size());
        }
    }
}

void CConnMgr::DisableExpeditedSends(CNode *pNode, bool fBlocks, bool fTxs)
{
    LOCK(cs_expedited);

    if (fBlocks && RemoveNode(vSendExpeditedBlocks, pNode))
        LogPrint("thin", "Disabled expedited blocks to peer %s (%u peers total)\n", pNode->GetLogName(),
            vSendExpeditedBlocks.size());

    if (fTxs && RemoveNode(vSendExpeditedTxs, pNode))
        LogPrint("thin", "Disabled expedited txs to peer %s (%u peers total)\n", pNode->GetLogName(),
            vSendExpeditedTxs.size());
}

void CConnMgr::HandleCommandLine()
{
    nExpeditedBlocksMax = GetArg("-maxexpeditedblockrecipients", nExpeditedBlocksMax);
    nExpeditedTxsMax = GetArg("-maxexpeditedtxrecipients", nExpeditedTxsMax);
}

// Called after a node is removed from the node list.
void CConnMgr::RemovedNode(CNode *pNode)
{
    LOCK(cs_expedited);

    RemoveNode(vSendExpeditedBlocks, pNode);
    RemoveNode(vSendExpeditedTxs, pNode);
    RemoveNode(vExpeditedUpstream, pNode);
}

void CConnMgr::ExpeditedNodeCounts(uint32_t &nBlocks, uint32_t &nTxs, uint32_t &nUpstream)
{
    LOCK(cs_expedited);

    nBlocks = vSendExpeditedBlocks.size();
    nTxs = vSendExpeditedTxs.size();
    nUpstream = vExpeditedUpstream.size();
}

VNodeRefs CConnMgr::ExpeditedBlockNodes()
{
    LOCK(cs_expedited);

    VNodeRefs vRefs;

    BOOST_FOREACH (CNode *pNode, vExpeditedUpstream)
        vRefs.push_back(CNodeRef(pNode));

    return vRefs;
}

bool CConnMgr::PushExpeditedRequest(CNode *pNode, uint64_t flags)
{
    if (!IsThinBlocksEnabled())
        return error("Thinblocks is not enabled so cannot request expedited blocks from peer %s", pNode->GetLogName());

    if (!pNode->ThinBlockCapable())
        return error("Remote peer has not enabled Thinblocks so you cannot request expedited blocks from %s",
            pNode->GetLogName());

    if (flags & EXPEDITED_BLOCKS)
    {
        LOCK(cs_expedited);

        // Add or remove this node as an upstream node
        if (flags & EXPEDITED_STOP)
        {
            RemoveNode(vExpeditedUpstream, pNode);
            LogPrintf("Requesting a stop of expedited blocks from peer %s\n", pNode->GetLogName());
        }
        else
        {
            if (FindNode(vExpeditedUpstream, pNode) == vExpeditedUpstream.end())
                AddNode(vExpeditedUpstream, pNode);
            LogPrintf("Requesting expedited blocks from peer %s\n", pNode->GetLogName());
        }
    }

    // Push even if its a repeat to allow the operator to use the CLI or GUI to force another message.
    pNode->PushMessage(NetMsgType::XPEDITEDREQUEST, flags);

    return true;
}

bool CConnMgr::IsExpeditedUpstream(CNode *pNode)
{
    LOCK(cs_expedited);

    return FindNode(vExpeditedUpstream, pNode) != vExpeditedUpstream.end();
}
