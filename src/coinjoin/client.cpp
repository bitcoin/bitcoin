// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/client.h>

#include <chain.h>
#include <chainparams.h>
#include <coinjoin/options.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <shutdown.h>
#include <util/check.h>
#include <util/irange.h>
#include <util/moneystr.h>
#include <util/ranges.h>
#include <util/system.h>
#include <util/translation.h>
#include <version.h>
#include <wallet/coincontrol.h>
#include <wallet/coinjoin.h>
#include <wallet/coinselection.h>
#include <wallet/receive.h>
#include <wallet/spend.h>

#include <memory>
#include <univalue.h>

using wallet::CCoinControl;
using wallet::CompactTallyItem;
using wallet::COutput;
using wallet::CoinType;
using wallet::CWallet;
using wallet::ReserveDestination;

MessageProcessingResult CCoinJoinClientQueueManager::ProcessMessage(NodeId from, CConnman& connman, PeerManager& peerman,
                                                                    std::string_view msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::DSQUEUE) {
        return {};
    }

    if (m_is_masternode) return {};
    if (!m_mn_sync.IsBlockchainSynced()) return {};

    assert(m_mn_metaman.IsValid());

    CCoinJoinQueue dsq;
    vRecv >> dsq;

    MessageProcessingResult ret{};
    ret.m_to_erase = CInv{MSG_DSQ, dsq.GetHash()};

    if (dsq.masternodeOutpoint.IsNull() && dsq.m_protxHash.IsNull()) {
        ret.m_error = MisbehavingError{100};
        return ret;
    }

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    if (dsq.masternodeOutpoint.IsNull()) {
        if (auto dmn = tip_mn_list.GetValidMN(dsq.m_protxHash)) {
            dsq.masternodeOutpoint = dmn->collateralOutpoint;
        } else {
            ret.m_error = MisbehavingError{10};
            return ret;
        }
    }

    {
        LOCK(cs_ProcessDSQueue);

        {
            LOCK(cs_vecqueue);
            // process every dsq only once
            for (const auto &q: vecCoinJoinQueue) {
                if (q == dsq) {
                    return ret;
                }
                if (q.fReady == dsq.fReady && q.masternodeOutpoint == dsq.masternodeOutpoint) {
                    // no way the same mn can send another dsq with the same readiness this soon
                    LogPrint(BCLog::COINJOIN, /* Continued */
                             "DSQUEUE -- Peer %d is sending WAY too many dsq messages for a masternode with collateral %s\n",
                             from, dsq.masternodeOutpoint.ToStringShort());
                    return ret;
                }
            }
        } // cs_vecqueue

        LogPrint(BCLog::COINJOIN, "DSQUEUE -- %s new\n", dsq.ToString());

        if (dsq.IsTimeOutOfBounds()) return ret;

        auto dmn = tip_mn_list.GetValidMNByCollateral(dsq.masternodeOutpoint);
        if (!dmn) return ret;

        if (dsq.m_protxHash.IsNull()) {
            dsq.m_protxHash = dmn->proTxHash;
        }

        if (!dsq.CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
            ret.m_error = MisbehavingError{10};
            return ret;
        }

        // if the queue is ready, submit if we can
        if (dsq.fReady &&
            m_walletman.ForAnyCJClientMan([&connman, &dmn](std::unique_ptr<CCoinJoinClientManager>& clientman) {
                return clientman->TrySubmitDenominate(dmn->proTxHash, connman);
            })) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- CoinJoin queue is ready, masternode=%s, queue=%s\n", dmn->proTxHash.ToString(), dsq.ToString());
            return ret;
        } else {
            int64_t nLastDsq = m_mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
            int64_t nDsqThreshold = m_mn_metaman.GetDsqThreshold(dmn->proTxHash, tip_mn_list.GetValidMNsCount());
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- nLastDsq: %d  nDsqThreshold: %d  nDsqCount: %d\n", nLastDsq,
                     nDsqThreshold, m_mn_metaman.GetDsqCount());
            // don't allow a few nodes to dominate the queuing process
            if (nLastDsq != 0 && nDsqThreshold > m_mn_metaman.GetDsqCount()) {
                LogPrint(BCLog::COINJOIN, "DSQUEUE -- Masternode %s is sending too many dsq messages\n",
                         dmn->proTxHash.ToString());
                return ret;
            }

            m_mn_metaman.AllowMixing(dmn->proTxHash);

            LogPrint(BCLog::COINJOIN, "DSQUEUE -- new CoinJoin queue, masternode=%s, queue=%s\n", dmn->proTxHash.ToString(), dsq.ToString());

            m_walletman.ForAnyCJClientMan([&dsq](const std::unique_ptr<CCoinJoinClientManager>& clientman) {
                return clientman->MarkAlreadyJoinedQueueAsTried(dsq);
            });

            WITH_LOCK(cs_vecqueue, vecCoinJoinQueue.push_back(dsq));
        }
    } // cs_ProcessDSQueue
    peerman.RelayDSQ(dsq);
    return ret;
}

void CCoinJoinClientManager::ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv)
{
    if (m_is_masternode) return;
    if (!CCoinJoinClientOptions::IsEnabled()) return;
    if (!m_mn_sync.IsBlockchainSynced()) return;

    if (!CheckDiskSpace(gArgs.GetDataDirNet())) {
        ResetPool();
        StopMixing();
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::ProcessMessage -- Not enough disk space, disabling CoinJoin.\n");
        return;
    }

    if (msg_type == NetMsgType::DSSTATUSUPDATE ||
               msg_type == NetMsgType::DSFINALTX ||
               msg_type == NetMsgType::DSCOMPLETE) {
        AssertLockNotHeld(cs_deqsessions);
        LOCK(cs_deqsessions);
        for (auto& session : deqSessions) {
            session.ProcessMessage(peer, active_chainstate, connman, mempool, msg_type, vRecv);
        }
    }
}

CCoinJoinClientSession::CCoinJoinClientSession(const std::shared_ptr<CWallet>& wallet, CCoinJoinClientManager& clientman,
                                               CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                                               const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                               const std::unique_ptr<CCoinJoinClientQueueManager>& queueman,
                                               bool is_masternode) :
    m_wallet(wallet),
    m_clientman(clientman),
    m_dmnman(dmnman),
    m_mn_metaman(mn_metaman),
    m_mn_sync(mn_sync),
    m_isman{isman},
    m_queueman(queueman),
    m_is_masternode{is_masternode}
{}

void CCoinJoinClientSession::ProcessMessage(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv)
{
    if (m_is_masternode) return;
    if (!CCoinJoinClientOptions::IsEnabled()) return;
    if (!m_mn_sync.IsBlockchainSynced()) return;

    if (!mixingMasternode) return;
    if (mixingMasternode->pdmnState->netInfo->GetPrimary() != peer.addr) return;

    if (msg_type == NetMsgType::DSSTATUSUPDATE) {
        CCoinJoinStatusUpdate psssup;
        vRecv >> psssup;

        ProcessPoolStateUpdate(psssup);
    } else if (msg_type == NetMsgType::DSFINALTX) {
        int nMsgSessionID;
        vRecv >> nMsgSessionID;
        CTransaction txNew(deserialize, vRecv);

        if (nSessionID != nMsgSessionID) {
            WalletCJLogPrint(m_wallet, "DSFINALTX -- message doesn't match current CoinJoin session: nSessionID: %d  nMsgSessionID: %d\n", nSessionID.load(), nMsgSessionID);
            return;
        }

        WalletCJLogPrint(m_wallet, "DSFINALTX -- txNew %s", txNew.ToString()); /* Continued */

        // check to see if input is spent already? (and probably not confirmed)
        SignFinalTransaction(peer, active_chainstate, connman, mempool, txNew);
    } else if (msg_type == NetMsgType::DSCOMPLETE) {
        int nMsgSessionID;
        PoolMessage nMsgMessageID;
        vRecv >> nMsgSessionID >> nMsgMessageID;

        if (nMsgMessageID < MSG_POOL_MIN || nMsgMessageID > MSG_POOL_MAX) {
            WalletCJLogPrint(m_wallet, "DSCOMPLETE -- nMsgMessageID is out of bounds: %d\n", nMsgMessageID);
            return;
        }

        if (nSessionID != nMsgSessionID) {
            WalletCJLogPrint(m_wallet, "DSCOMPLETE -- message doesn't match current CoinJoin session: nSessionID: %d  nMsgSessionID: %d\n", nSessionID.load(), nMsgSessionID);
            return;
        }

        WalletCJLogPrint(m_wallet, "DSCOMPLETE -- nMsgSessionID %d  nMsgMessageID %d (%s)\n", nMsgSessionID, nMsgMessageID, CoinJoin::GetMessageByID(nMsgMessageID).translated);

        CompletedTransaction(nMsgMessageID);
    }
}

bool CCoinJoinClientManager::StartMixing() {
    bool expected{false};
    return fMixing.compare_exchange_strong(expected, true);
}

void CCoinJoinClientManager::StopMixing() {
    fMixing = false;
}

bool CCoinJoinClientManager::IsMixing() const
{
    return fMixing;
}

void CCoinJoinClientSession::ResetPool()
{
    txMyCollateral = CMutableTransaction();
    UnlockCoins();
    WITH_LOCK(m_wallet->cs_wallet, keyHolderStorage.ReturnAll());
    WITH_LOCK(cs_coinjoin, SetNull());
}

void CCoinJoinClientManager::ResetPool()
{
    nCachedLastSuccessBlock = 0;
    vecMasternodesUsed.clear();
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        session.ResetPool();
    }
    deqSessions.clear();
}

