// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mnauth.h"

#include "activemasternode.h"
#include "evo/deterministicmns.h"
#include "masternode-sync.h"
#include "net.h"
#include "net_processing.h"
#include "netmessagemaker.h"
#include "validation.h"

#include <unordered_set>

void CMNAuth::PushMNAUTH(CNode* pnode, CConnman& connman)
{
    if (!fMasternodeMode || activeMasternodeInfo.proTxHash.IsNull()) {
        return;
    }

    uint256 signHash;
    {
        LOCK(pnode->cs_mnauth);
        if (pnode->receivedMNAuthChallenge.IsNull()) {
            return;
        }
        // We include fInbound in signHash to forbid interchanging of challenges by a man in the middle. This way
        // we protect ourself against MITM in this form:
        //   node1 <- Eve -> node2
        // It does not protect against:
        //   node1 -> Eve -> node2
        // This is ok as we only use MNAUTH as a DoS protection and not for sensitive stuff
        signHash = ::SerializeHash(std::make_tuple(*activeMasternodeInfo.blsPubKeyOperator, pnode->receivedMNAuthChallenge, pnode->fInbound));
    }

    CMNAuth mnauth;
    mnauth.proRegTxHash = activeMasternodeInfo.proTxHash;
    mnauth.sig = activeMasternodeInfo.blsKeyOperator->Sign(signHash);

    LogPrint("net", "CMNAuth::%s -- Sending MNAUTH, peer=%d\n", __func__, pnode->id);

    connman.PushMessage(pnode, CNetMsgMaker(pnode->GetSendVersion()).Make(NetMsgType::MNAUTH, mnauth));
}

void CMNAuth::ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (!masternodeSync.IsBlockchainSynced()) {
        // we can't really verify MNAUTH messages when we don't have the latest MN list
        return;
    }

    if (strCommand == NetMsgType::MNAUTH) {
        CMNAuth mnauth;
        vRecv >> mnauth;

        {
            LOCK(pnode->cs_mnauth);
            // only one MNAUTH allowed
            if (!pnode->verifiedProRegTxHash.IsNull()) {
                LOCK(cs_main);
                Misbehaving(pnode->id, 100);
                return;
            }
        }

        if (mnauth.proRegTxHash.IsNull() || !mnauth.sig.IsValid()) {
            LOCK(cs_main);
            Misbehaving(pnode->id, 100);
            return;
        }

        auto mnList = deterministicMNManager->GetListAtChainTip();
        auto dmn = mnList.GetMN(mnauth.proRegTxHash);
        if (!dmn) {
            LOCK(cs_main);
            // in case he was unlucky and not up to date, just let him be connected as a regular node, which gives him
            // a chance to get up-to-date and thus realize by himself that he's not a MN anymore. We still give him a
            // low DoS score.
            Misbehaving(pnode->id, 10);
            return;
        }

        uint256 signHash;
        {
            LOCK(pnode->cs_mnauth);
            // See comment in PushMNAUTH (fInbound is negated here as we're on the other side of the connection)
            signHash = ::SerializeHash(std::make_tuple(dmn->pdmnState->pubKeyOperator, pnode->sentMNAuthChallenge, !pnode->fInbound));
        }

        if (!mnauth.sig.VerifyInsecure(dmn->pdmnState->pubKeyOperator.Get(), signHash)) {
            LOCK(cs_main);
            // Same as above, MN seems to not know about his fate yet, so give him a chance to update. If this is a
            // malicious actor (DoSing us), we'll ban him soon.
            Misbehaving(pnode->id, 10);
            return;
        }

        connman.ForEachNode([&](CNode* pnode2) {
            if (pnode2->verifiedProRegTxHash == mnauth.proRegTxHash) {
                LogPrint("net", "CMNAuth::ProcessMessage -- Masternode %s has already verified as peer %d, dropping old connection. peer=%d\n",
                        mnauth.proRegTxHash.ToString(), pnode2->id, pnode->id);
                pnode2->fDisconnect = true;
            }
        });

        {
            LOCK(pnode->cs_mnauth);
            pnode->verifiedProRegTxHash = mnauth.proRegTxHash;
            pnode->verifiedPubKeyHash = dmn->pdmnState->pubKeyOperator.GetHash();
        }

        LogPrint("net", "CMNAuth::%s -- Valid MNAUTH for %s, peer=%d\n", __func__, mnauth.proRegTxHash.ToString(), pnode->id);
    }
}

void CMNAuth::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)
{
    // we're only interested in updated/removed MNs. Added MNs are of no interest for us
    if (diff.updatedMNs.empty() && diff.removedMns.empty()) {
        return;
    }

    g_connman->ForEachNode([&](CNode* pnode) {
        LOCK(pnode->cs_mnauth);
        if (pnode->verifiedProRegTxHash.IsNull()) {
            return;
        }
        bool doRemove = false;
        if (diff.removedMns.count(pnode->verifiedProRegTxHash)) {
            doRemove = true;
        } else {
            auto it = diff.updatedMNs.find(pnode->verifiedProRegTxHash);
            if (it != diff.updatedMNs.end()) {
                if (it->second->pubKeyOperator.GetHash() != pnode->verifiedPubKeyHash) {
                    doRemove = true;
                }
            }
        }

        if (doRemove) {
            LogPrint("net", "CMNAuth::NotifyMasternodeListChanged -- Disconnecting MN %s due to key changed/removed, peer=%d\n",
                     pnode->verifiedProRegTxHash.ToString(), pnode->id);
            pnode->fDisconnect = true;
        }
    });
}
