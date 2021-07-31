// Copyright (c) 2019-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_MNAUTH_H
#define BITCOIN_EVO_MNAUTH_H

#include <bls/bls.h>
#include <serialize.h>

class CConnman;
class CDataStream;
class CDeterministicMN;
class CDeterministicMNList;
class CDeterministicMNListDiff;
class CNode;
class UniValue;

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

    static void PushMNAUTH(CNode* pnode, CConnman& connman);
    static void ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    static void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff);
};


#endif // BITCOIN_EVO_MNAUTH_H