void CCoinJoinClientSession::SetNull()
{
    AssertLockHeld(cs_coinjoin);
    // Client side
    mixingMasternode = nullptr;
    pendingDsaRequest = CPendingDsaRequest();

    CCoinJoinBaseSession::SetNull();
}

//
// Unlock coins after mixing fails or succeeds
//
void CCoinJoinClientSession::UnlockCoins()
{
    if (!CCoinJoinClientOptions::IsEnabled()) return;

    while (true) {
        TRY_LOCK(m_wallet->cs_wallet, lockWallet);
        if (!lockWallet) {
            UninterruptibleSleep(std::chrono::milliseconds{50});
            continue;
        }
        for (const auto& outpoint : vecOutPointLocked)
            m_wallet->UnlockCoin(outpoint);
        break;
    }

    vecOutPointLocked.clear();
}

bilingual_str CCoinJoinClientSession::GetStatus(bool fWaitForBlock) const
{
    static int nStatusMessageProgress = 0;
    nStatusMessageProgress += 10;
    std::string strSuffix;

    if (fWaitForBlock || !m_mn_sync.IsBlockchainSynced()) {
        return strAutoDenomResult;
    }

    switch (nState) {
    case POOL_STATE_IDLE:
        return strprintf(_("%s is idle."), gCoinJoinName);
    case POOL_STATE_QUEUE:
        if (nStatusMessageProgress % 70 <= 30)
            strSuffix = ".";
        else if (nStatusMessageProgress % 70 <= 50)
            strSuffix = "..";
        else
            strSuffix = "...";
        return strprintf(_("Submitted to masternode, waiting in queue %s"), strSuffix);
    case POOL_STATE_ACCEPTING_ENTRIES:
        return strAutoDenomResult;
    case POOL_STATE_SIGNING:
        if (nStatusMessageProgress % 70 <= 40)
            return _("Found enough users, signing…");
        else if (nStatusMessageProgress % 70 <= 50)
            strSuffix = ".";
        else if (nStatusMessageProgress % 70 <= 60)
            strSuffix = "..";
        else
            strSuffix = "...";
        return strprintf(_("Found enough users, signing ( waiting %s )"), strSuffix);
    case POOL_STATE_ERROR:
        return strprintf(_("%s request incomplete:"), gCoinJoinName) + strLastMessage + Untranslated(" ") + _("Will retry…");
    default:
        return strprintf(_("Unknown state: id = %u"), nState);
    }
}

std::vector<std::string> CCoinJoinClientManager::GetStatuses() const
{
    AssertLockNotHeld(cs_deqsessions);

    bool fWaitForBlock{WaitForAnotherBlock()};
    std::vector<std::string> ret;

    LOCK(cs_deqsessions);
    for (const auto& session : deqSessions) {
        ret.push_back(session.GetStatus(fWaitForBlock).original);
    }
    return ret;
}

std::string CCoinJoinClientManager::GetSessionDenoms()
{
    std::string strSessionDenoms;

    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (const auto& session : deqSessions) {
        strSessionDenoms += CoinJoin::DenominationToString(session.nSessionDenom);
        strSessionDenoms += "; ";
    }
    return strSessionDenoms.empty() ? "N/A" : strSessionDenoms;
}

bool CCoinJoinClientSession::GetMixingMasternodeInfo(CDeterministicMNCPtr& ret) const
{
    ret = mixingMasternode;
    return ret != nullptr;
}

bool CCoinJoinClientManager::GetMixingMasternodesInfo(std::vector<CDeterministicMNCPtr>& vecDmnsRet) const
{
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (const auto& session : deqSessions) {
        CDeterministicMNCPtr dmn;
        if (session.GetMixingMasternodeInfo(dmn)) {
            vecDmnsRet.push_back(dmn);
        }
    }
    return !vecDmnsRet.empty();
}

//
// Check session timeouts
//
bool CCoinJoinClientSession::CheckTimeout()
{
    if (m_is_masternode) return false;

    if (nState == POOL_STATE_IDLE) return false;

    if (nState == POOL_STATE_ERROR) {
        if (GetTime() - nTimeLastSuccessfulStep >= 10) {
            // reset after being in POOL_STATE_ERROR for 10 or more seconds
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- resetting session %d\n", __func__, nSessionID.load());
            WITH_LOCK(cs_coinjoin, SetNull());
        }
        return false;
    }

    int nLagTime = 10; // give the server a few extra seconds before resetting.
    int nTimeout = (nState == POOL_STATE_SIGNING) ? COINJOIN_SIGNING_TIMEOUT : COINJOIN_QUEUE_TIMEOUT;
    bool fTimeout = GetTime() - nTimeLastSuccessfulStep >= nTimeout + nLagTime;

    if (!fTimeout) return false;

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- %s %d timed out (%ds)\n", __func__,
        (nState == POOL_STATE_SIGNING) ? "Signing at session" : "Session", nSessionID.load(), nTimeout);

    SetState(POOL_STATE_ERROR);
    UnlockCoins();
    WITH_LOCK(m_wallet->cs_wallet, keyHolderStorage.ReturnAll());
    nTimeLastSuccessfulStep = GetTime();
    strLastMessage = CoinJoin::GetMessageByID(ERR_SESSION);

    return true;
}

//
// Check all queues and sessions for timeouts
//
void CCoinJoinClientManager::CheckTimeout()
{
    AssertLockNotHeld(cs_deqsessions);
    if (m_is_masternode) return;

    if (!CCoinJoinClientOptions::IsEnabled() || !IsMixing()) return;

    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        if (session.CheckTimeout()) {
            strAutoDenomResult = _("Session timed out.");
        }
    }
}

//
// Execute a mixing denomination via a Masternode.
// This is only ran from clients
//
bool CCoinJoinClientSession::SendDenominate(const std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsIn, CConnman& connman)
{
    if (m_is_masternode) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- CoinJoin from a Masternode is not supported currently.\n");
        return false;
    }

    if (CTransaction(txMyCollateral).IsNull()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClient:SendDenominate -- CoinJoin collateral not set\n");
        return false;
    }

    // we should already be connected to a Masternode
    if (!nSessionID) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- No Masternode has been selected yet.\n");
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        WITH_LOCK(cs_coinjoin, SetNull());
        return false;
    }

    if (!CheckDiskSpace(gArgs.GetDataDirNet())) {
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        WITH_LOCK(cs_coinjoin, SetNull());
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- Not enough disk space.\n");
        return false;
    }

    SetState(POOL_STATE_ACCEPTING_ENTRIES);
    strLastMessage = Untranslated("");

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- Added transaction to pool.\n");

    CMutableTransaction tx; // for debug purposes only
    std::vector<CTxDSIn> vecTxDSInTmp;
    std::vector<CTxOut> vecTxOutTmp;

    for (const auto& [txDsIn, txOut] : vecPSInOutPairsIn) {
        vecTxDSInTmp.emplace_back(txDsIn);
        vecTxOutTmp.emplace_back(txOut);
        tx.vin.emplace_back(txDsIn);
        tx.vout.emplace_back(txOut);
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- Submitting partial tx %s", tx.ToString()); /* Continued */

    // store our entry for later use
    LOCK(cs_coinjoin);
    vecEntries.emplace_back(vecTxDSInTmp, vecTxOutTmp, CTransaction(txMyCollateral));
    RelayIn(vecEntries.back(), connman);
    nTimeLastSuccessfulStep = GetTime();

    return true;
}

// Process incoming messages from Masternode updating the progress of mixing
void CCoinJoinClientSession::ProcessPoolStateUpdate(CCoinJoinStatusUpdate psssup)
{
    if (m_is_masternode) return;

    // do not update state when mixing client state is one of these
    if (nState == POOL_STATE_IDLE || nState == POOL_STATE_ERROR) return;

    if (psssup.nState < POOL_STATE_MIN || psssup.nState > POOL_STATE_MAX) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- psssup.nState is out of bounds: %d\n", __func__, psssup.nState);
        return;
    }

    if (psssup.nMessageID < MSG_POOL_MIN || psssup.nMessageID > MSG_POOL_MAX) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- psssup.nMessageID is out of bounds: %d\n", __func__, psssup.nMessageID);
        return;
    }

    bilingual_str strMessageTmp = CoinJoin::GetMessageByID(psssup.nMessageID);
    strAutoDenomResult = _("Masternode:") + Untranslated(" ") + strMessageTmp;

    switch (psssup.nStatusUpdate) {
        case STATUS_REJECTED: {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- rejected by Masternode: %s\n", __func__, strMessageTmp.translated);
            SetState(POOL_STATE_ERROR);
            UnlockCoins();
            WITH_LOCK(m_wallet->cs_wallet, keyHolderStorage.ReturnAll());
            nTimeLastSuccessfulStep = GetTime();
            strLastMessage = strMessageTmp;
            break;
        }
        case STATUS_ACCEPTED: {
            if (nState == psssup.nState && psssup.nState == POOL_STATE_QUEUE && nSessionID == 0 && psssup.nSessionID != 0) {
                // new session id should be set only in POOL_STATE_QUEUE state
                nSessionID = psssup.nSessionID;
                nTimeLastSuccessfulStep = GetTime();
                strMessageTmp = strMessageTmp + strprintf(Untranslated(" Set nSessionID to %d."), nSessionID);
            }
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- accepted by Masternode: %s\n", __func__, strMessageTmp.translated);
            break;
        }
        default: {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- psssup.nStatusUpdate is out of bounds: %d\n", __func__, psssup.nStatusUpdate);
            break;
        }
    }
}

