// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/net_governance.h>

#include <chainparams.h>
#include <common/bloom.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <logging.h>
#include <masternode/sync.h>
#include <net.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <scheduler.h>

class CConnman;

void NetGovernance::Schedule(CScheduler& scheduler)
{
    // Code below is meant to be running only if governance validation is enabled
    //
    if (!m_gov_manager.IsValid()) return;
    scheduler.scheduleEvery(
        [this]() -> void {
            if (!m_node_sync.IsSynced()) return;

            // Request governance objects for orphan votes
            auto vecOrphanHashes = m_gov_manager.GetOrphanVoteObjectHashes();
            if (!vecOrphanHashes.empty()) {
                LogPrint(BCLog::GOBJECT, "NetGovernance::Schedule -- requesting %d orphan objects\n",
                         vecOrphanHashes.size());
                const CConnman::NodesSnapshot snap{m_connman, CConnman::FullyConnectedOnly};
                for (const uint256& nHash : vecOrphanHashes) {
                    for (CNode* pnode : snap.Nodes()) {
                        if (!pnode->CanRelay()) continue;
                        CNetMsgMaker msgMaker(pnode->GetCommonVersion());
                        CBloomFilter filter; // Empty filter - we want the object, not votes
                        m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, nHash, filter));
                    }
                }
            }

            // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS
            m_gov_manager.CheckAndRemove();
        },
        std::chrono::minutes{5});

    scheduler.scheduleEvery(
        [this]() -> void {
            auto relay_invs = m_gov_manager.FetchRelayInventory();
            for (const auto& inv : relay_invs) {
                m_peer_manager->PeerRelayInv(inv);
            }
        },
        // Tests need tighter timings to avoid timeouts, use more relaxed pacing otherwise
        Params().IsMockableChain() ? std::chrono::seconds{1} : std::chrono::seconds{5});
}

