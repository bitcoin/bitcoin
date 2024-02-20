// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/utils.h>
#include <evo/deterministicmns.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif
#include <masternode/sync.h>
#include <net.h>
#include <shutdown.h>
#include <validation.h>
#include <util/ranges.h>
#include <coinjoin/context.h>

void CMasternodeUtils::DoMaintenance(CConnman& connman, const CMasternodeSync& mn_sync, const CJContext& cj_ctx)
{
    if (!mn_sync.IsBlockchainSynced()) return;
    if (ShutdownRequested()) return;

    std::vector<CDeterministicMNCPtr> vecDmns; // will be empty when no wallet
#ifdef ENABLE_WALLET
    for (auto& pair : cj_ctx.walletman->raw()) {
        pair.second->GetMixingMasternodesInfo(vecDmns);
    }
#endif // ENABLE_WALLET

    // Don't disconnect masternode connections when we have less then the desired amount of outbound nodes
    int nonMasternodeCount = 0;
    connman.ForEachNode(CConnman::AllNodes, [&](const CNode* pnode) {
        if ((!pnode->IsInboundConn() &&
            !pnode->IsFeelerConn() &&
            !pnode->IsManualConn() &&
            !pnode->m_masternode_connection &&
            !pnode->m_masternode_probe_connection)
            ||
            // treat unverified MNs as non-MNs here
            pnode->GetVerifiedProRegTxHash().IsNull()) {
            nonMasternodeCount++;
        }
    });
    if (nonMasternodeCount < int(connman.GetMaxOutboundNodeCount())) {
        return;
    }

    connman.ForEachNode(CConnman::AllNodes, [&](CNode* pnode) {
        if (pnode->m_masternode_probe_connection) {
            // we're not disconnecting masternode probes for at least PROBE_WAIT_INTERVAL seconds
            if (GetSystemTimeInSeconds() - pnode->nTimeConnected < PROBE_WAIT_INTERVAL) return;
        } else {
            // we're only disconnecting m_masternode_connection connections
            if (!pnode->m_masternode_connection) return;
            if (!pnode->GetVerifiedProRegTxHash().IsNull()) {
                // keep _verified_ LLMQ connections
                if (connman.IsMasternodeQuorumNode(pnode)) {
                    return;
                }
                // keep _verified_ LLMQ relay connections
                if (connman.IsMasternodeQuorumRelayMember(pnode->GetVerifiedProRegTxHash())) {
                    return;
                }
                // keep _verified_ inbound connections
                if (pnode->IsInboundConn()) {
                    return;
                }
            } else if (GetSystemTimeInSeconds() - pnode->nTimeConnected < 5) {
                // non-verified, give it some time to verify itself
                return;
            } else if (pnode->qwatch) {
                // keep watching nodes
                return;
            }
        }

#ifdef ENABLE_WALLET
        bool fFound = ranges::any_of(vecDmns, [&pnode](const auto& dmn){ return pnode->addr == dmn->pdmnState->addr; });
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