//
// After we receive the finalized transaction from the Masternode, we must
// check it to make sure it's what we want, then sign it if we agree.
// If we refuse to sign, it's possible we'll be charged collateral
//
bool CCoinJoinClientSession::SignFinalTransaction(CNode& peer, CChainState& active_chainstate, CConnman& connman, const CTxMemPool& mempool, const CTransaction& finalTransactionNew)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    if (m_is_masternode) return false;
    if (!mixingMasternode) return false;

    LOCK(m_wallet->cs_wallet);
    LOCK(cs_coinjoin);

    finalMutableTransaction = CMutableTransaction{finalTransactionNew};
    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- finalMutableTransaction=%s", __func__, finalMutableTransaction.ToString()); /* Continued */

    // STEP 1: check final transaction general rules

    // Make sure it's BIP69 compliant
    sort(finalMutableTransaction.vin.begin(), finalMutableTransaction.vin.end(), CompareInputBIP69());
    sort(finalMutableTransaction.vout.begin(), finalMutableTransaction.vout.end(), CompareOutputBIP69());

    if (finalMutableTransaction.GetHash() != finalTransactionNew.GetHash()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- ERROR! Masternode %s is not BIP69 compliant!\n", __func__, mixingMasternode->proTxHash.ToString());
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        SetNull();
        return false;
    }

    // Make sure all inputs/outputs are valid
    PoolMessage nMessageID{MSG_NOERR};
    if (!IsValidInOuts(active_chainstate, m_isman, mempool, finalMutableTransaction.vin, finalMutableTransaction.vout,
                       nMessageID, nullptr)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- ERROR! IsValidInOuts() failed: %s\n", __func__, CoinJoin::GetMessageByID(nMessageID).translated);
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        SetNull();
        return false;
    }

    // STEP 2: make sure our own inputs/outputs are present, otherwise refuse to sign

    std::map<COutPoint, Coin> coins;

    for (const auto &entry: vecEntries) {
        // Check that the final transaction has all our outputs
        for (const auto &txout: entry.vecTxOut) {
            bool fFound = ranges::any_of(finalMutableTransaction.vout, [&txout](const auto& txoutFinal){
                return txoutFinal == txout;
            });
            if (!fFound) {
                // Something went wrong and we'll refuse to sign. It's possible we'll be charged collateral. But that's
                // better than signing if the transaction doesn't look like what we wanted.
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- an output is missing, refusing to sign! txout=%s\n", __func__, txout.ToString());
                UnlockCoins();
                keyHolderStorage.ReturnAll();
                SetNull();
                return false;
            }
        }

        for (const auto& txdsin : entry.vecTxDSIn) {
            /* Sign my transaction and all outputs */
            int nMyInputIndex = -1;
            CScript prevPubKey = CScript();

            for (const auto i : irange::range(finalMutableTransaction.vin.size())) {
                // cppcheck-suppress useStlAlgorithm
                if (finalMutableTransaction.vin[i] == txdsin) {
                    nMyInputIndex = i;
                    prevPubKey = txdsin.prevPubKey;
                    break;
                }
            }

            if (nMyInputIndex == -1) {
                // Can't find one of my own inputs, refuse to sign. It's possible we'll be charged collateral. But that's
                // better than signing if the transaction doesn't look like what we wanted.
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- missing input! txdsin=%s\n", __func__, txdsin.ToString());
                UnlockCoins();
                keyHolderStorage.ReturnAll();
                SetNull();
                return false;
            }

            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- found my input %i\n", __func__, nMyInputIndex);
            // add a pair with an empty value
            coins[finalMutableTransaction.vin.at(nMyInputIndex).prevout];
        }
    }

    // fill values for found outpoints
    m_wallet->chain().findCoins(coins);
    std::map<int, bilingual_str> signing_errors;
    m_wallet->SignTransaction(finalMutableTransaction, coins, SIGHASH_ALL | SIGHASH_ANYONECANPAY, signing_errors);

    for (const auto& [input_index, error_string] : signing_errors) {
        // NOTE: this is a partial signing so it's expected for SignTransaction to return
        // "Input not found or already spent" errors for inputs that aren't ours
        if (error_string.original != "Input not found or already spent") {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- signing input %d failed: %s!\n", __func__, input_index, error_string.original);
            UnlockCoins();
            keyHolderStorage.ReturnAll();
            SetNull();
            return false;
        }
    }

    std::vector<CTxIn> signed_inputs;
    for (const auto& txin : finalMutableTransaction.vin) {
        if (coins.find(txin.prevout) != coins.end()) {
            signed_inputs.push_back(txin);
        }
    }

    if (signed_inputs.empty()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- can't sign anything!\n", __func__);
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        SetNull();
        return false;
    }

    // push all of our signatures to the Masternode
    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- pushing signed inputs to the masternode, finalMutableTransaction=%s", __func__, finalMutableTransaction.ToString()); /* Continued */
    CNetMsgMaker msgMaker(peer.GetCommonVersion());
    connman.PushMessage(&peer, msgMaker.Make(NetMsgType::DSSIGNFINALTX, signed_inputs));
    SetState(POOL_STATE_SIGNING);
    nTimeLastSuccessfulStep = GetTime();

    return true;
}

// mixing transaction was completed (failed or successful)
void CCoinJoinClientSession::CompletedTransaction(PoolMessage nMessageID)
{
    if (m_is_masternode) return;

    if (nMessageID == MSG_SUCCESS) {
        m_clientman.UpdatedSuccessBlock();
        keyHolderStorage.KeepAll();
        WalletCJLogPrint(m_wallet, "CompletedTransaction -- success\n");
    } else {
        WITH_LOCK(m_wallet->cs_wallet, keyHolderStorage.ReturnAll());
        WalletCJLogPrint(m_wallet, "CompletedTransaction -- error\n");
    }
    UnlockCoins();
    WITH_LOCK(cs_coinjoin, SetNull());
    strLastMessage = CoinJoin::GetMessageByID(nMessageID);
}

void CCoinJoinClientManager::UpdatedSuccessBlock()
{
    if (m_is_masternode) return;
    nCachedLastSuccessBlock = nCachedBlockHeight;
}

bool CCoinJoinClientManager::WaitForAnotherBlock() const
{
    if (!m_mn_sync.IsBlockchainSynced()) return true;

    if (CCoinJoinClientOptions::IsMultiSessionEnabled()) return false;

    return nCachedBlockHeight - nCachedLastSuccessBlock < nMinBlocksToWait;
}

bool CCoinJoinClientManager::CheckAutomaticBackup()
{
    if (!CCoinJoinClientOptions::IsEnabled() || !IsMixing()) return false;

    // We don't need auto-backups for descriptor wallets
    if (!m_wallet->IsLegacy()) return true;

    switch (nWalletBackups) {
    case 0:
        strAutoDenomResult = _("Automatic backups disabled") + Untranslated(", ") + _("no mixing available.");
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);
        StopMixing();
        m_wallet->nKeysLeftSinceAutoBackup = 0; // no backup, no "keys since last backup"
        return false;
    case -1:
        // Automatic backup failed, nothing else we can do until user fixes the issue manually.
        // There is no way to bring user attention in daemon mode, so we just update status and
        // keep spamming if debug is on.
        strAutoDenomResult = _("ERROR! Failed to create automatic backup") + Untranslated(", ") + _("see debug.log for details.");
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);
        return false;
    case -2:
        // We were able to create automatic backup but keypool was not replenished because wallet is locked.
        // There is no way to bring user attention in daemon mode, so we just update status and
        // keep spamming if debug is on.
        strAutoDenomResult = _("WARNING! Failed to replenish keypool, please unlock your wallet to do so.") + Untranslated(", ") + _("see debug.log for details.");
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);
        return false;
    }

    if (m_wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_STOP) {
        // We should never get here via mixing itself but probably something else is still actively using keypool
        strAutoDenomResult = strprintf(_("Very low number of keys left: %d") + Untranslated(", ") +
                                           _("no mixing available."),
                                       m_wallet->nKeysLeftSinceAutoBackup);
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);
        // It's getting really dangerous, stop mixing
        StopMixing();
        return false;
    } else if (m_wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_WARNING) {
        // Low number of keys left, but it's still more or less safe to continue
        strAutoDenomResult = strprintf(_("Very low number of keys left: %d"), m_wallet->nKeysLeftSinceAutoBackup);
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);

        if (fCreateAutoBackups) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- Trying to create new backup.\n");
            bilingual_str errorString;
            std::vector<bilingual_str> warnings;

            if (!m_wallet->AutoBackupWallet("", errorString, warnings)) {
                if (!warnings.empty()) {
                    // There were some issues saving backup but yet more or less safe to continue
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- WARNING! Something went wrong on automatic backup: %s\n", Join(warnings, Untranslated("\n")).translated);
                }
                if (!errorString.original.empty()) {
                    // Things are really broken
                    strAutoDenomResult = _("ERROR! Failed to create automatic backup") + Untranslated(": ") + errorString;
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- %s\n", strAutoDenomResult.original);
                    return false;
                }
            }
        } else {
            // Wait for something else (e.g. GUI action) to create automatic backup for us
            return false;
        }
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- Keys left since latest backup: %d\n",
                     m_wallet->nKeysLeftSinceAutoBackup);

    return true;
}

