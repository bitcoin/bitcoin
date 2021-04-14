// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/masternodeutils.h>

#include <init.h>
#include <masternode/masternodesync.h>
#include <validation.h>
#include <shutdown.h>
struct CompareScoreMN
{
    bool operator()(const std::pair<arith_uint256, const CDeterministicMNCPtr&>& t1,
                    const std::pair<arith_uint256, const CDeterministicMNCPtr&>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->collateralOutpoint < t2.second->collateralOutpoint);
    }
};

void CMasternodeUtils::ProcessMasternodeConnections(CConnman& connman)
{
    // Don't disconnect masternode connections when we have less then the desired amount of outbound nodes
    size_t nonMasternodeCount = 0;
    connman.ForEachNode(AllNodes, [&](CNode* pnode) {
        if (!pnode->IsInboundConn() && !pnode->IsFeelerConn() && !pnode->IsManualConn() && !pnode->m_masternode_connection && !pnode->m_masternode_probe_connection) {
            nonMasternodeCount++;
        }
    });
    if (nonMasternodeCount < connman.GetMaxOutboundNodeCount()) {
        return;
    }

    connman.ForEachNode(AllNodes, [&](CNode* pnode) {
        // we're only disconnecting m_masternode_connection connections
        if (!pnode->m_masternode_connection) return;
        // we're only disconnecting outbound connections
        if (pnode->IsInboundConn()) return;
        // we're not disconnecting LLMQ connections
        if (connman.IsMasternodeQuorumNode(pnode)) return;
        // we're not disconnecting masternode probes for at least a few seconds
        if (pnode->m_masternode_probe_connection && GetSystemTimeInSeconds() - pnode->nTimeConnected < 5) return;

        if (fLogIPs) {
            LogPrint(BCLog::NET, "Closing Masternode connection: peer=%d, addr=%s\n", pnode->GetId(), pnode->addr.ToString());
        } else {
            LogPrint(BCLog::NET, "Closing Masternode connection: peer=%d\n", pnode->GetId());
        }
        pnode->fDisconnect = true;
    });
}

void CMasternodeUtils::DoMaintenance(CConnman& connman)
{
    if(!masternodeSync.IsBlockchainSynced() || ShutdownRequested())
        return;

    static unsigned int nTick = 0;

    nTick++;

    if(nTick % 60 == 0) {
        ProcessMasternodeConnections(connman);
    }
}

