// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sstream>
#include <string>

#include "connmgr.h"
#include "dosman.h"
#include "expedited.h"
#include "main.h" // Misbehaving, cs_main


#define NUM_XPEDITED_STORE 10

// Just save the last few expedited sent blocks so we don't resend (uint256)
static uint256 xpeditedBlkSent[NUM_XPEDITED_STORE];

// zeros on construction)
static int xpeditedBlkSendPos = 0;

bool CheckAndRequestExpeditedBlocks(CNode *pfrom)
{
    if (pfrom->nVersion >= EXPEDITED_VERSION)
    {
        BOOST_FOREACH (std::string &strAddr, mapMultiArgs["-expeditedblock"])
        {
            std::string strListeningPeerIP;
            std::string strPeerIP = pfrom->addr.ToString();
            // Add the peer's listening port if it was provided (only misbehaving clients do not provide it)
            if (pfrom->addrFromPort != 0)
            {
                // get addrFromPort
                std::ostringstream ss;
                ss << pfrom->addrFromPort;

                int pos1 = strAddr.rfind(":");
                int pos2 = strAddr.rfind("]:");
                if (pos1 <= 0 && pos2 <= 0)
                    strAddr += ':' + ss.str();

                pos1 = strPeerIP.rfind(":");
                pos2 = strPeerIP.rfind("]:");

                // Handle both ipv4 and ipv6 cases
                if (pos1 <= 0 && pos2 <= 0)
                    strListeningPeerIP = strPeerIP + ':' + ss.str();
                else if (pos1 > 0)
                    strListeningPeerIP = strPeerIP.substr(0, pos1) + ':' + ss.str();
                else
                    strListeningPeerIP = strPeerIP.substr(0, pos2) + ':' + ss.str();
            }
            else
                strListeningPeerIP = strPeerIP;

            if (strAddr == strListeningPeerIP)
                connmgr->PushExpeditedRequest(pfrom, EXPEDITED_BLOCKS);
        }
    }
    return false;
}

bool HandleExpeditedRequest(CDataStream &vRecv, CNode *pfrom)
{
    uint64_t options;
    vRecv >> options;

    if (!pfrom->ThinBlockCapable() || !IsThinBlocksEnabled())
    {
        dosMan.Misbehaving(pfrom, 5);
        return false;
    }

    if (options & EXPEDITED_STOP)
        connmgr->DisableExpeditedSends(pfrom, options & EXPEDITED_BLOCKS, options & EXPEDITED_TXNS);
    else
        connmgr->EnableExpeditedSends(pfrom, options & EXPEDITED_BLOCKS, options & EXPEDITED_TXNS, false);

    return true;
}

bool IsRecentlyExpeditedAndStore(const uint256 &hash)
{
    for (int i = 0; i < NUM_XPEDITED_STORE; i++)
        if (xpeditedBlkSent[i] == hash)
            return true;

    xpeditedBlkSent[xpeditedBlkSendPos] = hash;
    xpeditedBlkSendPos++;
    if (xpeditedBlkSendPos >= NUM_XPEDITED_STORE)
        xpeditedBlkSendPos = 0;

    return false;
}

bool HandleExpeditedBlock(CDataStream &vRecv, CNode *pfrom)
{
    unsigned char hops;
    unsigned char msgType;

    if (!connmgr->IsExpeditedUpstream(pfrom))
        return false;

    vRecv >> msgType >> hops;
    if (msgType == EXPEDITED_MSG_XTHIN)
    {
        return CXThinBlock::HandleMessage(vRecv, pfrom, NetMsgType::XPEDITEDBLK, hops + 1);
    }
    else
    {
        return error(
            "Received unknown (0x%x) expedited message from peer %s hop %d\n", msgType, pfrom->GetLogName(), hops);
    }
}

void SendExpeditedBlock(CXThinBlock &thinBlock, unsigned char hops, const CNode *skip)
{
    VNodeRefs vNodeRefs(connmgr->ExpeditedBlockNodes());

    BOOST_FOREACH (CNodeRef &nodeRef, vNodeRefs)
    {
        CNode *n = nodeRef.get();

        if (n->fDisconnect)
        {
            connmgr->RemovedNode(n);
        }
        else if (n != skip) // Don't send back to the sending node to avoid looping
        {
            LogPrint(
                "thin", "Sending expedited block %s to %s\n", thinBlock.header.GetHash().ToString(), n->GetLogName());

            n->PushMessage(NetMsgType::XPEDITEDBLK, (unsigned char)EXPEDITED_MSG_XTHIN, hops, thinBlock);
            n->blocksSent += 1;
        }
    }
}

void SendExpeditedBlock(const CBlock &block, const CNode *skip)
{
    if (!IsRecentlyExpeditedAndStore(block.GetHash()))
    {
        CXThinBlock thinBlock(block);
        SendExpeditedBlock(thinBlock, 0, skip);
    }
    // else, nothing to do
}