//
// Passively run mixing in the background to mix funds based on the given configuration.
//
bool CCoinJoinClientSession::DoAutomaticDenominating(ChainstateManager& chainman, CConnman& connman,
                                                     const CTxMemPool& mempool, bool fDryRun)
{
    if (m_is_masternode) return false; // no client-side mixing on masternodes
    if (nState != POOL_STATE_IDLE) return false;

    if (!m_mn_sync.IsBlockchainSynced()) {
        strAutoDenomResult = _("Can't mix while sync in progress.");
        return false;
    }

    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    CAmount nBalanceNeedsAnonymized;

    {
        LOCK(m_wallet->cs_wallet);

        if (!fDryRun && m_wallet->IsLocked(true)) {
            strAutoDenomResult = _("Wallet is locked.");
            return false;
        }

        if (GetEntriesCount() > 0) {
            strAutoDenomResult = _("Mixing in progress…");
            return false;
        }

        TRY_LOCK(cs_coinjoin, lockDS);
        if (!lockDS) {
            strAutoDenomResult = _("Lock is already in place.");
            return false;
        }

        if (m_dmnman.GetListAtChainTip().GetValidMNsCount() == 0 &&
            Params().NetworkIDString() != CBaseChainParams::REGTEST) {
            strAutoDenomResult = _("No Masternodes detected.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- %s\n", strAutoDenomResult.original);
            return false;
        }

        const auto bal = GetBalance(*m_wallet);

        // check if there is anything left to do
        CAmount nBalanceAnonymized = bal.m_anonymized;
        nBalanceNeedsAnonymized = CCoinJoinClientOptions::GetAmount() * COIN - nBalanceAnonymized;

        if (nBalanceNeedsAnonymized < 0) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- Nothing to do\n");
            // nothing to do, just keep it in idle mode
            return false;
        }

        CAmount nValueMin = CoinJoin::GetSmallestDenomination();

        // if there are no confirmed DS collateral inputs yet
        if (!m_wallet->HasCollateralInputs()) {
            // should have some additional amount for them
            nValueMin += CoinJoin::GetMaxCollateralAmount();
        }

        // including denoms but applying some restrictions
        CAmount nBalanceAnonymizable = m_wallet->GetAnonymizableBalance();

        // mixable balance is way too small
        if (nBalanceAnonymizable < nValueMin) {
            strAutoDenomResult = _("Not enough funds to mix.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- %s\n", strAutoDenomResult.original);
            return false;
        }

        // excluding denoms
        CAmount nBalanceAnonimizableNonDenom = m_wallet->GetAnonymizableBalance(true);
        // denoms
        CAmount nBalanceDenominatedConf = bal.m_denominated_trusted;
        CAmount nBalanceDenominatedUnconf = bal.m_denominated_untrusted_pending;
        CAmount nBalanceDenominated = nBalanceDenominatedConf + nBalanceDenominatedUnconf;
        CAmount nBalanceToDenominate = CCoinJoinClientOptions::GetAmount() * COIN - nBalanceDenominated;

        // adjust nBalanceNeedsAnonymized to consume final denom
        if (nBalanceDenominated - nBalanceAnonymized > nBalanceNeedsAnonymized) {
            auto denoms = CoinJoin::GetStandardDenominations();
            CAmount nAdditionalDenom{0};
            for (const auto& denom : denoms) {
                if (nBalanceNeedsAnonymized < denom) {
                    nAdditionalDenom = denom;
                } else {
                    break;
                }
            }
            nBalanceNeedsAnonymized += nAdditionalDenom;
        }

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- current stats:\n"
            "    nValueMin: %s\n"
            "    nBalanceAnonymizable: %s\n"
            "    nBalanceAnonymized: %s\n"
            "    nBalanceNeedsAnonymized: %s\n"
            "    nBalanceAnonimizableNonDenom: %s\n"
            "    nBalanceDenominatedConf: %s\n"
            "    nBalanceDenominatedUnconf: %s\n"
            "    nBalanceDenominated: %s\n"
            "    nBalanceToDenominate: %s\n",
            FormatMoney(nValueMin),
            FormatMoney(nBalanceAnonymizable),
            FormatMoney(nBalanceAnonymized),
            FormatMoney(nBalanceNeedsAnonymized),
            FormatMoney(nBalanceAnonimizableNonDenom),
            FormatMoney(nBalanceDenominatedConf),
            FormatMoney(nBalanceDenominatedUnconf),
            FormatMoney(nBalanceDenominated),
            FormatMoney(nBalanceToDenominate)
            );

        if (fDryRun) return true;

        // Check if we have should create more denominated inputs i.e.
        // there are funds to denominate and denominated balance does not exceed
        // max amount to mix yet.
        if (nBalanceAnonimizableNonDenom >= nValueMin + CoinJoin::GetCollateralAmount() && nBalanceToDenominate > 0) {
            CreateDenominated(nBalanceToDenominate);
        }

        //check if we have the collateral sized inputs
        if (!m_wallet->HasCollateralInputs()) {
            return !m_wallet->HasCollateralInputs(false) && MakeCollateralAmounts();
        }

        if (nSessionID) {
            strAutoDenomResult = _("Mixing in progress…");
            return false;
        }

        // Initial phase, find a Masternode
        // Clean if there is anything left from previous session
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        SetNull();

        // should be no unconfirmed denoms in non-multi-session mode
        if (!CCoinJoinClientOptions::IsMultiSessionEnabled() && nBalanceDenominatedUnconf > 0) {
            strAutoDenomResult = _("Found unconfirmed denominated outputs, will wait till they confirm to continue.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- %s\n", strAutoDenomResult.original);
            return false;
        }

        //check our collateral and create new if needed
        std::string strReason;
        if (CTransaction(txMyCollateral).IsNull()) {
            if (!CreateCollateralTransaction(txMyCollateral, strReason)) {
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- create collateral error:%s\n", strReason);
                return false;
            }
        } else {
            if (!CoinJoin::IsCollateralValid(chainman, m_isman, mempool, CTransaction(txMyCollateral))) {
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- invalid collateral, recreating...\n");
                if (!CreateCollateralTransaction(txMyCollateral, strReason)) {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- create collateral error: %s\n", strReason);
                    return false;
                }
            }
        }
        // lock the funds we're going to use for our collateral
        for (const auto& txin : txMyCollateral.vin) {
            m_wallet->LockCoin(txin.prevout);
            vecOutPointLocked.push_back(txin.prevout);
        }
    } // LOCK(m_wallet->cs_wallet);

    // Always attempt to join an existing queue
    if (JoinExistingQueue(nBalanceNeedsAnonymized, connman)) {
        return true;
    }

    // If we were unable to find/join an existing queue then start a new one.
    if (StartNewQueue(nBalanceNeedsAnonymized, connman)) return true;

    strAutoDenomResult = _("No compatible Masternode found.");
    return false;
}

bool CCoinJoinClientManager::DoAutomaticDenominating(ChainstateManager& chainman, CConnman& connman,
                                                     const CTxMemPool& mempool, bool fDryRun)
{
    if (m_is_masternode) return false; // no client-side mixing on masternodes
    if (!CCoinJoinClientOptions::IsEnabled() || !IsMixing()) return false;

    if (!m_mn_sync.IsBlockchainSynced()) {
        strAutoDenomResult = _("Can't mix while sync in progress.");
        return false;
    }

    if (!fDryRun && m_wallet->IsLocked(true)) {
        strAutoDenomResult = _("Wallet is locked.");
        return false;
    }

    int nMnCountEnabled = m_dmnman.GetListAtChainTip().GetValidMNsCount();

    // If we've used 90% of the Masternode list then drop the oldest first ~30%
    int nThreshold_high = nMnCountEnabled * 0.9;
    int nThreshold_low = nThreshold_high * 0.7;
    WalletCJLogPrint(m_wallet, "Checking vecMasternodesUsed: size: %d, threshold: %d\n", (int)vecMasternodesUsed.size(), nThreshold_high);

    if ((int)vecMasternodesUsed.size() > nThreshold_high) {
        vecMasternodesUsed.erase(vecMasternodesUsed.begin(), vecMasternodesUsed.begin() + vecMasternodesUsed.size() - nThreshold_low);
        WalletCJLogPrint(m_wallet, "  vecMasternodesUsed: new size: %d, threshold: %d\n", (int)vecMasternodesUsed.size(), nThreshold_high);
    }

    bool fResult = true;
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    if (int(deqSessions.size()) < CCoinJoinClientOptions::GetSessions()) {
        deqSessions.emplace_back(m_wallet, *this, m_dmnman, m_mn_metaman, m_mn_sync, m_isman, m_queueman, m_is_masternode);
    }
    for (auto& session : deqSessions) {
        if (!CheckAutomaticBackup()) return false;

        if (WaitForAnotherBlock()) {
            strAutoDenomResult = _("Last successful action was too recent.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::DoAutomaticDenominating -- %s\n", strAutoDenomResult.original);
            return false;
        }

        fResult &= session.DoAutomaticDenominating(chainman, connman, mempool, fDryRun);
    }

    return fResult;
}

void CCoinJoinClientManager::AddUsedMasternode(const COutPoint& outpointMn)
{
    vecMasternodesUsed.push_back(outpointMn);
}

