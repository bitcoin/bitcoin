// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/masternode-utils.h>
#include <evo/deterministicmns.h>

#ifdef ENABLE_WALLET
#include <coinjoin/coinjoin-client.h>
#endif
#include <masternode/masternode-sync.h>
#include <net.h>
#include <shutdown.h>
#include <validation.h>


void CMasternodeUtils::ProcessMasternodeConnections(CConnman& connman)
{
    std::vector<CDeterministicMNCPtr> vecDmns; // will be empty when no wallet
#ifdef ENABLE_WALLET
    for (const auto& pair : coinJoinClientManagers) {
        pair.second->GetMixingMasternodesInfo(vecDmns);
    }
#endif // ENABLE_WALLET

    // Don't disconnect masternode connections when we have less then the desired amount of outbound nodes
    int nonMasternodeCount = 0;
    connman.ForEachNode(CConnman::AllNodes, [&](CNode* pnode) {
        if (!pnode->fInbound && !pnode->fFeeler && !pnode->m_manual_connection && !pnode->m_masternode_connection && !pnode->m_masternode_probe_connection) {
            nonMasternodeCount++;
        }
    });
    if (nonMasternodeCount < connman.GetMaxOutboundNodeCount()) {
        return;
    }

    connman.ForEachNode(CConnman::AllNodes, [&](CNode* pnode) {
        // we're only disconnecting m_masternode_connection connections
        if (!pnode->m_masternode_connection) return;
        // we're only disconnecting outbound connections
        if (pnode->fInbound) return;
        // we're not disconnecting LLMQ connections
        if (connman.IsMasternodeQuorumNode(pnode)) return;
        // we're not disconnecting masternode probes for at least a few seconds
        if (pnode->m_masternode_probe_connection && GetSystemTimeInSeconds() - pnode->nTimeConnected < 5) return;

#ifdef ENABLE_WALLET
        bool fFound = false;
        for (const auto& dmn : vecDmns) {
            if (pnode->addr == dmn->pdmnState->addr) {
                fFound = true;
                break;
            }
        }
        if (fFound) return; // do NOT disconnect mixing masternodes
#endif // ENABLE_WALLET
        if (fLogIPs) {
            LogPrint(BCLog::NET_NETCONN, "Closing Masternode connection: peer=%d, addr=%s\n", pnode->GetId(), pnode->addr.ToString());
        } else {
            LogPrint(BCLog::NET_NETCONN, "Closing Masternode connection: peer=%d\n", pnode->GetId());
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

