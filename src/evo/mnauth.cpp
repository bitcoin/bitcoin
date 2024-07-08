// Copyright (c) 2019-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/mnauth.h>

#include <evo/deterministicmns.h>
#include <llmq/quorums_utils.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodemeta.h>
#include <masternode/masternodesync.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <validation.h>
#include <timedata.h>
#include <unordered_set>
#include <common/args.h>
#include <logging.h>
void CMNAuth::PushMNAUTH(CNode* pnode, CConnman& connman, const int nHeight)
{
    LOCK(activeMasternodeInfoCs);
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    uint256 signHash;
    const auto receivedMNAuthChallenge = pnode->GetReceivedMNAuthChallenge();
    if (receivedMNAuthChallenge.IsNull()) {
        return;
    }
    // We include fInbound in signHash to forbid interchanging of challenges by a man in the middle (MITM). This way
    // we protect ourselves against MITM in this form:
    //   node1 <- Eve -> node2
    // It does not protect against:
    //   node1 -> Eve -> node2
    // This is ok as we only use MNAUTH as a DoS protection and not for sensitive stuff
    int nOurNodeVersion{PROTOCOL_VERSION};
    if (fRegTest && gArgs.IsArgSet("-pushversion")) {
        nOurNodeVersion = gArgs.GetIntArg("-pushversion", PROTOCOL_VERSION);
    }
    bool isV19active = llmq::CLLMQUtils::IsV19Active(nHeight);
    const CBLSPublicKeyVersionWrapper pubKey(*activeMasternodeInfo.blsPubKeyOperator, !isV19active);
    if (pnode->nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
        signHash = ::SerializeHash(std::make_tuple(pubKey, receivedMNAuthChallenge, pnode->IsInboundConn()));
    } else {
        signHash = ::SerializeHash(std::make_tuple(pubKey, receivedMNAuthChallenge, pnode->IsInboundConn(), nOurNodeVersion));
    }

    CMNAuth mnauth;
    mnauth.proRegTxHash = activeMasternodeInfo.proTxHash;
    mnauth.sig = activeMasternodeInfo.blsKeyOperator->Sign(signHash);

    LogPrint(BCLog::NET, "CMNAuth::%s -- Sending MNAUTH, peer=%d\n", __func__, pnode->GetId());

    connman.PushMessage(pnode, CNetMsgMaker(pnode->GetCommonVersion()).Make(NetMsgType::MNAUTH, mnauth));
}

