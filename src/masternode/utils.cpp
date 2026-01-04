// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/utils.h>

#include <net.h>
#include <shutdown.h>
#include <util/ranges.h>

#include <coinjoin/walletman.h>
#include <evo/deterministicmns.h>
#include <masternode/sync.h>

void CMasternodeUtils::DoMaintenance(CConnman& connman, CDeterministicMNManager& dmnman, const CMasternodeSync& mn_sync,
                                     CJWalletManager* const cj_walletman)
{
    if (!mn_sync.IsBlockchainSynced()) return;
    if (ShutdownRequested()) return;

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

    auto mixing_masternodes = cj_walletman ? cj_walletman->getMixingMasternodes() : std::vector<CDeterministicMNCPtr>{};
    connman.ForEachNode(CConnman::AllNodes, [&](CNode* pnode) {
        if (pnode->m_masternode_probe_connection) {
            // we're not disconnecting masternode probes for at least PROBE_WAIT_INTERVAL seconds
            if (GetTime<std::chrono::seconds>() - pnode->m_connected < PROBE_WAIT_INTERVAL) return;
        } else {
            // we're only disconnecting m_masternode_connection connections
            if (!pnode->m_masternode_connection) return;
            if (!pnode->GetVerifiedProRegTxHash().IsNull()) {
                const auto tip_mn_list = dmnman.GetListAtChainTip();
                // keep _verified_ LLMQ connections
                if (connman.IsMasternodeQuorumNode(pnode, tip_mn_list)) {
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
            } else if (GetTime<std::chrono::seconds>() - pnode->m_connected < PROBE_WAIT_INTERVAL) {
                // non-verified, give it some time to verify itself
                return;
            } else if (pnode->qwatch) {
                // keep watching nodes
                return;
            }
        }

        bool fFound = ranges::any_of(mixing_masternodes, [&pnode](const auto& dmn) {
            return pnode->addr == dmn->pdmnState->netInfo->GetPrimary();
        });
        if (fFound) return; // do NOT disconnect mixing masternodes
        if (fLogIPs) {
            LogPrint(BCLog::NET_NETCONN, "Closing Masternode connection: peer=%d, addr=%s\n", pnode->GetId(),
                     pnode->addr.ToStringAddrPort());
        } else {
            LogPrint(BCLog::NET_NETCONN, "Closing Masternode connection: peer=%d\n", pnode->GetId());
        }
        pnode->fDisconnect = true;
    });
}
