// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIVATESENDSERVER_H
#define PRIVATESENDSERVER_H

#include "net.h"
#include "privatesend.h"

class CPrivateSendServer;
class UniValue;

// The main object for accessing mixing
extern CPrivateSendServer privateSendServer;

/** Used to keep track of current status of mixing pool
 */
class CPrivateSendServer : public CPrivateSendBaseSession, public CPrivateSendBaseManager
{
private:
    // Mixing uses collateral transactions to trust parties entering the pool
    // to behave honestly. If they don't it takes their money.
    std::vector<CTransactionRef> vecSessionCollaterals;

    // Maximum number of participants in a certain session, random between min and max.
    int nSessionMaxParticipants;

    bool fUnitTest;

    /// Add a clients entry to the pool
    bool AddEntry(CConnman& connman, const CPrivateSendEntry& entry, PoolMessage& nMessageIDRet);
    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin);

    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees(CConnman& connman);
    /// Rarely charge fees to pay miners
    void ChargeRandomFees(CConnman& connman);
    /// Consume collateral in cases when peer misbehaved
    void ConsumeCollateral(CConnman& connman, const CTransactionRef& txref);

    /// Check for process
    void CheckPool(CConnman& connman);

    void CreateFinalTransaction(CConnman& connman);
    void CommitFinalTransaction(CConnman& connman);

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDSA(const CPrivateSendAccept& dsa, PoolMessage& nMessageIDRet);
    bool CreateNewSession(const CPrivateSendAccept& dsa, PoolMessage& nMessageIDRet, CConnman& connman);
    bool AddUserToExistingSession(const CPrivateSendAccept& dsa, PoolMessage& nMessageIDRet);
    /// Do we have enough users to take entries?
    bool IsSessionReady();

    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete();
    /// Check to make sure a given input matches an input in the pool and its scriptSig is valid
    bool IsInputScriptSigValid(const CTxIn& txin);

    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    /// Relay mixing Messages
    void RelayFinalTransaction(const CTransaction& txFinal, CConnman& connman);
    void PushStatus(CNode* pnode, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID, CConnman& connman);
    void RelayStatus(PoolStatusUpdate nStatusUpdate, CConnman& connman, PoolMessage nMessageID = MSG_NOERR);
    void RelayCompletedTransaction(PoolMessage nMessageID, CConnman& connman);

    void SetNull();

public:
    CPrivateSendServer() :
        vecSessionCollaterals(),
        nSessionMaxParticipants(0),
        fUnitTest(false) {}

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    void CheckTimeout(CConnman& connman);
    void CheckForCompleteQueue(CConnman& connman);

    void DoMaintenance(CConnman& connman);

    void GetJsonInfo(UniValue& obj) const;
};

#endif