void CMNAuth::ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv, ChainstateManager &chainman, CConnman& connman, PeerManager& peerman)
{
    if (strCommand != NetMsgType::MNAUTH || !masternodeSync.IsBlockchainSynced()) {
        // we can't verify MNAUTH messages when we don't have the latest MN list
        return;
    }

    CMNAuth mnauth;
    vRecv >> mnauth;
    PeerRef peer = peerman.GetPeerRef(pnode->GetId());
    // only one MNAUTH allowed
    if (!pnode->GetVerifiedProRegTxHash().IsNull()) {
        if(peer)
            peerman.Misbehaving(*peer, 100, "duplicate mnauth");
        return;
    }

    if (peer && !CanServeBlocks(*peer)) {
        // NODE_NETWORK bit is missing in node's services
        peerman.Misbehaving(*peer, 100, "mnauth from a node with invalid services");
        return;
    }

    if (mnauth.proRegTxHash.IsNull()) {
        if(peer)
            peerman.Misbehaving(*peer, 100, "empty mnauth proRegTxHash");
        return;
    }

    if (!mnauth.sig.IsValid()) {
        if(peer)
            peerman.Misbehaving(*peer, 100, "invalid mnauth signature");
        LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- invalid mnauth for protx=%s with sig=%s\n", mnauth.proRegTxHash.ToString(), mnauth.sig.ToString());
        return;
    }
    auto mnList = deterministicMNManager->GetListAtChainTip();
    const auto dmn = mnList.GetMN(mnauth.proRegTxHash);
    if (!dmn) {
        // in case node was unlucky and not up to date, just let it be connected as a regular node, which gives it
        // a chance to get up-to-date and thus realize that it's not a MN anymore. We still give it a
        // low DoS score.
        if(peer)
            peerman.Misbehaving(*peer, 10, "missing mnauth masternode");
        return;
    }

    uint256 signHash;
    
    int nOurNodeVersion{PROTOCOL_VERSION};
    if (fRegTest && gArgs.IsArgSet("-pushversion")) {
        nOurNodeVersion = gArgs.GetIntArg("-pushversion", PROTOCOL_VERSION);
    }
    int nHeight = WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight());
    bool isV19active = llmq::CLLMQUtils::IsV19Active(nHeight);
    ConstCBLSPublicKeyVersionWrapper pubKey(dmn->pdmnState->pubKeyOperator.Get(), !isV19active);
    // See comment in PushMNAUTH (fInbound is negated here as we're on the other side of the connection)
    if (pnode->nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
        signHash = ::SerializeHash(std::make_tuple(pubKey, pnode->GetSentMNAuthChallenge(), !pnode->IsInboundConn()));
    } else {
        signHash = ::SerializeHash(std::make_tuple(pubKey, pnode->GetSentMNAuthChallenge(), !pnode->IsInboundConn(), pnode->nVersion.load()));
    }
    LogPrint(BCLog::NET, "CMNAuth::%s -- constructed signHash for nVersion %d, peer=%d\n", __func__, pnode->nVersion, pnode->GetId());
    if (!mnauth.sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator.Get(), signHash)) {
        // Same as above, MN seems to not know its fate yet, so give it a chance to update. If this is a
        // malicious node (DoSing us), it'll get banned soon.
        if(peer)
            peerman.Misbehaving(*peer, 10, "mnauth signature verification failed");
        return;
    }

    if (!pnode->IsInboundConn()) {
        mmetaman->GetMetaInfo(mnauth.proRegTxHash)->SetLastOutboundSuccess(TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()));
        if (pnode->m_masternode_probe_connection) {
            LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- Masternode probe successful for %s, disconnecting. peer=%d\n",
                        mnauth.proRegTxHash.ToString(), pnode->GetId());
            pnode->fDisconnect = true;
            return;
        }
    }
    uint256 proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    connman.ForEachNode([&](CNode* pnode2) {
        if (pnode->fDisconnect) {
            // we've already disconnected the new peer
            return;
        }

        if (pnode2->GetVerifiedProRegTxHash() == mnauth.proRegTxHash) {
            if (fMasternodeMode) {
                auto deterministicOutbound = llmq::CLLMQUtils::DeterministicOutboundConnection(proTxHash, mnauth.proRegTxHash);
                LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, deterministicOutbound=%s. peer=%d\n",
                            mnauth.proRegTxHash.ToString(), pnode2->GetId(), deterministicOutbound.ToString(), pnode->GetId());
                if (deterministicOutbound == proTxHash) {
                    if (pnode2->IsInboundConn()) {
                        LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- dropping old inbound, peer=%d\n", pnode2->GetId());
                        pnode2->fDisconnect = true;
                    } else if (pnode->IsInboundConn()) {
                        LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- dropping new inbound, peer=%d\n", pnode->GetId());
                        pnode->fDisconnect = true;
                    }
                } else {
                    if (!pnode2->IsInboundConn()) {
                        LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- dropping old outbound, peer=%d\n", pnode2->GetId());
                        pnode2->fDisconnect = true;
                    } else if (!pnode->IsInboundConn()) {
                        LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- dropping new outbound, peer=%d\n", pnode->GetId());
                        pnode->fDisconnect = true;
                    }
                }
            } else if (!fRegTest) {
                LogPrint(BCLog::NET, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, dropping new connection. peer=%d\n",
                        mnauth.proRegTxHash.ToString(), pnode2->GetId(), pnode->GetId());
                pnode->fDisconnect = true;
            }
        }
    });
    if (pnode->fDisconnect) {
        return;
    }
    pnode->SetVerifiedProRegTxHash(mnauth.proRegTxHash);
    pnode->SetVerifiedPubKeyHash(dmn->pdmnState->pubKeyOperator.GetHash());
    bool iqr;
    {
        LOCK(pnode->cs_mnauth);
        iqr = pnode->m_masternode_iqr_connection;
        
    }
    if (!iqr && connman.IsMasternodeQuorumRelayMember(mnauth.proRegTxHash)) {
        // Tell our peer that we're interested in plain LLMQ recovered signatures.
        // Otherwise the peer would only announce/send messages resulting from QRECSIG,
        // e.g. InstantSend locks or ChainLocks. SPV and regular full nodes should not send
        // this message as they are usually only interested in the higher level messages.
        const CNetMsgMaker msgMaker(pnode->GetCommonVersion());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSENDRECSIGS));
        {
            LOCK(pnode->cs_mnauth);
            pnode->m_masternode_iqr_connection = true;
        }
        LogPrint(BCLog::NET, "CMNAuth::%s -- Valid MNAUTH for %s, peer=%d\n", __func__, mnauth.proRegTxHash.ToString(), pnode->GetId());
    }

}

void CMNAuth::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff, CConnman& connman)
{
    // we're only interested in updated/removed MNs. Added MNs are of no interest for us
    if (diff.updatedMNs.empty() && diff.removedMns.empty()) {
        return;
    }

    connman.ForEachNode([&oldMNList, &diff](CNode* pnode) {
        const auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (verifiedProRegTxHash.IsNull()) {
            return;
        }
        const auto verifiedDmn = oldMNList.GetMN(verifiedProRegTxHash);
        if (!verifiedDmn) {
            return;
        }
        bool doRemove = false;
        if (diff.removedMns.count(verifiedDmn->GetInternalId())) {
            doRemove = true;
        } else if (const auto it = diff.updatedMNs.find(verifiedDmn->GetInternalId()); it != diff.updatedMNs.end()) {
            if ((it->second.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && it->second.state.pubKeyOperator.GetHash() != pnode->GetVerifiedPubKeyHash()) {
                doRemove = true;
            }
        }

        if (doRemove) {
            LogPrint(BCLog::NET, "CMNAuth::NotifyMasternodeListChanged -- Disconnecting MN %s due to key changed/removed, peer=%d\n",
                     verifiedProRegTxHash.ToString(), pnode->GetId());
            pnode->fDisconnect = true;
        }
    });
    
}
