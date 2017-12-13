// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIVATESENDSERVER_H
#define PRIVATESENDSERVER_H

#include "net.h"
#include "privatesend.h"

class CPrivateSendServer;

// The main object for accessing mixing
extern CPrivateSendServer privateSendServer;

/** Used to keep track of current status of mixing pool
 */
class CPrivateSendServer : public CPrivateSendBase
{
private:
    mutable CCriticalSection cs_darksend;

    // Mixing uses collateral transactions to trust parties entering the pool
    // to behave honestly. If they don't it takes their money.
    std::vector<CTransaction> vecSessionCollaterals;

    bool fUnitTest;

    /// Add a clients entry to the pool
    bool AddEntry(const CDarkSendEntry& entryNew, PoolMessage& nMessageIDRet);
    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin);

    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees(CConnman& connman);
    /// Rarely charge fees to pay miners
    void ChargeRandomFees(CConnman& connman);

    /// Check for process
    void CheckPool(CConnman& connman);

    void CreateFinalTransaction(CConnman& connman);
    void CommitFinalTransaction(CConnman& connman);

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDenomAndCollateral(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet);
    bool CreateNewSession(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet, CConnman& connman);
    bool AddUserToExistingSession(int nDenom, CTransaction txCollateral, PoolMessage &nMessageIDRet);
    /// Do we have enough users to take entries?
    bool IsSessionReady() { return (int)vecSessionCollaterals.size() >= CPrivateSend::GetMaxPoolTransactions(); }

    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete();
    /// Check to make sure a given input matches an input in the pool and its scriptSig is valid
    bool IsInputScriptSigValid(const CTxIn& txin);
    /// Are these outputs compatible with other client in the pool?
    bool IsOutputsCompatibleWithSessionDenom(const std::vector<CTxDSOut>& vecTxDSOut);

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
        fUnitTest(false) { SetNull(); }

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    void CheckTimeout(CConnman& connman);
    void CheckForCompleteQueue(CConnman& connman);
};

void ThreadCheckPrivateSendServer(CConnman& connman);

#endif
