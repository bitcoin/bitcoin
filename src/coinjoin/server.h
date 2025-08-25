// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_SERVER_H
#define BITCOIN_COINJOIN_SERVER_H

#include <coinjoin/coinjoin.h>

#include <net_types.h>
#include <protocol.h>

class CActiveMasternodeManager;
class CCoinJoinServer;
class CConnman;
class CDataStream;
class CDeterministicMNManager;
class CDSTXManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CNode;
class CTxMemPool;
class PeerManager;

class UniValue;

/** Used to keep track of current status of mixing pool
 */
class CCoinJoinServer : public CCoinJoinBaseSession, public CCoinJoinBaseManager
{
private:
    ChainstateManager& m_chainman;
    CConnman& connman;
    CDeterministicMNManager& m_dmnman;
    CDSTXManager& m_dstxman;
    CMasternodeMetaMan& m_mn_metaman;
    CTxMemPool& mempool;
    PeerManager& m_peerman;
    const CActiveMasternodeManager& m_mn_activeman;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;

    // Mixing uses collateral transactions to trust parties entering the pool
    // to behave honestly. If they don't it takes their money.
    std::vector<CTransactionRef> vecSessionCollaterals;

    bool fUnitTest;

    /// Add a clients entry to the pool
    bool AddEntry(const CCoinJoinEntry& entry, PoolMessage& nMessageIDRet) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Add signature to a txin
    bool AddScriptSig(const CTxIn& txin) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees() const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Rarely charge fees to pay miners
    void ChargeRandomFees() const;
    /// Consume collateral in cases when peer misbehaved
    void ConsumeCollateral(const CTransactionRef& txref) const;

    /// Check for process
    void CheckPool();

    void CreateFinalTransaction() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    void CommitFinalTransaction() EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    /// Is this nDenom and txCollateral acceptable?
    bool IsAcceptableDSA(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) const;
    bool CreateNewSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
    bool AddUserToExistingSession(const CCoinJoinAccept& dsa, PoolMessage& nMessageIDRet);
    /// Do we have enough users to take entries?
    bool IsSessionReady() const;

    /// Check that all inputs are signed. (Are all inputs signed?)
    bool IsSignaturesComplete() const EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    /// Check to make sure a given input matches an input in the pool and its scriptSig is valid
    bool IsInputScriptSigValid(const CTxIn& txin) const EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

    // Set the 'state' value, with some logging and capturing when the state changed
    void SetState(PoolState nStateNew);

    /// Relay mixing Messages
    void RelayFinalTransaction(const CTransaction& txFinal) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    void PushStatus(CNode& peer, PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID) const;
    void RelayStatus(PoolStatusUpdate nStatusUpdate, PoolMessage nMessageID = MSG_NOERR) EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);
    void RelayCompletedTransaction(PoolMessage nMessageID) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    void ProcessDSACCEPT(CNode& peer, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
    [[nodiscard]] MessageProcessingResult ProcessDSQUEUE(NodeId from, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_vecqueue);
    void ProcessDSVIN(CNode& peer, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);
    void ProcessDSSIGNFINALTX(CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_coinjoin);

    void SetNull() override EXCLUSIVE_LOCKS_REQUIRED(cs_coinjoin);

public:
    explicit CCoinJoinServer(ChainstateManager& chainman, CConnman& _connman, CDeterministicMNManager& dmnman,
                             CDSTXManager& dstxman, CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                             PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                             const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman) :
        m_chainman(chainman),
        connman(_connman),
        m_dmnman(dmnman),
        m_dstxman(dstxman),
        m_mn_metaman(mn_metaman),
        mempool(mempool),
        m_peerman(peerman),
        m_mn_activeman(mn_activeman),
        m_mn_sync(mn_sync),
        m_isman{isman},
        vecSessionCollaterals(),
        fUnitTest(false)
    {}

    [[nodiscard]] MessageProcessingResult ProcessMessage(CNode& pfrom, std::string_view msg_type, CDataStream& vRecv);

    bool HasTimedOut() const;
    void CheckTimeout();
    void CheckForCompleteQueue();

    void DoMaintenance();

    void GetJsonInfo(UniValue& obj) const;
};

#endif // BITCOIN_COINJOIN_SERVER_H
