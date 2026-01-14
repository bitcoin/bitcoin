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
    NetGovernance(PeerManagerInternal* peer_manager, CGovernanceManager& gov_manager, CMasternodeSync& node_sync, CNetFulfilledRequestManager& netfulfilledman,
                  CConnman& connman) :
        NetHandler(peer_manager),
        m_gov_manager(gov_manager),
        m_node_sync(node_sync),
        m_netfulfilledman(netfulfilledman),
        m_connman(connman)
    {
    }
    void Schedule(CScheduler& scheduler) override;

    void ProcessMessage(CNode& peer, const std::string& msg_type, CDataStream& vRecv) override;

private:
    CGovernanceManager& m_gov_manager;
    CMasternodeSync& m_node_sync;
    CNetFulfilledRequestManager& m_netfulfilledman;
    CConnman& m_connman;
};

#endif // BITCOIN_GOVERNANCE_NET_GOVERNANCE_H
