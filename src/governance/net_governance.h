// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_NET_GOVERNANCE_H
#define BITCOIN_GOVERNANCE_NET_GOVERNANCE_H

#include <net_processing.h>

class CGovernanceManager;
class CMasternodeSync;
class CNetFulfilledRequestManager;

class NetGovernance final : public NetHandler
{
public:
    NetGovernance(PeerManagerInternal* peer_manager, CGovernanceManager& gov_manager, CMasternodeSync& node_sync,
                  CNetFulfilledRequestManager& netfulfilledman) :
        NetHandler(peer_manager),
        m_gov_manager(gov_manager),
        m_node_sync(node_sync),
        m_netfulfilledman(netfulfilledman)
    {
    }
    void Schedule(CScheduler& scheduler, CConnman& connman) override;

    void ProcessMessage(CNode& peer, CConnman& connman, const std::string& msg_type, CDataStream& vRecv) override;
private:
    void SendGovernanceSyncRequest(CNode* pnode, CConnman& connman) const;
    int RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman) const;
    void ProcessTick(CConnman& connman);

    CGovernanceManager& m_gov_manager;
    CMasternodeSync& m_node_sync;
    CNetFulfilledRequestManager& m_netfulfilledman;
};

#endif // BITCOIN_GOVERNANCE_NET_GOVERNANCE_H