CDeterministicMNCPtr CCoinJoinClientManager::GetRandomNotUsedMasternode()
{
    auto mnList = m_dmnman.GetListAtChainTip();

    size_t nCountEnabled = mnList.GetValidMNsCount();
    size_t nCountNotExcluded = nCountEnabled - vecMasternodesUsed.size();

    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- %d enabled masternodes, %d masternodes to choose from\n", __func__, nCountEnabled, nCountNotExcluded);
    if (nCountNotExcluded < 1) {
        return nullptr;
    }

    // fill a vector
    std::vector<CDeterministicMNCPtr> vpMasternodesShuffled;
    vpMasternodesShuffled.reserve(nCountEnabled);
    mnList.ForEachMNShared(true, [&vpMasternodesShuffled](const CDeterministicMNCPtr& dmn) {
        vpMasternodesShuffled.emplace_back(dmn);
    });

    // shuffle pointers
    Shuffle(vpMasternodesShuffled.begin(), vpMasternodesShuffled.end(), FastRandomContext());

    std::set<COutPoint> excludeSet(vecMasternodesUsed.begin(), vecMasternodesUsed.end());

    // loop through
    for (const auto& dmn : vpMasternodesShuffled) {
        if (excludeSet.count(dmn->collateralOutpoint)) {
            continue;
        }

        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- found, masternode=%s\n", __func__, dmn->collateralOutpoint.ToStringShort());
        return dmn;
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- failed\n", __func__);
    return nullptr;
}

static int WinnersToSkip()
{
    return (Params().NetworkIDString() == CBaseChainParams::DEVNET ||
            Params().NetworkIDString() == CBaseChainParams::REGTEST)
            ? 1 : 8;
}

bool CCoinJoinClientSession::JoinExistingQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;
    if (m_queueman == nullptr) return false;

    const auto mnList = m_dmnman.GetListAtChainTip();
    const int nWeightedMnCount = mnList.GetValidWeightedMNsCount();

    // Look through the queues and see if anything matches
    CCoinJoinQueue dsq;
    while (m_queueman->GetQueueItemAndTry(dsq)) {
        auto dmn = mnList.GetValidMNByCollateral(dsq.masternodeOutpoint);

        if (!dmn) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- dsq masternode is not in masternode list, masternode=%s\n", dsq.masternodeOutpoint.ToStringShort());
            continue;
        }

        // skip next mn payments winners
        if (dmn->pdmnState->nLastPaidHeight + nWeightedMnCount < mnList.GetHeight() + WinnersToSkip()) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- skipping winner, masternode=%s\n", dmn->proTxHash.ToString());
            continue;
        }

        // mixing rate limit i.e. nLastDsq check should already pass in DSQUEUE ProcessMessage
        // in order for dsq to get into vecCoinJoinQueue, so we should be safe to mix already,
        // no need for additional verification here

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- trying queue: %s\n", dsq.ToString());

        std::vector<CTxDSIn> vecTxDSInTmp;

        // Try to match their denominations if possible, select exact number of denominations
        if (!m_wallet->SelectTxDSInsByDenomination(dsq.nDenom, nBalanceNeedsAnonymized, vecTxDSInTmp)) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- Couldn't match denomination %d (%s)\n", dsq.nDenom, CoinJoin::DenominationToString(dsq.nDenom));
            continue;
        }

        m_clientman.AddUsedMasternode(dsq.masternodeOutpoint);

        if (connman.IsMasternodeOrDisconnectRequested(dmn->pdmnState->netInfo->GetPrimary())) {
            WalletCJLogPrint(m_wallet, /* Continued */
                             "CCoinJoinClientSession::JoinExistingQueue -- skipping connection, masternode=%s\n", dmn->proTxHash.ToString());
            continue;
        }

        nSessionDenom = dsq.nDenom;
        mixingMasternode = dmn;
        pendingDsaRequest = CPendingDsaRequest(dmn->proTxHash, CCoinJoinAccept(nSessionDenom, txMyCollateral));
        connman.AddPendingMasternode(dmn->proTxHash);
        SetState(POOL_STATE_QUEUE);
        nTimeLastSuccessfulStep = GetTime();
        WalletCJLogPrint(m_wallet, /* Continued */
                         "CCoinJoinClientSession::JoinExistingQueue -- pending connection, masternode=%s, nSessionDenom=%d (%s)\n",
                         dmn->proTxHash.ToString(), nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));
        strAutoDenomResult = _("Trying to connect…");
        return true;
    }
    strAutoDenomResult = _("Failed to find mixing queue to join");
    return false;
}

bool CCoinJoinClientSession::StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman)
{
    assert(m_mn_metaman.IsValid());

    if (!CCoinJoinClientOptions::IsEnabled()) return false;
    if (nBalanceNeedsAnonymized <= 0) return false;

    int nTries = 0;
    const auto mnList = m_dmnman.GetListAtChainTip();
    const int nMnCount = mnList.GetValidMNsCount();
    const int nWeightedMnCount = mnList.GetValidWeightedMNsCount();

    // find available denominated amounts
    std::set<CAmount> setAmounts;
    if (!m_wallet->SelectDenominatedAmounts(nBalanceNeedsAnonymized, setAmounts)) {
        // this should never happen
        strAutoDenomResult = _("Can't mix: no compatible inputs found!");
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- %s\n", strAutoDenomResult.original);
        return false;
    }

    // otherwise, try one randomly
    while (nTries < 10) {
        auto dmn = m_clientman.GetRandomNotUsedMasternode();
        if (!dmn) {
            strAutoDenomResult = _("Can't find random Masternode.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- %s\n", strAutoDenomResult.original);
            return false;
        }

        m_clientman.AddUsedMasternode(dmn->collateralOutpoint);

        // skip next mn payments winners
        if (dmn->pdmnState->nLastPaidHeight + nWeightedMnCount < mnList.GetHeight() + WinnersToSkip()) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- skipping winner, masternode=%s\n", dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        int64_t nLastDsq = m_mn_metaman.GetMetaInfo(dmn->proTxHash)->GetLastDsq();
        int64_t nDsqThreshold = m_mn_metaman.GetDsqThreshold(dmn->proTxHash, nMnCount);
        if (nLastDsq != 0 && nDsqThreshold > m_mn_metaman.GetDsqCount()) {
            WalletCJLogPrint(m_wallet, /* Continued */
                             "CCoinJoinClientSession::StartNewQueue -- too early to mix with node," /* Continued */
                             " masternode=%s, nLastDsq=%d, nDsqThreshold=%d, nDsqCount=%d\n",
                             dmn->proTxHash.ToString(), nLastDsq, nDsqThreshold, m_mn_metaman.GetDsqCount());
            nTries++;
            continue;
        }

        if (connman.IsMasternodeOrDisconnectRequested(dmn->pdmnState->netInfo->GetPrimary())) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- skipping connection, masternode=%s\n",
                             dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- attempting connection, masternode=%s, tries=%s\n",
                         dmn->proTxHash.ToString(), nTries);

        // try to get a single random denom out of setAmounts
        while (nSessionDenom == 0) {
            for (auto it = setAmounts.rbegin(); it != setAmounts.rend(); ++it) {
                if (setAmounts.size() > 1 && GetRand<size_t>(/*nMax=*/2)) continue;
                nSessionDenom = CoinJoin::AmountToDenomination(*it);
                break;
            }
        }

        mixingMasternode = dmn;
        connman.AddPendingMasternode(dmn->proTxHash);
        pendingDsaRequest = CPendingDsaRequest(dmn->proTxHash, CCoinJoinAccept(nSessionDenom, txMyCollateral));
        SetState(POOL_STATE_QUEUE);
        nTimeLastSuccessfulStep = GetTime();
        WalletCJLogPrint( /* Continued */
            m_wallet, "CCoinJoinClientSession::StartNewQueue -- pending connection, masternode=%s, nSessionDenom=%d (%s)\n",
            dmn->proTxHash.ToString(), nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));
        strAutoDenomResult = _("Trying to connect…");
        return true;
    }
    strAutoDenomResult = _("Failed to start a new mixing queue");
    return false;
}

bool CCoinJoinClientSession::ProcessPendingDsaRequest(CConnman& connman)
{
    if (!pendingDsaRequest) return false;

    CService mn_addr;
    if (auto dmn = m_dmnman.GetListAtChainTip().GetMN(pendingDsaRequest.GetProTxHash())) {
        mn_addr = Assert(dmn->pdmnState)->netInfo->GetPrimary();
    } else {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- cannot find address to connect, masternode=%s\n", __func__,
            pendingDsaRequest.GetProTxHash().ToString());
        WITH_LOCK(cs_coinjoin, SetNull());
        return false;
    }

    bool fDone = connman.ForNode(mn_addr, [this, &connman](CNode* pnode) {
        WalletCJLogPrint(m_wallet, "-- processing dsa queue for addr=%s\n", pnode->addr.ToStringAddrPort());
        nTimeLastSuccessfulStep = GetTime();
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSACCEPT, pendingDsaRequest.GetDSA()));
        return true;
    });

    if (fDone) {
        pendingDsaRequest = CPendingDsaRequest();
    } else if (pendingDsaRequest.IsExpired()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- failed to connect, masternode=%s\n", __func__,
                         pendingDsaRequest.GetProTxHash().ToString());
        WITH_LOCK(cs_coinjoin, SetNull());
    }

    return fDone;
}

void CCoinJoinClientManager::ProcessPendingDsaRequest(CConnman& connman)
{
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        if (session.ProcessPendingDsaRequest(connman)) {
            strAutoDenomResult = _("Mixing in progress…");
        }
    }
}

bool CCoinJoinClientManager::TrySubmitDenominate(const uint256& proTxHash, CConnman& connman)
{
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        CDeterministicMNCPtr mnMixing;
        if (session.GetMixingMasternodeInfo(mnMixing) && mnMixing->proTxHash == proTxHash && session.GetState() == POOL_STATE_QUEUE) {
            session.SubmitDenominate(connman);
            return true;
        }
    }
    return false;
}

bool CCoinJoinClientManager::MarkAlreadyJoinedQueueAsTried(CCoinJoinQueue& dsq) const
{
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (const auto& session : deqSessions) {
        CDeterministicMNCPtr mnMixing;
        if (session.GetMixingMasternodeInfo(mnMixing) && mnMixing->collateralOutpoint == dsq.masternodeOutpoint) {
            dsq.fTried = true;
            return true;
        }
    }
    return false;
}

