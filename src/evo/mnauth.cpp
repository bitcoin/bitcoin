// Copyright (c) 2019-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/mnauth.h>

#include <bls/bls.h>
#include <chain.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <llmq/utils.h>
#include <masternode/meta.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <validation.h>

void CMNAuth::PushMNAUTH(CNode& peer, CConnman& connman, const CBlockIndex* tip)
{
    LOCK(activeMasternodeInfoCs);
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    uint256 signHash;
    const auto receivedMNAuthChallenge = peer.GetReceivedMNAuthChallenge();
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
    if (Params().NetworkIDString() != CBaseChainParams::MAIN && gArgs.IsArgSet("-pushversion")) {
        nOurNodeVersion = gArgs.GetArg("-pushversion", PROTOCOL_VERSION);
    }
    bool isV19active = llmq::utils::IsV19Active(tip);
    const CBLSPublicKeyVersionWrapper pubKey(*activeMasternodeInfo.blsPubKeyOperator, !isV19active);
    if (peer.nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
        signHash = ::SerializeHash(std::make_tuple(pubKey, receivedMNAuthChallenge, peer.fInbound));
    } else {
        signHash = ::SerializeHash(std::make_tuple(pubKey, receivedMNAuthChallenge, peer.fInbound, nOurNodeVersion));
    }

    CMNAuth mnauth;
    mnauth.proRegTxHash = activeMasternodeInfo.proTxHash;
    mnauth.sig = activeMasternodeInfo.blsKeyOperator->Sign(signHash);

    LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- Sending MNAUTH, peer=%d\n", __func__, peer.GetId());

    connman.PushMessage(&peer, CNetMsgMaker(peer.GetSendVersion()).Make(NetMsgType::MNAUTH, mnauth));
}

