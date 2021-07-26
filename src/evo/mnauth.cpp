// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/mnauth.h>

#include <evo/deterministicmns.h>
#include <llmq/quorums_utils.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-meta.h>
#include <masternode/masternode-sync.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <validation.h>

#include <unordered_set>

void CMNAuth::PushMNAUTH(CNode* pnode, CConnman& connman)
{
    LOCK(activeMasternodeInfoCs);
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    uint256 signHash;
    auto receivedMNAuthChallenge = pnode->GetReceivedMNAuthChallenge();
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
    if (pnode->nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
        signHash = ::SerializeHash(std::make_tuple(*activeMasternodeInfo.blsPubKeyOperator, receivedMNAuthChallenge, pnode->fInbound));
    } else {
        signHash = ::SerializeHash(std::make_tuple(*activeMasternodeInfo.blsPubKeyOperator, receivedMNAuthChallenge, pnode->fInbound, nOurNodeVersion));
    }

    CMNAuth mnauth;
    mnauth.proRegTxHash = activeMasternodeInfo.proTxHash;
    mnauth.sig = activeMasternodeInfo.blsKeyOperator->Sign(signHash);

    LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- Sending MNAUTH, peer=%d\n", __func__, pnode->GetId());

    connman.PushMessage(pnode, CNetMsgMaker(pnode->GetSendVersion()).Make(NetMsgType::MNAUTH, mnauth));
}