bool CCoinJoinClientSession::SubmitDenominate(CConnman& connman)
{
    LOCK(m_wallet->cs_wallet);

    std::string strError;
    std::vector<CTxDSIn> vecTxDSIn;
    std::vector<std::pair<CTxDSIn, CTxOut> > vecPSInOutPairsTmp;

    if (!SelectDenominate(strError, vecTxDSIn)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- SelectDenominate failed, error: %s\n", strError);
        return false;
    }

    std::vector<std::pair<int, size_t> > vecInputsByRounds;

    for (const auto i : irange::range(CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds())) {
        if (PrepareDenominate(i, i, strError, vecTxDSIn, vecPSInOutPairsTmp, true)) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Running CoinJoin denominate for %d rounds, success\n", i);
            vecInputsByRounds.emplace_back(i, vecPSInOutPairsTmp.size());
        } else {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Running CoinJoin denominate for %d rounds, error: %s\n", i, strError);
        }
    }

    // more inputs first, for equal input count prefer the one with fewer rounds
    std::sort(vecInputsByRounds.begin(), vecInputsByRounds.end(), [](const auto& a, const auto& b) {
        return a.second > b.second || (a.second == b.second && a.first < b.first);
    });

    WalletCJLogPrint(m_wallet, "vecInputsByRounds for denom %d\n", nSessionDenom);
    for (const auto& pair : vecInputsByRounds) {
        WalletCJLogPrint(m_wallet, "vecInputsByRounds: rounds: %d, inputs: %d\n", pair.first, pair.second);
    }

    int nRounds = vecInputsByRounds.begin()->first;
    if (PrepareDenominate(nRounds, nRounds, strError, vecTxDSIn, vecPSInOutPairsTmp)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Running CoinJoin denominate for %d rounds, success\n", nRounds);
        return SendDenominate(vecPSInOutPairsTmp, connman);
    }

    // We failed? That's strange but let's just make final attempt and try to mix everything
    if (PrepareDenominate(0, CCoinJoinClientOptions::GetRounds() - 1, strError, vecTxDSIn, vecPSInOutPairsTmp)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Running CoinJoin denominate for all rounds, success\n");
        return SendDenominate(vecPSInOutPairsTmp, connman);
    }

    // Should never actually get here but just in case
    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Running CoinJoin denominate for all rounds, error: %s\n", strError);
    strAutoDenomResult = Untranslated(strError);
    return false;
}

bool CCoinJoinClientSession::SelectDenominate(std::string& strErrorRet, std::vector<CTxDSIn>& vecTxDSInRet)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    if (m_wallet->IsLocked(true)) {
        strErrorRet = "Wallet locked, unable to create transaction!";
        return false;
    }

    if (GetEntriesCount() > 0) {
        strErrorRet = "Already have pending entries in the CoinJoin pool";
        return false;
    }

    vecTxDSInRet.clear();

    bool fSelected = m_wallet->SelectTxDSInsByDenomination(nSessionDenom, CoinJoin::GetMaxPoolAmount(), vecTxDSInRet);
    if (!fSelected) {
        strErrorRet = "Can't select current denominated inputs";
        return false;
    }

    return true;
}

bool CCoinJoinClientSession::PrepareDenominate(int nMinRounds, int nMaxRounds, std::string& strErrorRet, const std::vector<CTxDSIn>& vecTxDSIn, std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsRet, bool fDryRun)
{
    AssertLockHeld(m_wallet->cs_wallet);

    if (!CoinJoin::IsValidDenomination(nSessionDenom)) {
        strErrorRet = "Incorrect session denom";
        return false;
    }
    CAmount nDenomAmount = CoinJoin::DenominationToAmount(nSessionDenom);

    // NOTE: No need to randomize order of inputs because they were
    // initially shuffled in CWallet::SelectTxDSInsByDenomination already.
    size_t nSteps{0};
    vecPSInOutPairsRet.clear();

    // Try to add up to COINJOIN_ENTRY_MAX_SIZE of every needed denomination
    for (const auto& entry : vecTxDSIn) {
        if (nSteps >= COINJOIN_ENTRY_MAX_SIZE) break;
        if (entry.nRounds < nMinRounds || entry.nRounds > nMaxRounds) continue;

        CScript scriptDenom;
        if (fDryRun) {
            scriptDenom = CScript();
        } else {
            // randomly skip some inputs when we have at least one of the same denom already
            // TODO: make it adjustable via options/cmd-line params
            if (nSteps >= 1 && GetRand<size_t>(/*nMax=*/5) == 0) {
                // still count it as a step to randomize number of inputs
                // if we have more than (or exactly) COINJOIN_ENTRY_MAX_SIZE of them
                ++nSteps;
                continue;
            }
            scriptDenom = keyHolderStorage.AddKey(m_wallet.get());
        }
        vecPSInOutPairsRet.emplace_back(entry, CTxOut(nDenomAmount, scriptDenom));
        // step is complete
        ++nSteps;
    }

    if (vecPSInOutPairsRet.empty()) {
        keyHolderStorage.ReturnAll();
        strErrorRet = "Can't prepare current denominated outputs";
        return false;
    }

    if (fDryRun) {
        return true;
    }

    for (const auto& [txDsIn, txDsOut] : vecPSInOutPairsRet) {
        m_wallet->LockCoin(txDsIn.prevout);
        vecOutPointLocked.push_back(txDsIn.prevout);
    }

    return true;
}

// Create collaterals by looping through inputs grouped by addresses
bool CCoinJoinClientSession::MakeCollateralAmounts()
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    LOCK(m_wallet->cs_wallet);

    // NOTE: We do not allow txes larger than 100 kB, so we have to limit number of inputs here.
    // We still want to consume a lot of inputs to avoid creating only smaller denoms though.
    // Knowing that each CTxIn is at least 148 B big, 400 inputs should take 400 x ~148 B = ~60 kB.
    // This still leaves more than enough room for another data of typical MakeCollateralAmounts tx.
    std::vector<CompactTallyItem> vecTally = m_wallet->SelectCoinsGroupedByAddresses(false, false, true, 400);
    if (vecTally.empty()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::MakeCollateralAmounts -- SelectCoinsGroupedByAddresses can't find any inputs!\n");
        return false;
    }

    // Start from the smallest balances first to consume tiny amounts and cleanup UTXO a bit
    std::sort(vecTally.begin(), vecTally.end(), [](const CompactTallyItem& a, const CompactTallyItem& b) {
        return a.nAmount < b.nAmount;
    });

    // First try to use only non-denominated funds
    if (ranges::any_of(vecTally, [&](const auto& item) EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet) {
            return MakeCollateralAmounts(item, false);
        })) {
        return true;
    }

    // There should be at least some denominated funds we should be able to break in pieces to continue mixing
    if (ranges::any_of(vecTally, [&](const auto& item) EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet) {
            return MakeCollateralAmounts(item, true);
        })) {
        return true;
    }

    // If we got here then something is terribly broken actually
    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::MakeCollateralAmounts -- ERROR: Can't make collaterals!\n");
    return false;
}

