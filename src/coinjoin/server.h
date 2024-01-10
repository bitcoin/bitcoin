// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_SERVER_H
#define BITCOIN_COINJOIN_SERVER_H

#include <coinjoin/coinjoin.h>

#include <net_types.h>

class CChainState;
class CCoinJoinServer;
class CDataStream;
class CNode;
class CTxMemPool;

class UniValue;

/** Used to keep track of current status of mixing pool
 */
class CCoinJoinServer : public CCoinJoinBaseSession, public CCoinJoinBaseManager
{
private:
    CChainState& m_chainstate;
    CConnman& connman;
    CTxMemPool& mempool;
    const CMasternodeSync& m_mn_sync;

    // Mixing uses collateral transactions to trust parties entering the pool
    // to behave honestly. If they don't it takes their money.
    std::vector<CTransactionRef> vecSessionCollaterals;

    bool fUnitTest;

    /// Add a clients entry to the pool
    bool AddEntry(const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet) LOCKS_EXCLUDED(cs_coinjoin);
    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin) LOCKS_EXCLUDED(cs_coinjoin);

    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees() const LOCKS_EXCLUDED(cs_coinjoin);
    /// Rarely charge fees to pay miners
    void ChargeRandomFees() const;
    /// Consume collateral in cases when peer misbehaved
    void ConsumeCollateral(const CTransactionRef& txref) const;

    /// Check for process
    void CheckPool();

    void CreateFinalTransaction() LOCKS_EXCLUDED(cs_coinjoin);
    void CommitFinalTransaction() LOCKS_EXCLUDED(cs_coinjoin);

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDSA(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) const;
    bool CreateNewSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) LOCKS_EXCLUDED(cs_vecqueue);
    bool AddUserToExistingSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet);
    /// Do we have enough users to take entries?
    bool IsSessionReady() const;

    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete() const LOCKS_EXCLUDED(cs_coinjoin);
    /// Check to make sure a given input matches an input in the pool and its scriptSig is valid
    bool IsInputScriptSigValid(const CTxIn& txin) const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    /// Relay mixing Messages
    void RelayFinalTransaction(const CTransaction& txFinal) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    void PushStatus(CNode& peer, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) const;
    void RelayStatus(PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID = MSG_NOERR) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    void RelayCompletedTransaction(PoolMessage nMessageID) LOCKS_EXCLUDED(cs_coinjoin);

    void ProcessDSACCEPT(CNode& peer, CDataStream& vRecv) LOCKS_EXCLUDED(cs_vecqueue);
    PeerMsgRet ProcessDSQUEUE(const CNode& peer, CDataStream& vRecv) LOCKS_EXCLUDED(cs_vecqueue);
    void ProcessDSVIN(CNode& peer, CDataStream& vRecv) LOCKS_EXCLUDED(cs_coinjoin);
    void ProcessDSSIGNFINALTX(CDataStream& vRecv) LOCKS_EXCLUDED(cs_coinjoin);

    void SetNull() EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

public:
    explicit CCoinJoinServer(CChainState& chainstate, CConnman& _connman, CTxMemPool& mempool, const CMasternodeSync& mn_sync) :
        m_chainstate(chainstate),
        connman(_connman),
        mempool(mempool),
        m_mn_sync(mn_sync),
        vecSessionCollaterals(),
        fUnitTest(false)
    {}

    PeerMsgRet ProcessMessage(CNode& pfrom, std::string_view msg_type, CDataStream& vRecv);

    bool HasTimedOut() const;
    void CheckTimeout();
    void CheckForCompleteQueue();

    void DoMaintenance();

    void GetJsonInfo(UniValue& obj) const;
};

#endif // BITCOIN_COINJOIN_SERVER_H