void CMNAuth::ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (!masternodeSync.IsBlockchainSynced()) {
        // we can't verify MNAUTH messages when we don't have the latest MN list
        return;
    }

    if (strCommand == NetMsgType::MNAUTH) {
        CMNAuth mnauth;
        vRecv >> mnauth;

        // only one MNAUTH allowed
        bool fAlreadyHaveMNAUTH = !pnode->GetVerifiedProRegTxHash().IsNull();
        if (fAlreadyHaveMNAUTH) {
            LOCK(cs_main);
            Misbehaving(pnode->GetId(), 100, "duplicate mnauth");
            return;
        }

        if ((~pnode->nServices) & (NODE_NETWORK | NODE_BLOOM)) {
            // either NODE_NETWORK or NODE_BLOOM bit is missing in node's services
            LOCK(cs_main);
            Misbehaving(pnode->GetId(), 100, "mnauth from a node with invalid services");
            return;
        }

        if (mnauth.proRegTxHash.IsNull()) {
            LOCK(cs_main);
            Misbehaving(pnode->GetId(), 100, "empty mnauth proRegTxHash");
            return;
        }

        if (!mnauth.sig.IsValid()) {
            LOCK(cs_main);
            Misbehaving(pnode->GetId(), 100, "invalid mnauth signature");
            return;
        }

        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmn = mnList.GetMN(mnauth.proRegTxHash);
        if (!dmn) {
            LOCK(cs_main);
            // in case node was unlucky and not up to date, just let it be connected as a regular node, which gives it
            // a chance to get up-to-date and thus realize that it's not a MN anymore. We still give it a
            // low DoS score.
            Misbehaving(pnode->GetId(), 10, "missing mnauth masternode");
            return;
        }

        uint256 signHash;
        int nOurNodeVersion{PROTOCOL_VERSION};
        if (Params().NetworkIDString() != CBaseChainParams::MAIN && gArgs.IsArgSet("-pushversion")) {
            nOurNodeVersion = gArgs.GetArg("-pushversion", PROTOCOL_VERSION);
        }
        // See comment in PushMNAUTH (fInbound is negated here as we're on the other side of the connection)
        if (pnode->nVersion < MNAUTH_NODE_VER_VERSION || nOurNodeVersion < MNAUTH_NODE_VER_VERSION) {
            signHash = ::SerializeHash(std::make_tuple(dmn->pdmnState->pubKeyOperator, pnode->GetSentMNAuthChallenge(), !pnode->fInbound));
        } else {
            signHash = ::SerializeHash(std::make_tuple(dmn->pdmnState->pubKeyOperator, pnode->GetSentMNAuthChallenge(), !pnode->fInbound, pnode->nVersion.load()));
        }
        LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- constructed signHash for nVersion %d, peer=%d\n", __func__, pnode->nVersion, pnode->GetId());

        if (!mnauth.sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator.Get(), signHash)) {
            LOCK(cs_main);
            // Same as above, MN seems to not know its fate yet, so give it a chance to update. If this is a
            // malicious node (DoSing us), it'll get banned soon.
            Misbehaving(pnode->GetId(), 10, "mnauth signature verification failed");
            return;
        }

        if (!pnode->fInbound) {
            mmetaman.GetMetaInfo(mnauth.proRegTxHash)->SetLastOutboundSuccess(GetAdjustedTime());
            if (pnode->m_masternode_probe_connection) {
                LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode probe successful for %s, disconnecting. peer=%d\n",
                         mnauth.proRegTxHash.ToString(), pnode->GetId());
                pnode->fDisconnect = true;
                return;
            }
        }

        connman.ForEachNode([&](CNode* pnode2) {
            if (pnode->fDisconnect) {
                // we've already disconnected the new peer
                return;
            }

            if (pnode2->GetVerifiedProRegTxHash() == mnauth.proRegTxHash) {
                if (fMasternodeMode) {
                    auto deterministicOutbound = WITH_LOCK(activeMasternodeInfoCs, return llmq::CLLMQUtils::DeterministicOutboundConnection(activeMasternodeInfo.proTxHash, mnauth.proRegTxHash));
                    LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, deterministicOutbound=%s. peer=%d\n",
                             mnauth.proRegTxHash.ToString(), pnode2->GetId(), deterministicOutbound.ToString(), pnode->GetId());
                    if (WITH_LOCK(activeMasternodeInfoCs, return deterministicOutbound == activeMasternodeInfo.proTxHash)) {
                        if (pnode2->fInbound) {
                            LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping old inbound, peer=%d\n", pnode2->GetId());
                            pnode2->fDisconnect = true;
                        } else if (pnode->fInbound) {
                            LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping new inbound, peer=%d\n", pnode->GetId());
                            pnode->fDisconnect = true;
                        }
                    } else {
                        if (!pnode2->fInbound) {
                            LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping old outbound, peer=%d\n", pnode2->GetId());
                            pnode2->fDisconnect = true;
                        } else if (!pnode->fInbound) {
                            LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- dropping new outbound, peer=%d\n", pnode->GetId());
                            pnode->fDisconnect = true;
                        }
                    }
                } else {
                    LogPrint(BCLog::NET_NETCONN, "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, dropping new connection. peer=%d\n",
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

        if (!pnode->m_masternode_iqr_connection && connman.IsMasternodeQuorumRelayMember(pnode->GetVerifiedProRegTxHash())) {
            // Tell our peer that we're interested in plain LLMQ recovered signatures.
            // Otherwise the peer would only announce/send messages resulting from QRECSIG,
            // e.g. InstantSend locks or ChainLocks. SPV and regular full nodes should not send
            // this message as they are usually only interested in the higher level messages.
            const CNetMsgMaker msgMaker(pnode->GetSendVersion());
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSENDRECSIGS, true));
            pnode->m_masternode_iqr_connection = true;
        }

        LogPrint(BCLog::NET_NETCONN, "CMNAuth::%s -- Valid MNAUTH for %s, peer=%d\n", __func__, mnauth.proRegTxHash.ToString(), pnode->GetId());
    }
}

void CMNAuth::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    // we're only interested in updated/removed MNs. Added MNs are of no interest for us
    if (diff.updatedMNs.empty() && diff.removedMns.empty()) {
        return;
    }

    g_connman->ForEachNode([&](CNode* pnode) {
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (verifiedProRegTxHash.IsNull()) {
            return;
        }
        auto verifiedDmn = oldMNList.GetMN(verifiedProRegTxHash);
        if (!verifiedDmn) {
            return;
        }
        bool doRemove = false;
        if (diff.removedMns.count(verifiedDmn->GetInternalId())) {
            doRemove = true;
        } else {
            auto it = diff.updatedMNs.find(verifiedDmn->GetInternalId());
            if (it != diff.updatedMNs.end()) {
                if ((it->second.fields & CDeterministicMNStateDiff::Field_pubKeyOperator) && it->second.state.pubKeyOperator.GetHash() != pnode->GetVerifiedPubKeyHash()) {
                    doRemove = true;
                }
            }
        }

        if (doRemove) {
            LogPrint(BCLog::NET_NETCONN, "CMNAuth::NotifyMasternodeListChanged -- Disconnecting MN %s due to key changed/removed, peer=%d\n",
                     verifiedProRegTxHash.ToString(), pnode->GetId());
            pnode->fDisconnect = true;
        }
    });
}