// Split up large inputs or create fee sized inputs
bool CCoinJoinClientSession::MakeCollateralAmounts(const CompactTallyItem& tallyItem, bool fTryDenominated)
{
    // TODO: consider refactoring to remove duplicated code with CCoinJoinClientSession::CreateDenominated
    AssertLockHeld(m_wallet->cs_wallet);

    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    // Denominated input is always a single one, so we can check its amount directly and return early
    if (!fTryDenominated && tallyItem.outpoints.size() == 1 && CoinJoin::IsDenominatedAmount(tallyItem.nAmount)) {
        return false;
    }

    // Skip single inputs that can be used as collaterals already
    if (tallyItem.outpoints.size() == 1 && CoinJoin::IsCollateralAmount(tallyItem.nAmount)) {
        return false;
    }

    CTransactionBuilder txBuilder(*m_wallet, tallyItem);

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Start %s\n", __func__, txBuilder.ToString());

    // Skip way too tiny amounts. Smallest we want is minimum collateral amount in a one output tx
    if (!txBuilder.CouldAddOutput(CoinJoin::GetCollateralAmount())) {
        return false;
    }

    int nCase{0}; // Just for debug logs
    if (txBuilder.CouldAddOutputs({CoinJoin::GetMaxCollateralAmount(), CoinJoin::GetCollateralAmount()})) {
        nCase = 1;
        // <case1>, see TransactionRecord::decomposeTransaction
        // Out1 == CoinJoin::GetMaxCollateralAmount()
        // Out2 >= CoinJoin::GetCollateralAmount()

        txBuilder.AddOutput(CoinJoin::GetMaxCollateralAmount());
        // Note, here we first add a zero amount output to get the remainder after all fees and then assign it
        CTransactionBuilderOutput* out = txBuilder.AddOutput();
        CAmount nAmountLeft = txBuilder.GetAmountLeft();
        // If remainder is denominated add one duff to the fee
        out->UpdateAmount(CoinJoin::IsDenominatedAmount(nAmountLeft) ? nAmountLeft - 1 : nAmountLeft);

    } else if (txBuilder.CouldAddOutputs({CoinJoin::GetCollateralAmount(), CoinJoin::GetCollateralAmount()})) {
        nCase = 2;
        // <case2>, see TransactionRecord::decomposeTransaction
        // Out1 CoinJoin::IsCollateralAmount()
        // Out2 CoinJoin::IsCollateralAmount()

        // First add two outputs to get the available value after all fees
        CTransactionBuilderOutput* out1 = txBuilder.AddOutput();
        CTransactionBuilderOutput* out2 = txBuilder.AddOutput();

        // Create two equal outputs from the available value. This adds one duff to the fee if txBuilder.GetAmountLeft() is odd.
        CAmount nAmountOutputs = txBuilder.GetAmountLeft() / 2;

        assert(CoinJoin::IsCollateralAmount(nAmountOutputs));

        out1->UpdateAmount(nAmountOutputs);
        out2->UpdateAmount(nAmountOutputs);

    } else { // still at least possible to add one CoinJoin::GetCollateralAmount() output
        nCase = 3;
        // <case3>, see TransactionRecord::decomposeTransaction
        // Out1 CoinJoin::IsCollateralAmount()
        // Out2 Skipped
        CTransactionBuilderOutput* out = txBuilder.AddOutput();
        out->UpdateAmount(txBuilder.GetAmountLeft());

        assert(CoinJoin::IsCollateralAmount(out->GetAmount()));
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Done with case %d: %s\n", __func__, nCase, txBuilder.ToString());

    assert(txBuilder.IsDust(txBuilder.GetAmountLeft()));

    bilingual_str strResult;
    if (!txBuilder.Commit(strResult)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Commit failed: %s\n", __func__, strResult.original);
        return false;
    }

    m_clientman.UpdatedSuccessBlock();

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- txid: %s\n", __func__, strResult.original);

    return true;
}

bool CCoinJoinClientSession::CreateCollateralTransaction(CMutableTransaction& txCollateral, std::string& strReason)
{
    AssertLockHeld(m_wallet->cs_wallet);

    CCoinControl coin_control(CoinType::ONLY_COINJOIN_COLLATERAL);
    std::vector<COutput> vCoins{AvailableCoinsListUnspent(*m_wallet, &coin_control).coins};
    if (vCoins.empty()) {
        strReason = strprintf("%s requires a collateral transaction and could not locate an acceptable input!", gCoinJoinName);
        return false;
    }

    const auto& output = vCoins.at(GetRand(vCoins.size()));
    const CTxOut txout = output.txout;

    txCollateral.vin.clear();
    txCollateral.vin.emplace_back(output.outpoint.hash, output.outpoint.n);
    txCollateral.vout.clear();

    // pay collateral charge in fees
    // NOTE: no need for protobump patch here,
    // CoinJoin::IsCollateralAmount in GetCollateralTxDSIn should already take care of this
    if (txout.nValue >= CoinJoin::GetCollateralAmount() * 2) {
        // make our change address
        CScript scriptChange;
        CTxDestination dest;
        ReserveDestination reserveDest(m_wallet.get());
        bool success = reserveDest.GetReservedDestination(dest, true);
        assert(success); // should never fail, as we just unlocked
        scriptChange = GetScriptForDestination(dest);
        reserveDest.KeepDestination();
        // return change
        txCollateral.vout.emplace_back(txout.nValue - CoinJoin::GetCollateralAmount(), scriptChange);
    } else { // txout.nValue < CoinJoin::GetCollateralAmount() * 2
        // create dummy data output only and pay everything as a fee
        txCollateral.vout.emplace_back(0, CScript() << OP_RETURN);
    }

    if (!m_wallet->SignTransaction(txCollateral)) {
        strReason = "Unable to sign collateral transaction!";
        return false;
    }

    return true;
}

// Create denominations by looping through inputs grouped by addresses
bool CCoinJoinClientSession::CreateDenominated(CAmount nBalanceToDenominate)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    LOCK(m_wallet->cs_wallet);

    // NOTE: We do not allow txes larger than 100 kB, so we have to limit number of inputs here.
    // We still want to consume a lot of inputs to avoid creating only smaller denoms though.
    // Knowing that each CTxIn is at least 148 B big, 400 inputs should take 400 x ~148 B = ~60 kB.
    // This still leaves more than enough room for another data of typical CreateDenominated tx.
    std::vector<CompactTallyItem> vecTally = m_wallet->SelectCoinsGroupedByAddresses(true, true, true, 400);
    if (vecTally.empty()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::CreateDenominated -- SelectCoinsGroupedByAddresses can't find any inputs!\n");
        return false;
    }

    // Start from the largest balances first to speed things up by creating txes with larger/largest denoms included
    std::sort(vecTally.begin(), vecTally.end(), [](const CompactTallyItem& a, const CompactTallyItem& b) {
        return a.nAmount > b.nAmount;
    });

    bool fCreateMixingCollaterals = !m_wallet->HasCollateralInputs();

    for (const auto& item : vecTally) {
        if (!CreateDenominated(nBalanceToDenominate, item, fCreateMixingCollaterals)) continue;
        return true;
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::CreateDenominated -- failed!\n");
    return false;
}

// Create denominations
bool CCoinJoinClientSession::CreateDenominated(CAmount nBalanceToDenominate, const CompactTallyItem& tallyItem, bool fCreateMixingCollaterals)
{
    AssertLockHeld(m_wallet->cs_wallet);

    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    // denominated input is always a single one, so we can check its amount directly and return early
    if (tallyItem.outpoints.size() == 1 && CoinJoin::IsDenominatedAmount(tallyItem.nAmount)) {
        return false;
    }

    CTransactionBuilder txBuilder(*m_wallet, tallyItem);

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Start %s\n", __func__, txBuilder.ToString());

    // ****** Add an output for mixing collaterals ************ /

    if (fCreateMixingCollaterals && !txBuilder.AddOutput(CoinJoin::GetMaxCollateralAmount())) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Failed to add collateral output\n", __func__);
        return false;
    }

    // ****** Add outputs for denoms ************ /

    bool fAddFinal = true;
    auto denoms = CoinJoin::GetStandardDenominations();

    std::map<CAmount, int> mapDenomCount;
    for (auto nDenomValue : denoms) {
        mapDenomCount.insert(std::pair<CAmount, int>(nDenomValue, m_wallet->CountInputsWithAmount(nDenomValue)));
    }

    // Will generate outputs for the createdenoms up to coinjoinmaxdenoms per denom

    // This works in the way creating PS denoms has traditionally worked, assuming enough funds,
    // it will start with the smallest denom then create 11 of those, then go up to the next biggest denom create 11
    // and repeat. Previously, once the largest denom was reached, as many would be created were created as possible and
    // then any remaining was put into a change address and denominations were created in the same manner a block later.
    // Now, in this system, so long as we don't reach COINJOIN_DENOM_OUTPUTS_THRESHOLD outputs the process repeats in
    // the same transaction, creating up to nCoinJoinDenomsHardCap per denomination in a single transaction.

    while (txBuilder.CouldAddOutput(CoinJoin::GetSmallestDenomination()) && txBuilder.CountOutputs() < COINJOIN_DENOM_OUTPUTS_THRESHOLD) {
        for (auto it = denoms.rbegin(); it != denoms.rend(); ++it) {
            CAmount nDenomValue = *it;
            auto currentDenomIt = mapDenomCount.find(nDenomValue);

            int nOutputs = 0;

            const auto& strFunc = __func__;
            auto needMoreOutputs = [&]() {
                if (txBuilder.CouldAddOutput(nDenomValue)) {
                    if (fAddFinal && nBalanceToDenominate > 0 && nBalanceToDenominate < nDenomValue) {
                        fAddFinal = false; // add final denom only once, only the smalest possible one
                        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - FINAL - nDenomValue: %f, nBalanceToDenominate: %f, nOutputs: %d, %s\n",
                                                     strFunc, (float) nDenomValue / COIN, (float) nBalanceToDenominate / COIN, nOutputs, txBuilder.ToString());
                        return true;
                    } else if (nBalanceToDenominate >= nDenomValue) {
                        return true;
                    }
                }
                return false;
            };

            // add each output up to 11 times or until it can't be added again or until we reach nCoinJoinDenomsGoal
            while (needMoreOutputs() && nOutputs <= 10 && currentDenomIt->second < CCoinJoinClientOptions::GetDenomsGoal()) {
                // Add output and subtract denomination amount
                if (txBuilder.AddOutput(nDenomValue)) {
                    ++nOutputs;
                    ++currentDenomIt->second;
                    nBalanceToDenominate -= nDenomValue;
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - nDenomValue: %f, nBalanceToDenominate: %f, nOutputs: %d, %s\n",
                                                 __func__, (float) nDenomValue / COIN, (float) nBalanceToDenominate / COIN, nOutputs, txBuilder.ToString());
                } else {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - Error: AddOutput failed for nDenomValue: %f, nBalanceToDenominate: %f, nOutputs: %d, %s\n",
                                                 __func__, (float) nDenomValue / COIN, (float) nBalanceToDenominate / COIN, nOutputs, txBuilder.ToString());
                    return false;
                }

            }

            if (txBuilder.GetAmountLeft() == 0 || nBalanceToDenominate <= 0) break;
        }

        bool finished = true;
        for (const auto& [denom, count] : mapDenomCount) {
            // Check if this specific denom could use another loop, check that there aren't nCoinJoinDenomsGoal of this
            // denom and that our nValueLeft/nBalanceToDenominate is enough to create one of these denoms, if so, loop again.
            if (count < CCoinJoinClientOptions::GetDenomsGoal() && txBuilder.CouldAddOutput(denom) && nBalanceToDenominate > 0) {
                finished = false;
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - NOT finished - nDenomValue: %f, count: %d, nBalanceToDenominate: %f, %s\n",
                                             __func__, (float) denom / COIN, count, (float) nBalanceToDenominate / COIN, txBuilder.ToString());
                break;
            }
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - FINISHED - nDenomValue: %f, count: %d, nBalanceToDenominate: %f, %s\n",
                                         __func__, (float) denom / COIN, count, (float) nBalanceToDenominate / COIN, txBuilder.ToString());
        }

        if (finished) break;
    }

    // Now that nCoinJoinDenomsGoal worth of each denom have been created or the max number of denoms given the value of the input, do something with the remainder.
    if (txBuilder.CouldAddOutput(CoinJoin::GetSmallestDenomination()) && nBalanceToDenominate >= CoinJoin::GetSmallestDenomination() && txBuilder.CountOutputs() < COINJOIN_DENOM_OUTPUTS_THRESHOLD) {
        CAmount nLargestDenomValue = denoms.front();

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 2 - Process remainder: %s\n", __func__, txBuilder.ToString());

        auto countPossibleOutputs = [&](CAmount nAmount) -> int {
            std::vector<CAmount> vecOutputs;
            while (true) {
                // Create a potential output
                vecOutputs.push_back(nAmount);
                if (!txBuilder.CouldAddOutputs(vecOutputs) || txBuilder.CountOutputs() + vecOutputs.size() > COINJOIN_DENOM_OUTPUTS_THRESHOLD) {
                    // If it's not possible to add it due to insufficient amount left or total number of outputs exceeds
                    // COINJOIN_DENOM_OUTPUTS_THRESHOLD drop the output again and stop trying.
                    vecOutputs.pop_back();
                    break;
                }
            }
            return static_cast<int>(vecOutputs.size());
        };

        // Go big to small
        for (auto nDenomValue : denoms) {
            if (nBalanceToDenominate <= 0) break;
            int nOutputs = 0;

            // Number of denoms we can create given our denom and the amount of funds we have left
            int denomsToCreateValue = countPossibleOutputs(nDenomValue);
            // Prefer overshooting the target balance by larger denoms (hence `+1`) instead of a more
            // accurate approximation by many smaller denoms. This is ok because when we get here we
            // should have nCoinJoinDenomsGoal of each smaller denom already. Also, without `+1`
            // we can end up in a situation when there is already nCoinJoinDenomsHardCap of smaller
            // denoms, yet we can't mix the remaining nBalanceToDenominate because it's smaller than
            // nDenomValue (and thus denomsToCreateBal == 0), so the target would never get reached
            // even when there is enough funds for that.
            int denomsToCreateBal = (nBalanceToDenominate / nDenomValue) + 1;
            // Use the smaller value
            int denomsToCreate = denomsToCreateValue > denomsToCreateBal ? denomsToCreateBal : denomsToCreateValue;
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 2 - nBalanceToDenominate: %f, nDenomValue: %f, denomsToCreateValue: %d, denomsToCreateBal: %d\n",
                                         __func__, (float) nBalanceToDenominate / COIN, (float) nDenomValue / COIN, denomsToCreateValue, denomsToCreateBal);
            auto it = mapDenomCount.find(nDenomValue);
            for (const auto i : irange::range(denomsToCreate)) {
                // Never go above the cap unless it's the largest denom
                if (nDenomValue != nLargestDenomValue && it->second >= CCoinJoinClientOptions::GetDenomsHardCap()) break;

                // Increment helpers, add output and subtract denomination amount
                if (txBuilder.AddOutput(nDenomValue)) {
                    nOutputs++;
                    it->second++;
                    nBalanceToDenominate -= nDenomValue;
                } else {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 2 - Error: AddOutput failed at %d/%d, %s\n", __func__, i + 1, denomsToCreate, txBuilder.ToString());
                    break;
                }
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 2 - nDenomValue: %f, nBalanceToDenominate: %f, nOutputs: %d, %s\n",
                                             __func__, (float) nDenomValue / COIN, (float) nBalanceToDenominate / COIN, nOutputs, txBuilder.ToString());
                if (txBuilder.CountOutputs() >= COINJOIN_DENOM_OUTPUTS_THRESHOLD) break;
            }
            if (txBuilder.CountOutputs() >= COINJOIN_DENOM_OUTPUTS_THRESHOLD) break;
        }
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 3 - nBalanceToDenominate: %f, %s\n", __func__, (float) nBalanceToDenominate / COIN, txBuilder.ToString());

    for (const auto& [denom, count] : mapDenomCount) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 3 - DONE - nDenomValue: %f, count: %d\n", __func__, (float) denom / COIN, count);
    }

    // No reasons to create mixing collaterals if we can't create denoms to mix
    if ((fCreateMixingCollaterals && txBuilder.CountOutputs() == 1) || txBuilder.CountOutputs() == 0) {
        return false;
    }

    bilingual_str strResult;
    if (!txBuilder.Commit(strResult)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Commit failed: %s\n", __func__, strResult.original);
        return false;
    }

    // use the same nCachedLastSuccessBlock as for DS mixing to prevent race
    m_clientman.UpdatedSuccessBlock();

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- txid: %s\n", __func__, strResult.original);

    return true;
}