void NetGovernance::ProcessMessage(CNode& peer, const std::string& msg_type, CDataStream& vRecv)
{
    if (!m_gov_manager.IsValid()) return;
    if (!m_node_sync.IsBlockchainSynced()) return;

    // ANOTHER USER IS ASKING US TO HELP THEM SYNC GOVERNANCE OBJECT DATA
    if (msg_type == NetMsgType::MNGOVERNANCESYNC) {
        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!m_node_sync.IsSynced()) return;

        uint256 nProp;
        CBloomFilter filter;
        vRecv >> nProp;
        vRecv >> filter;

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- syncing governance objects to our peer %s\n", peer.GetLogString());
        if (nProp == uint256()) {
            // Full sync of all governance objects
            assert(m_netfulfilledman.IsValid());
            if (m_netfulfilledman.HasFulfilledRequest(peer.addr, NetMsgType::MNGOVERNANCESYNC)) {
                // Asking for the whole list multiple times in a short period of time is no good
                LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- peer already asked me for the list\n");
                m_peer_manager->PeerMisbehaving(peer.GetId(), 20);
                return;
            }
            m_netfulfilledman.AddFulfilledRequest(peer.addr, NetMsgType::MNGOVERNANCESYNC);

            auto invs = m_gov_manager.GetSyncableObjectInvs();
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- syncing %d objects to peer=%d\n", invs.size(), peer.GetId());

            CNetMsgMaker msgMaker(peer.GetCommonVersion());
            m_connman.PushMessage(&peer, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ,
                                                       static_cast<int>(invs.size())));
            for (const auto& inv : invs) {
                m_peer_manager->PeerRelayInv(inv);
            }
        } else {
            // Sync votes for a specific governance object
            auto invs = m_gov_manager.GetSyncableVoteInvs(nProp, filter);
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCESYNC -- syncing %d votes for %s to peer=%d\n", invs.size(),
                     nProp.ToString(), peer.GetId());

            CNetMsgMaker msgMaker(peer.GetCommonVersion());
            m_connman.PushMessage(&peer, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_GOVOBJ_VOTE,
                                                       static_cast<int>(invs.size())));
            for (const auto& inv : invs) {
                m_peer_manager->PeerRelayInv(inv);
            }
        }
    }
    // A NEW GOVERNANCE OBJECT HAS ARRIVED
    else if (msg_type == NetMsgType::MNGOVERNANCEOBJECT) {
        // MAKE SURE WE HAVE A VALID REFERENCE TO THE TIP BEFORE CONTINUING
        CGovernanceObject govobj;
        vRecv >> govobj;

        uint256 nHash = govobj.GetHash();

        WITH_LOCK(::cs_main, m_peer_manager->PeerEraseObjectRequest(peer.GetId(), CInv{MSG_GOVERNANCE_OBJECT, nHash}));

        if (!m_node_sync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- masternode list not synced\n");
            return;
        }

        std::string strHash = nHash.ToString();

        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received object: %s\n", strHash);

        if (!m_gov_manager.AcceptMessage(nHash)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECT -- Received unrequested object: %s\n", strHash);
            return;
        }

        if (!WITH_LOCK(::cs_main, return m_gov_manager.ProcessObject(peer, nHash, govobj))) {
            // apply node's ban score
            m_peer_manager->PeerMisbehaving(peer.GetId(), 20);
        }
    }

    // A NEW GOVERNANCE OBJECT VOTE HAS ARRIVED
    else if (msg_type == NetMsgType::MNGOVERNANCEOBJECTVOTE) {
        CGovernanceVote vote;
        vRecv >> vote;

        uint256 nHash = vote.GetHash();

        WITH_LOCK(::cs_main, m_peer_manager->PeerEraseObjectRequest(peer.GetId(), CInv{MSG_GOVERNANCE_OBJECT_VOTE, nHash}));

        // Ignore such messages until masternode list is synced
        if (!m_node_sync.IsBlockchainSynced()) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- masternode list not synced\n");
            return;
        }

        const auto tip_mn_list = m_gov_manager.GetMNManager().GetListAtChainTip();
        LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Received vote: %s\n", vote.ToString(tip_mn_list));

        std::string strHash = nHash.ToString();

        if (!m_gov_manager.AcceptMessage(nHash)) {
            LogPrint(BCLog::GOBJECT, /* Continued */
                     "MNGOVERNANCEOBJECTVOTE -- Received unrequested vote object: %s, hash: %s, peer = %d\n",
                     vote.ToString(tip_mn_list), strHash, peer.GetId());
            return;
        }

        CGovernanceException exception;
        uint256 hashToRequest;
        if (m_gov_manager.ProcessVote(&peer, vote, exception, hashToRequest)) {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- %s new\n", strHash);
            m_node_sync.BumpAssetLastTime("MNGOVERNANCEOBJECTVOTE");

            if (!m_node_sync.IsSynced()) {
                LogPrint(BCLog::GOBJECT, "%s -- won't relay until fully synced\n", __func__);
                return;
            }
            auto dmn = tip_mn_list.GetMNByCollateral(vote.GetMasternodeOutpoint());
            if (!dmn) {
                return;
            }
            m_gov_manager.RelayVote(vote);
            // TODO: figure out why immediate sending of inventory doesn't work here!
            // m_peer_manager->PeerRelayInv(CInv{MSG_GOVERNANCE_OBJECT_VOTE, nHash});
        } else {
            LogPrint(BCLog::GOBJECT, "MNGOVERNANCEOBJECTVOTE -- Rejected vote, error = %s\n", exception.what());
            if (hashToRequest != uint256()) {
                // Orphan vote - request the missing governance object
                CNetMsgMaker msgMaker(peer.GetCommonVersion());
                CBloomFilter filter; // Empty filter - we just want the object, not votes
                m_connman.PushMessage(&peer, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, hashToRequest, filter));
            }
            if ((exception.GetNodePenalty() != 0) && m_node_sync.IsSynced()) {
                m_peer_manager->PeerMisbehaving(peer.GetId(), exception.GetNodePenalty());
            }
        }
    }
}
