// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_SYNC_MANAGER_H
#define BITCOIN_NODE_SYNC_MANAGER_H

#include <masternode/sync.h>
#include <net_processing.h>

class CGovernanceManager;
class CMasternodeSync;
class CNetFulfilledRequestManager;

class SyncManager final : public NetHandler
{
public:
    SyncManager(PeerManagerInternal* peer_manager, CGovernanceManager& gov_manager, CMasternodeSync& node_sync, CConnman& connman,
                  CNetFulfilledRequestManager& netfulfilledman) :
        NetHandler(peer_manager),
        m_gov_manager(gov_manager),
        m_node_sync(node_sync),
        m_connman(connman),
        m_netfulfilledman(netfulfilledman)
    {
    }
    void Schedule(CScheduler& scheduler) override;
    void ProcessMessage(CNode& peer, const std::string& msg_type, CDataStream& vRecv) override;

private:
    void SendGovernanceSyncRequest(CNode* pnode) const;
    void SendGovernanceObjectSyncRequest(CNode* pnode, const uint256& nHash, bool fUseFilter) const;
    int RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy) const;
    void ProcessTick();

    CGovernanceManager& m_gov_manager;
    CMasternodeSync& m_node_sync;
    CConnman& m_connman;
    CNetFulfilledRequestManager& m_netfulfilledman;
};

class NodeSyncNotifierImpl : public NodeSyncNotifier
{
public:
    NodeSyncNotifierImpl(CConnman& connman, CNetFulfilledRequestManager& netfulfilledman) :
        m_connman(connman),
        m_netfulfilledman(netfulfilledman)
    {}

    void SyncReset() override;
    void SyncFinished() override;
private:
    CConnman& m_connman;
    CNetFulfilledRequestManager& m_netfulfilledman;
};
#endif // BITCOIN_NODE_SYNC_MANAGER_H