void CCoinJoinClientSession::RelayIn(const CCoinJoinEntry& entry, CConnman& connman) const
{
    if (!mixingMasternode) return;

    connman.ForNode(mixingMasternode->pdmnState->netInfo->GetPrimary(), [&entry, &connman, this](CNode* pnode) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::RelayIn -- found master, relaying message to %s\n",
                         pnode->addr.ToStringAddrPort());
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSVIN, entry));
        return true;
    });
}

void CCoinJoinClientSession::SetState(PoolState nStateNew)
{
    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SetState -- nState: %d, nStateNew: %d\n", nState.load(), nStateNew);
    nState = nStateNew;
}

void CCoinJoinClientManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    nCachedBlockHeight = pindex->nHeight;
    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::UpdatedBlockTip -- nCachedBlockHeight: %d\n", nCachedBlockHeight);
}

void CCoinJoinClientQueueManager::DoMaintenance()
{
    if (m_is_masternode) return; // no client-side mixing on masternodes

    if (!m_mn_sync.IsBlockchainSynced() || ShutdownRequested()) return;

    CheckQueue();
}

void CCoinJoinClientManager::DoMaintenance(ChainstateManager& chainman, CConnman& connman, const CTxMemPool& mempool)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return;
    if (m_is_masternode) return; // no client-side mixing on masternodes

    if (!m_mn_sync.IsBlockchainSynced() || ShutdownRequested()) return;

    static int nTick = 0;
    static int nDoAutoNextRun = nTick + COINJOIN_AUTO_TIMEOUT_MIN;

    nTick++;
    CheckTimeout();
    ProcessPendingDsaRequest(connman);
    if (nDoAutoNextRun == nTick) {
        DoAutomaticDenominating(chainman, connman, mempool);
        nDoAutoNextRun = nTick + COINJOIN_AUTO_TIMEOUT_MIN + GetRand<int>(/*nMax=*/COINJOIN_AUTO_TIMEOUT_MAX - COINJOIN_AUTO_TIMEOUT_MIN);
    }
}

void CCoinJoinClientSession::GetJsonInfo(UniValue& obj) const
{
    assert(obj.isObject());
    if (mixingMasternode != nullptr) {
        assert(mixingMasternode->pdmnState);
        obj.pushKV("protxhash", mixingMasternode->proTxHash.ToString());
        obj.pushKV("outpoint", mixingMasternode->collateralOutpoint.ToStringShort());
        if (m_wallet->chain().rpcEnableDeprecated("service")) {
            obj.pushKV("service", mixingMasternode->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
        }
        obj.pushKV("addresses", mixingMasternode->pdmnState->netInfo->ToJson());
    }
    obj.pushKV("denomination", ValueFromAmount(CoinJoin::DenominationToAmount(nSessionDenom)));
    obj.pushKV("state", GetStateString());
    obj.pushKV("entries_count", GetEntriesCount());
}

void CCoinJoinClientManager::GetJsonInfo(UniValue& obj) const
{
    assert(obj.isObject());
    obj.pushKV("running", IsMixing());

    UniValue arrSessions(UniValue::VARR);
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (const auto& session : deqSessions) {
        if (session.GetState() != POOL_STATE_IDLE) {
            UniValue objSession(UniValue::VOBJ);
            session.GetJsonInfo(objSession);
            arrSessions.push_back(objSession);
        }
    }
    obj.pushKV("sessions", arrSessions);
}

void CoinJoinWalletManager::Add(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallet_manager_map);
    m_wallet_manager_map.try_emplace(wallet->GetName(),
                                     std::make_unique<CCoinJoinClientManager>(wallet, m_dmnman, m_mn_metaman, m_mn_sync,
                                                                              m_isman, m_queueman, m_is_masternode));
}

void CoinJoinWalletManager::DoMaintenance(CConnman& connman)
{
    LOCK(cs_wallet_manager_map);
    for (auto& [_, clientman] : m_wallet_manager_map) {
        clientman->DoMaintenance(m_chainman, connman, m_mempool);
    }
}

void CoinJoinWalletManager::Remove(const std::string& name) {
    LOCK(cs_wallet_manager_map);
    m_wallet_manager_map.erase(name);
}

void CoinJoinWalletManager::Flush(const std::string& name)
{
    auto clientman = Assert(Get(name));
    clientman->ResetPool();
    clientman->StopMixing();
}

CCoinJoinClientManager* CoinJoinWalletManager::Get(const std::string& name) const
{
    LOCK(cs_wallet_manager_map);
    auto it = m_wallet_manager_map.find(name);
    return (it != m_wallet_manager_map.end()) ? it->second.get() : nullptr;
}