void CMNAuth::ProcessMessage(CNode& peer, PeerManager& peerman, CConnman& connman, std::string_view msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::MNAUTH || !::masternodeSync->IsBlockchainSynced()) {
        // we can't verify MNAUTH messages when we don't have the latest MN list
        return;
    }

    CMNAuth mnauth;
    vRecv >> mnauth;

    // only one MNAUTH allowed
    if (!peer.GetVerifiedProRegTxHash().IsNull()) {
        peerman.Misbehaving(peer.GetId(), 100, "duplicate mnauth");
        return;
    }

    if ((~peer.nServices) & (NODE_NETWORK | NODE_BLOOM)) {
        // either NODE_NETWORK or NODE_BLOOM bit is missing in node's services
        peerman.Misbehaving(peer.GetId(), 100, "mnauth from a node with invalid services");
        return;
    }

    if (mnauth.proRegTxHash.IsNull()) {
        peerman.Misbehaving(peer.GetId(), 100, "empty mnauth proRegTxHash");
        return;
    }

    if (!mnauth.sig.IsValid()) {
        peerman.Misbehaving(peer.GetId(), 100, "invalid mnauth signature");
        LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- invalid mnauth for protx=%s with sig=%s\n", mnauth.proRegTxHash.ToString(), mnauth.sig.ToString());
        return;
    }

    const auto mnList = deterministicMNManager->GetListAtChainTip();
    const auto dmn = mnList.GetMN(mnauth.proRegTxHash);
    if (!dmn) {
        // in case node was unlucky and not up to date, just let it be connected as a regular node, which gives it
        // a chance to get up-to-date and thus realize that it's not a MN anymore. We still give it a
        // low DoS score.
        peerman.Misbehaving(peer.GetId(), 10, "missing mnauth masternode");
        return;
    }

    uint256 signHash;
    int nOurNodeVersion{PROTOCOL_VERSION};
    if (Params().NetworkIDString() != CBaseChainParams::MAIN && gArgs.IsArgSet("-pushversion")) {
        nOurNodeVersion = gArgs.GetArg("-pushversion", PROTOCOL_VERSION);
    }
    const CBlockIndex* tip = ::ChainActive().Tip();
    bool isV19active = llmq::utils::IsV19Active(tip);
    ConstCBLSPublicKeyVersionWrapper pubKey(dmn->pdmnState->pubKeyOperator.Get(), !isV19active);
    // See comment in PushMNAUTH (fInbound is negated here as we're on the other side of the connection)
    if (peer.nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
        signHash = ::SerializeHash(std::make_tuple(pubKey, peer.GetSentMNAuthChallenge(), !peer.fInbound));
    } else {
        signHash = ::SerializeHash(std::make_tuple(pubKey, peer.GetSentMNAuthChallenge(), !peer.fInbound, peer.nVersion.load()));
    }
    LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- constructed signHash for nVersion %d, peer=%d\n", __func__, peer.nVersion, peer.GetId());

    if (!mnauth.sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator.Get(), signHash)) {
        // Same as above, MN seems to not know its fate yet, so give it a chance to update. If this is a
        // malicious node (DoSing us), it'll get banned soon.
        peerman.Misbehaving(peer.GetId(), 10, "mnauth signature verification failed");
        return;
    }

    if (!peer.fInbound) {
        mmetaman.GetMetaInfo(mnauth.proRegTxHash)->SetLastOutboundSuccess(GetAdjustedTime());
        if (peer.m_masternode_probe_connection) {
            LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode probe successful for %s, disconnecting. peer=%d\n",
                     mnauth.proRegTxHash.ToString(), peer.GetId());
            peer.fDisconnect = true;
            return;
        }
    }

    connman.ForEachNode([&](CNode* pnode2) {
        if (peer.fDisconnect) {
            // we've already disconnected the new peer
            return;
        }

        if (pnode2->GetVerifiedProRegTxHash() == mnauth.proRegTxHash) {
            if (fMasternodeMode) {
                const auto deterministicOutbound = WITH_LOCK(activeMasternodeInfoCs, return llmq::utils::DeterministicOutboundConnection(activeMasternodeInfo.proTxHash, mnauth.proRegTxHash));
                LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, deterministicOutbound=%s. peer=%d\n",
                         mnauth.proRegTxHash.ToString(), pnode2->GetId(), deterministicOutbound.ToString(), peer.GetId());
                if (WITH_LOCK(activeMasternodeInfoCs, return deterministicOutbound == activeMasternodeInfo.proTxHash)) {
                    if (pnode2->fInbound) {
                        LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping old inbound, peer=%d\n", pnode2->GetId());
                        pnode2->fDisconnect = true;
                    } else if (peer.fInbound) {
                        LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping new inbound, peer=%d\n", peer.GetId());
                        peer.fDisconnect = true;
                    }
                } else {
                    if (!pnode2->fInbound) {
                        LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping old outbound, peer=%d\n", pnode2->GetId());
                        pnode2->fDisconnect = true;
                    } else if (!peer.fInbound) {
                        LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping new outbound, peer=%d\n", peer.GetId());
                        peer.fDisconnect = true;
                    }
                }
            } else {
                LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, dropping new connection. peer=%d\n",
                         mnauth.proRegTxHash.ToString(), pnode2->GetId(), peer.GetId());
                peer.fDisconnect = true;
            }
        }
    });

    if (peer.fDisconnect) {
        return;
    }

    peer.SetVerifiedProRegTxHash(mnauth.proRegTxHash);
    peer.SetVerifiedPubKeyHash(dmn->pdmnState->pubKeyOperator.GetHash());

    if (!peer.m_masternode_iqr_connection && connman.IsMasternodeQuorumRelayMember(peer.GetVerifiedProRegTxHash())) {
        // Tell our peer that we're interested in plain LLMQ recovered signatures.
        // Otherwise, the peer would only announce/send messages resulting from QRECSIG,
        // e.g. InstantSend locks or ChainLocks. SPV and regular full nodes should not send
        // this message as they are usually only interested in the higher level messages.
        const CNetMsgMaker msgMaker(peer.GetSendVersion());
        connman.PushMessage(&peer, msgMaker.Make(NetMsgType::QSENDRECSIGS, true));
        peer.m_masternode_iqr_connection = true;
    }

    LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- Valid MNAUTH for %s, peer=%d\n", __func__, mnauth.proRegTxHash.ToString(), peer.GetId());
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
            LogPrint(BCLog::NET_NETCONN, "CMNAuth::NotifyMasternodeListChanged -- Disconnecting MN %s due to key changed/removed, peer=%d\n",
                     verifiedProRegTxHash.ToString(), pnode->GetId());
            pnode->fDisconnect = true;
        }
    });
}
