// Copyright (c) 2019-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_MNAUTH_H
#define BITCOIN_EVO_MNAUTH_H

#include <bls/bls.h>
#include <protocol.h>
#include <serialize.h>
#include <uint256.h>

#include <string_view>

class CActiveMasternodeManager;
class CBlockIndex;
class CConnman;
class CDataStream;
class CDeterministicMNList;
class CDeterministicMNListDiff;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNode;

enum ServiceFlags : uint64_t;

/**
 * This class handles the p2p message MNAUTH. MNAUTH is sent directly after VERACK and authenticates the sender as a
 * masternode. It is only sent when the sender is actually a masternode.
 *
 * MNAUTH signs a challenge that was previously sent via VERSION. The challenge is signed differently depending on
 * the connection being an inbound or outbound connection, which avoids MITM of this form:
 *   node1 <- Eve -> node2
 * while still allowing:
 *   node1 -> Eve -> node2
 *
 * This is fine as we only use this mechanism for DoS protection. It allows us to keep masternode connections open for
 * a very long time without evicting the connections when inbound connection limits are hit (non-MNs will then be evicted).
 *
 * If we ever want to add transfer of sensitive data, THIS AUTHENTICATION MECHANISM IS NOT ENOUGH!! We'd need to implement
 * proper encryption for these connections first.
 */

class CMNAuth
{
public:
    uint256 proRegTxHash;
    CBLSSignature sig;

    SERIALIZE_METHODS(CMNAuth, obj)
    {
        READWRITE(obj.proRegTxHash, obj.sig);
    }

    static void PushMNAUTH(CNode& peer, CConnman& connman, const CActiveMasternodeManager& mn_activeman);

    /**
     * @pre CMasternodeMetaMan's database must be successfully loaded before
     *      attempting to call this function regardless of sync state
     */
    static PeerMsgRet ProcessMessage(CNode& peer, ServiceFlags node_services, CConnman& connman, CMasternodeMetaMan& mn_metaman, const CActiveMasternodeManager* const mn_activeman,
                                     const CMasternodeSync& mn_sync, const CDeterministicMNList& tip_mn_list,
                                     std::string_view msg_type, CDataStream& vRecv);
    static void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff, CConnman& connman);
};


#endif // BITCOIN_EVO_MNAUTH_H
