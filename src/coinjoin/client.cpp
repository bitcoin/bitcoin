// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/client.h>

#include <coinjoin/options.h>
#include <evo/deterministicmns.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <util/helpers.h>
#include <wallet/coinjoin.h>

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <core_io.h>
#include <net.h>
#include <netmessagemaker.h>
#include <shutdown.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/fs_helpers.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <version.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/walletdb.h>

#include <memory>
#include <optional>
#include <ranges>

#include <univalue.h>

using wallet::CCoinControl;
using wallet::CompactTallyItem;
using wallet::COutput;
using wallet::CoinType;
using wallet::CWallet;
using wallet::ReserveDestination;

CCoinJoinClientManager::CCoinJoinClientManager(const std::shared_ptr<wallet::CWallet>& wallet,
                                               CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                                               const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                               CoinJoinQueueManager* queueman) :
    m_wallet{wallet},
    m_dmnman{dmnman},
    m_mn_metaman{mn_metaman},
    m_mn_sync{mn_sync},
    m_isman{isman},
    m_queueman{queueman}
{
}

CCoinJoinClientManager::~CCoinJoinClientManager() = default;

void CCoinJoinClientManager::ProcessMessage(CNode& peer, Chainstate& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return;
    if (!m_mn_sync.IsBlockchainSynced()) return;

    if (!CheckDiskSpace(gArgs.GetDataDirNet())) {
        resetPool();
        stopMixing();
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
                                               const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman) :
    m_wallet(wallet),
    m_clientman(clientman),
    m_dmnman(dmnman),
    m_mn_metaman(mn_metaman),
    m_mn_sync(mn_sync),
    m_isman{isman}
{}

void CCoinJoinClientSession::ProcessMessage(CNode& peer, Chainstate& active_chainstate, CConnman& connman, const CTxMemPool& mempool, std::string_view msg_type, CDataStream& vRecv)
{
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

bool CCoinJoinClientManager::startMixing() {
    return m_wallet->StartMixing();
}

void CCoinJoinClientManager::stopMixing() {
    m_wallet->StopMixing();
}

bool CCoinJoinClientManager::isMixing() const
{
    return m_wallet->IsMixing();
}

void CCoinJoinClientSession::ResetPool()
{
    txMyCollateral = CMutableTransaction();
    UnlockCoins();
    WITH_LOCK(m_wallet->cs_wallet, keyHolderStorage.ReturnAll());
    WITH_LOCK(cs_coinjoin, SetNull());
}

void CCoinJoinClientManager::resetPool()
{
    nCachedLastSuccessBlock = 0;
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

    // Post-V24: rebalance inputs are recorded in vecOutPointLocked at lock time
    // (SelectRebalanceInputs), so UnlockCoins() releases them - just clear the bookkeeping
    m_fPromotion = false;
    m_fDemotion = false;
    m_vecRebalanceInputs.clear();

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

std::vector<std::string> CCoinJoinClientManager::getSessionStatuses() const
{
    AssertLockNotHeld(cs_deqsessions);

    bool fWaitForBlock{WaitForAnotherBlock()};
    std::vector<std::string> ret;

    LOCK(cs_deqsessions);
    ret.reserve(deqSessions.size());
    for (const auto& session : deqSessions) {
        ret.push_back(session.GetStatus(fWaitForBlock).original);
    }
    return ret;
}

std::string CCoinJoinClientManager::getSessionDenoms() const
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

    if (!CCoinJoinClientOptions::IsEnabled() || !isMixing()) return;

    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        session.CheckTimeout();
    }
}

//
// Execute a mixing denomination via a Masternode.
// This is only ran from clients
//
bool CCoinJoinClientSession::SendDenominate(const std::vector<std::pair<CTxDSIn, CTxOut> >& vecPSInOutPairsIn, CConnman& connman)
{
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
        // For promotion/demotion, filter out empty inputs/outputs
        // Promotion: 10 inputs with only 1 real output (others are empty)
        // Demotion: 1 input with 10 outputs (only first has real input)
        if (!txDsIn.prevout.IsNull()) {
            vecTxDSInTmp.emplace_back(txDsIn);
            tx.vin.emplace_back(txDsIn);
        }
        if (txOut.nValue > 0) {
            vecTxOutTmp.emplace_back(txOut);
            tx.vout.emplace_back(txOut);
        }
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SendDenominate -- Submitting partial tx with %d inputs, %d outputs: %s\n",
                     vecTxDSInTmp.size(), vecTxOutTmp.size(), tx.ToString());

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
bool CCoinJoinClientSession::SignFinalTransaction(CNode& peer, Chainstate& active_chainstate, CConnman& connman, const CTxMemPool& mempool, const CTransaction& finalTransactionNew)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    if (!mixingMasternode) return false;

    // Evaluated before taking cs_wallet (IsPromotionDemotionActive locks cs_main).
    // A session we joined for 1:1 mixing can still admit a rebalance participant, so the final
    // tx may legitimately be unbalanced while our own tip is one block short of V24 - refusing
    // to sign it would cost us our collateral. Further behind than that is our own problem.
    const bool fRebalanceShapesPossible =
        CoinJoin::IsPromotionDemotionActive(active_chainstate.m_chainman, /*fNextBlock=*/true);

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
    CoinJoin::SessionDenomCounts denomCounts;
    if (!IsValidInOuts(active_chainstate, m_isman, mempool, finalMutableTransaction.vin, finalMutableTransaction.vout,
                       nSessionDenom, /*fAllowRebalanceShapes=*/fRebalanceShapesPossible, nMessageID, nullptr, /*fFinalTx=*/true, &denomCounts)) {
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
            bool fFound = std::ranges::any_of(finalMutableTransaction.vout,
                                              [&txout](const auto& txoutFinal) { return txoutFinal == txout; });
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

            for (const auto i : util::irange(finalMutableTransaction.vin.size())) {
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

    // Post-V24: refuse to sign unless somebody else holds coins at the session denomination on
    // whichever side we do. Only same-denomination coins conceal ours - coins at the larger
    // adjacent denomination are told apart by amount - so a malicious masternode could
    // otherwise pad the transaction with larger-denom coins and publish one that links our ten
    // promotion inputs, or our ten demotion outputs, to each other in plain sight. The server
    // never finalizes a session where a side has a lone occupant, so this never rejects an
    // honest final transaction.
    if (fRebalanceShapesPossible) {
        const CAmount nSessionAmount = CoinJoin::DenominationToAmount(nSessionDenom);
        size_t nOwnSessionInputs{0};
        size_t nOwnSessionOutputs{0};
        for (const auto& entry : vecEntries) {
            // Our own promotion inputs and standard inputs are at the session denomination; a
            // demotion spends a single larger-denom coin, which needs no cover of its own.
            if (entry.GetMixShape() != CoinJoin::MixShape::DEMOTION) nOwnSessionInputs += entry.vecTxDSIn.size();
            for (const auto& txout : entry.vecTxOut) {
                if (txout.nValue == nSessionAmount) ++nOwnSessionOutputs;
            }
        }

        const bool fInputsCovered = nOwnSessionInputs == 0 || denomCounts.inputs > nOwnSessionInputs;
        const bool fOutputsCovered = nOwnSessionOutputs == 0 || denomCounts.outputs > nOwnSessionOutputs;
        if (!fInputsCovered || !fOutputsCovered) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- no cover at the session denom (inputs %d/%d, outputs %d/%d), refusing to sign!\n",
                             __func__, nOwnSessionInputs, denomCounts.inputs, nOwnSessionOutputs, denomCounts.outputs);
            UnlockCoins();
            keyHolderStorage.ReturnAll();
            SetNull();
            return false;
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
    if (nMessageID == MSG_SUCCESS) {
        m_clientman.UpdatedSuccessBlock();
        keyHolderStorage.KeepAll();
        // Our inputs are now committed to the finalized mixing transaction but the
        // DSCOMPLETE message may arrive well before the (trickle-relayed) DSTX does.
        // Do not release those inputs yet: keep them locked until the wallet actually
        // observes a transaction spending them, otherwise another session could pick
        // them up in the meantime (esp. with -coinjoinmultisession) and double-spend
        // them. Anything else we locked (e.g. collateral inputs which may never be
        // spent) is unlocked immediately as before.
        std::set<COutPoint> setMixingInputs;
        {
            LOCK(cs_coinjoin);
            for (const auto& entry : vecEntries) {
                for (const auto& txdsin : entry.vecTxDSIn) {
                    setMixingInputs.emplace(txdsin.prevout);
                }
            }
        }
        std::vector<COutPoint> vecPending;
        std::vector<COutPoint> vecUnlockNow;
        for (const auto& outpoint : vecOutPointLocked) {
            if (setMixingInputs.count(outpoint)) {
                vecPending.push_back(outpoint);
            } else {
                vecUnlockNow.push_back(outpoint);
            }
        }
        vecOutPointLocked = std::move(vecUnlockNow);
        m_clientman.AddPendingObservation(vecPending);
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
    nCachedLastSuccessBlock = nCachedBlockHeight;
}

// NOTE: the record which owns the locks and the persistent locks themselves are
// separate writes and each write is its own implicit transaction, so commit them in
// one explicit database transaction: a crash in between must neither leave
// persistently locked coins behind with nothing tracking them (nothing would ever
// release them again) nor persist the record without its locks (on restart the
// missing lock reads as a manual unlock, the entry is dropped and the input becomes
// selectable again while the finalized mixing transaction may still be in flight).
// If no transaction can be started, don't persist anything rather than risk exactly
// those partial states. The locks of all entries are written, not only of the ones
// just added: entries added while nothing could be persisted are locked in memory only.
// A lock released manually in the meantime is left alone, the next check drops its entry.
static bool PersistPendingObservations(const CWallet& wallet, wallet::WalletBatch& batch,
                                       const std::map<COutPoint, int64_t>& pending)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    if (!batch.TxnBegin()) return false;
    bool fPersisted{batch.WriteCoinJoinPendingObs(pending)};
    for (auto it = pending.begin(); fPersisted && it != pending.end(); ++it) {
        if (wallet.IsLockedCoin(it->first)) fPersisted = batch.WriteLockedUTXO(it->first);
    }
    if (!fPersisted) {
        batch.TxnAbort();
        return false;
    }
    return batch.TxnCommit();
}

void CCoinJoinClientManager::AddPendingObservation(const std::vector<COutPoint>& outpoints)
{
    AssertLockNotHeld(cs_pending_obs);
    if (outpoints.empty()) return;

    LOCK(m_wallet->cs_wallet);
    LOCK(cs_pending_obs);
    wallet::WalletBatch batch(m_wallet->GetDatabase());
    LoadPendingObservations(batch);

    const int64_t nNow{GetTime()};
    for (const auto& outpoint : outpoints) {
        m_pending_obs.emplace(outpoint, nNow);
        // The coins are locked in memory already (see PrepareDenominate), the lock is
        // persisted below so that a restart before the finalized transaction is observed
        // cannot make the input available for selection again
        m_wallet->LockCoin(outpoint);
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- %s is locked until the finalized mixing transaction is observed\n",
                         __func__, outpoint.ToStringShort());
    }
    // Never overwrite a record which could not be read (LoadPendingObservations() left
    // m_pending_obs_loaded unset), that would permanently orphan the locks it still tracks
    m_pending_obs_dirty = !m_pending_obs_loaded || !PersistPendingObservations(*m_wallet, batch, m_pending_obs);
    if (m_pending_obs_dirty) {
        // The in-memory lock still protects these inputs for as long as this process
        // runs, but a restart before CheckPendingObservations() manages to persist them
        // would make them selectable again while a valid mixing transaction spending them
        // may already be in flight. Nothing more we can do about it here beyond making
        // the failure loud - the wallet database is broken.
        LogPrintf("CCoinJoinClientManager::%s -- ERROR: failed to persist locks for %d successfully mixed input(s), " /* Continued */
                  "they will not survive a restart until this succeeds\n",
                  __func__, outpoints.size());
    }
}

void CCoinJoinClientManager::LoadPendingObservations(wallet::WalletBatch& batch)
{
    AssertLockHeld(cs_pending_obs);

    if (m_pending_obs_loaded) return;
    // Missing record simply means nothing was pending when we last shut down.
    // NOTE: this only tells a missing record apart from one which exists but cannot be
    // deserialized, which is the realistic failure. It cannot tell a missing record apart
    // from a database error while looking for it: both BerkeleyBatch::HasKey() and
    // SQLiteBatch::HasKey() report anything which is not a hit as a miss, so an error
    // there still reads as "nothing was pending". Telling those apart would need a
    // tri-state DatabaseBatch API for both backends, which is not worth it for this.
    if (!batch.HasCoinJoinPendingObs()) {
        m_pending_obs_loaded = true;
        return;
    }
    // Read into a scratch map: a failed read of an existing record must not be mistaken
    // for an empty one, or the persisted locks it tracks would be left with nothing to
    // ever release them, and must not leave partially deserialized entries behind
    // either. Not marking the state loaded retries the read on the next pass and, more
    // importantly, keeps everything else from overwriting the record before its
    // contents have been recovered.
    std::map<COutPoint, int64_t> pending;
    if (!batch.ReadCoinJoinPendingObs(pending)) {
        // Only complain once: a corrupt (as opposed to transiently unreadable) record
        // never becomes readable, and the retry runs for as long as this node does.
        if (!m_pending_obs_load_failed) {
            m_pending_obs_load_failed = true;
            LogPrintf("CCoinJoinClientManager::%s -- ERROR: failed to read the pending observation record, will keep " /* Continued */
                      "retrying silently. The inputs it tracks stay locked until it can be read, `lockunspent` "
                      "releases them manually\n",
                      __func__);
        }
        return;
    }
    // Keep any entries added in memory while the record was unreadable
    m_pending_obs.insert(pending.begin(), pending.end());
    m_pending_obs_loaded = true;
    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- loaded %d pending observation(s)\n", __func__,
                     pending.size());
}

void CCoinJoinClientManager::CheckPendingObservations(const CTxMemPool& mempool)
{
    AssertLockNotHeld(cs_pending_obs);

    // nothing to do in the common case, don't touch the wallet at all
    if (WITH_LOCK(cs_pending_obs, return m_pending_obs_loaded && m_pending_obs.empty() && !m_pending_obs_dirty)) return;

    LOCK(m_wallet->cs_wallet);
    LOCK(cs_pending_obs);

    // Entries which could not be persisted before (e.g. added while the record could not
    // be read, see AddPendingObservation()) are locked in memory only, retry persisting them
    bool fChanged{m_pending_obs_dirty};
    if (!m_pending_obs_loaded) {
        // Read-only, no need to checkpoint the database on the way out
        wallet::WalletBatch batch_load(m_wallet->GetDatabase(), /*_fFlushOnClose=*/false);
        LoadPendingObservations(batch_load);
    }

    // Constructed on the first write only: this keeps running for as long as anything is
    // pending and every WalletBatch checkpoints the wallet database when it goes out of
    // scope, which is far too expensive to pay for a pass which changes nothing.
    std::optional<wallet::WalletBatch> batch;
    const auto get_batch = [&]() -> wallet::WalletBatch& {
        if (!batch.has_value()) batch.emplace(m_wallet->GetDatabase());
        return batch.value();
    };

    const int64_t nNow{GetTime()};
    const bool fSynced{m_mn_sync.IsBlockchainSynced()};
    for (auto it = m_pending_obs.begin(); it != m_pending_obs.end();) {
        const COutPoint& outpoint = it->first;
        if (!m_wallet->IsLockedCoin(outpoint)) {
            // The user released the lock manually (e.g. via lockunspent), respect that
            it = m_pending_obs.erase(it);
            fChanged = true;
            continue;
        }
        if (m_wallet->IsSpent(outpoint)) {
            // The wallet observed a transaction spending this input, it is no longer
            // selectable anyway, drop the protective lock
            WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- observed spend of %s, releasing lock\n", __func__,
                             outpoint.ToStringShort());
            m_wallet->UnlockCoin(outpoint, &get_batch());
            it = m_pending_obs.erase(it);
            fChanged = true;
            continue;
        }
        // Only ever consult chain/mempool spentness on a synced chain: while catching up
        // the spending transaction may sit in a block we have not downloaded yet, and
        // releasing the input on the strength of that would defeat the whole point
        if (fSynced && nNow - it->second >= COINJOIN_PENDING_OBSERVATION_TIMEOUT) {
            // NOTE: findCoins() reports outputs which exist in the chain UTXO set or are
            // created by a mempool transaction; it does NOT know whether a mempool
            // transaction spends them (CCoinsViewMemPool::GetCoin never looks at
            // mapNextTx). The finalized mixing transaction sitting unprocessed in our own
            // mempool is exactly the case we must not misread as "never propagated", so
            // ask the mempool about spentness explicitly as well.
            std::map<COutPoint, Coin> coins{{outpoint, Coin{}}};
            m_wallet->chain().findCoins(coins);
            if (!coins.at(outpoint).IsSpent() && !mempool.isSpent(outpoint)) {
                // Still unspent in chain and mempool long after the session completed -
                // the finalized transaction most likely never propagated, release the
                // input so the wallet does not lose it forever
                LogPrintf("CCoinJoinClientManager::%s -- WARNING: never observed finalized mixing transaction for %s, " /* Continued */
                          "releasing lock after %d seconds\n",
                          __func__, outpoint.ToStringShort(), COINJOIN_PENDING_OBSERVATION_TIMEOUT);
                m_wallet->UnlockCoin(outpoint, &get_batch());
                it = m_pending_obs.erase(it);
                fChanged = true;
                continue;
            }
            // Spent according to chain/mempool but the wallet has not recorded the
            // spending transaction yet, keep waiting for it. Note that this can be
            // terminal: if the wallet never learns about the spend (restored from an old
            // backup and never rescanned, say) the entry stays for good. Releasing it
            // would be wrong - such a coin still looks unspent to the wallet and could be
            // selected for another session - so only refresh the timer to keep the check
            // above from running on every pass. The refresh is deliberately not persisted,
            // the worst a restart can do is re-run the check once.
            WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::%s -- %s is spent in chain/mempool but not according to " /* Continued */
                                       "the wallet, keeping it locked\n",
                             __func__, outpoint.ToStringShort());
            it->second = nNow;
        }
        ++it;
    }
    // Never overwrite a record which could not be read (m_pending_obs_loaded unset),
    // it may still track locks nothing else would ever release. An entry released
    // above but left in such a record self-heals: once the record is readable again
    // the entry reloads, its coin is no longer locked and it is dropped right here.
    if (fChanged && m_pending_obs_loaded) {
        const bool fPersisted{PersistPendingObservations(*m_wallet, get_batch(), m_pending_obs)};
        // Retried on every pass until it succeeds, only complain when it starts failing
        if (!fPersisted && !m_pending_obs_dirty) {
            LogPrintf("CCoinJoinClientManager::%s -- ERROR: failed to persist %d pending observation(s), " /* Continued */
                      "will keep retrying\n",
                      __func__, m_pending_obs.size());
        }
        m_pending_obs_dirty = !fPersisted;
    }
}

bool CCoinJoinClientManager::IsPendingObservation(const COutPoint& outpoint) const
{
    AssertLockNotHeld(cs_pending_obs);
    LOCK(cs_pending_obs);
    return m_pending_obs.count(outpoint) > 0;
}

size_t CCoinJoinClientManager::GetPendingObservationCount() const
{
    AssertLockNotHeld(cs_pending_obs);
    LOCK(cs_pending_obs);
    return m_pending_obs.size();
}

bool CCoinJoinClientManager::WaitForAnotherBlock() const
{
    if (!m_mn_sync.IsBlockchainSynced()) return true;

    if (CCoinJoinClientOptions::IsMultiSessionEnabled()) return false;

    return nCachedBlockHeight - nCachedLastSuccessBlock < nMinBlocksToWait;
}

bool CCoinJoinClientManager::CheckAutomaticBackup()
{
    if (!CCoinJoinClientOptions::IsEnabled() || !isMixing()) return false;

    // We don't need auto-backups for descriptor wallets
    if (!m_wallet->IsLegacy()) return true;

    switch (nWalletBackups) {
    case 0:
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- Automatic backups disabled, no mixing available.\n");
        stopMixing();
        m_wallet->nKeysLeftSinceAutoBackup = 0; // no backup, no "keys since last backup"
        return false;
    case -1:
        // Automatic backup failed, nothing else we can do until user fixes the issue manually.
        // There is no way to bring user attention in daemon mode, so we just
        // keep spamming if debug is on.
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- ERROR! Failed to create automatic backup, see debug.log for details.\n");
        return false;
    case -2:
        // We were able to create automatic backup but keypool was not replenished because wallet is locked.
        // There is no way to bring user attention in daemon mode, so we just
        // keep spamming if debug is on.
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- WARNING! Failed to replenish keypool, please unlock your wallet to do so, see debug.log for details.\n");
        return false;
    }

    if (m_wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_STOP) {
        // We should never get here via mixing itself but probably something else is still actively using keypool
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- Very low number of keys left: %d, no mixing available.\n", m_wallet->nKeysLeftSinceAutoBackup);
        // It's getting really dangerous, stop mixing
        stopMixing();
        return false;
    } else if (m_wallet->nKeysLeftSinceAutoBackup < COINJOIN_KEYS_THRESHOLD_WARNING) {
        // Low number of keys left, but it's still more or less safe to continue
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- Very low number of keys left: %d\n", m_wallet->nKeysLeftSinceAutoBackup);

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
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::CheckAutomaticBackup -- ERROR! Failed to create automatic backup: %s\n", errorString.original);
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
    if (nState != POOL_STATE_IDLE) return false;

    if (!m_mn_sync.IsBlockchainSynced()) {
        strAutoDenomResult = _("Can't mix while sync in progress.");
        return false;
    }

    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    // Post-V24: whether denomination promotion/demotion is active. Evaluated before taking
    // cs_wallet (IsPromotionDemotionActive locks cs_main).
    const bool fV24Active = CoinJoin::IsPromotionDemotionActive(chainman);

    CAmount nBalanceNeedsAnonymized;
    struct Rebalance {
        int nDenom; //!< session denomination: the smaller of the two, which both conversions run at
        bool fPromotion;
    };
    std::vector<Rebalance> vecRebalances; // post-V24 promotion/demotion opportunities

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

        if (m_dmnman.GetListAtChainTip().GetCounts().enabled() == 0 &&
            Params().NetworkIDString() != CBaseChainParams::REGTEST) {
            strAutoDenomResult = _("No Masternodes detected.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- %s\n", strAutoDenomResult.original);
            return false;
        }

        const auto bal = GetBalance(*m_wallet);

        // check if there is anything left to do
        CAmount nBalanceAnonymized = bal.m_anonymized;
        nBalanceNeedsAnonymized = CCoinJoinClientOptions::GetAmount() * COIN - nBalanceAnonymized;

        // Post-V24: check for promotion/demotion opportunities before the "nothing to do"
        // exits below - rebalancing matters precisely when the anonymization target is
        // reached and all coins are already fully mixed
        if (fV24Active) {
            // Single wallet scan covering all denominations, reused across every
            // adjacent-pair check instead of re-scanning per pair.
            const auto denomCounts = m_wallet->GetDenominationCounts();
            // Denominations: 10, 1, 0.1, 0.01, 0.001 (indices 0-4, smaller index = larger denom)
            for (size_t i = 0; i + 1 < CoinJoin::vecStandardDenominations.size(); ++i) {
                const int nLargerDenom = 1 << i;
                const int nSmallerDenom = 1 << (i + 1);
                if (m_clientman.ShouldPromote(nSmallerDenom, nLargerDenom, denomCounts)) {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- Promotion opportunity: %d x %s -> 1 x %s\n",
                        CoinJoin::PROMOTION_RATIO,
                        CoinJoin::DenominationToString(nSmallerDenom),
                        CoinJoin::DenominationToString(nLargerDenom));
                    vecRebalances.push_back({nSmallerDenom, /*fPromotion=*/true});
                }
                if (m_clientman.ShouldDemote(nLargerDenom, nSmallerDenom, denomCounts)) {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::DoAutomaticDenominating -- Demotion opportunity: 1 x %s -> %d x %s\n",
                        CoinJoin::DenominationToString(nLargerDenom),
                        CoinJoin::PROMOTION_RATIO,
                        CoinJoin::DenominationToString(nSmallerDenom));
                    vecRebalances.push_back({nSmallerDenom, /*fPromotion=*/false});
                }
            }
        }
        const bool fRebalanceOpportunity{!vecRebalances.empty()};

        if (nBalanceNeedsAnonymized < 0 && !fRebalanceOpportunity) {
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

        // mixable balance is way too small; note that fully-mixed coins don't count as
        // anonymizable, so a rebalance opportunity must bypass this check too
        if (nBalanceAnonymizable < nValueMin && !fRebalanceOpportunity) {
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

    // Post-V24: joining an existing queue at a conversion's denomination is cheap, but a new
    // promotion/demotion queue has to wait for participants that may never come (pre-rebalance
    // clients can't join it). Only start one when there is no standard mixing to do, or a lasting
    // rebalance opportunity would keep the wallet from ever mixing again.
    for (const auto& [nDenom, fPromotion] : vecRebalances) {
        if (JoinExistingQueue(nBalanceNeedsAnonymized, connman, nDenom, fPromotion, !fPromotion)) return true;
    }

    // Standard mixing only makes sense while there is still balance to anonymize; a
    // rebalance-only pass (target reached) must not fall through to standard queues
    if (nBalanceNeedsAnonymized > 0) {
        // Always attempt to join an existing queue
        if (JoinExistingQueue(nBalanceNeedsAnonymized, connman)) {
            return true;
        }

        // If we were unable to find/join an existing queue then start a new one.
        if (StartNewQueue(nBalanceNeedsAnonymized, connman)) return true;
    }

    for (const auto& [nDenom, fPromotion] : vecRebalances) {
        if (StartNewQueue(nBalanceNeedsAnonymized, connman, nDenom, fPromotion, !fPromotion)) return true;
    }

    strAutoDenomResult = _("No compatible Masternode found.");
    return false;
}

bool CCoinJoinClientManager::DoAutomaticDenominating(ChainstateManager& chainman, CConnman& connman,
                                                     const CTxMemPool& mempool, bool fDryRun)
{
    if (!CCoinJoinClientOptions::IsEnabled() || !isMixing()) return false;

    if (!m_mn_sync.IsBlockchainSynced()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::DoAutomaticDenominating -- Can't mix while sync in progress.\n");
        return false;
    }

    if (!fDryRun && m_wallet->IsLocked(true)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::DoAutomaticDenominating -- Wallet is locked.\n");
        return false;
    }

    int nMnCountEnabled = m_dmnman.GetListAtChainTip().GetCounts().enabled();

    // If we've used 90% of the Masternode list then drop the oldest first ~30%
    int nThreshold_high = nMnCountEnabled * 0.9;
    int nThreshold_low = nThreshold_high * 0.7;
    size_t used_count{m_mn_metaman.GetUsedMasternodesCount()};

    WalletCJLogPrint(m_wallet, "Checking threshold - used: %d, threshold: %d\n", static_cast<int>(used_count), nThreshold_high);

    if (static_cast<int>(used_count) > nThreshold_high) {
        m_mn_metaman.RemoveUsedMasternodes(used_count - nThreshold_low);
        WalletCJLogPrint(m_wallet, "  new used: %d, threshold: %d\n", static_cast<int>(m_mn_metaman.GetUsedMasternodesCount()),
                         nThreshold_high);
    }

    bool fResult = true;
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    if (int(deqSessions.size()) < CCoinJoinClientOptions::GetSessions()) {
        deqSessions.emplace_back(m_wallet, *this, m_dmnman, m_mn_metaman, m_mn_sync, m_isman);
    }
    for (auto& session : deqSessions) {
        if (!CheckAutomaticBackup()) return false;

        if (WaitForAnotherBlock()) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientManager::DoAutomaticDenominating -- Last successful action was too recent.\n");
            return false;
        }

        fResult &= session.DoAutomaticDenominating(chainman, connman, mempool, fDryRun);
    }

    return fResult;
}

CDeterministicMNCPtr CCoinJoinClientSession::GetRandomNotUsedMasternode()
{
    auto mnList = m_dmnman.GetListAtChainTip();

    size_t nCountEnabled = mnList.GetCounts().enabled();
    size_t nCountNotExcluded{nCountEnabled - m_mn_metaman.GetUsedMasternodesCount()};

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- %d enabled masternodes, %d masternodes to choose from\n", __func__, nCountEnabled, nCountNotExcluded);
    if (nCountNotExcluded < 1) {
        return nullptr;
    }

    // fill a vector
    std::vector<CDeterministicMNCPtr> vpMasternodesShuffled;
    vpMasternodesShuffled.reserve(nCountEnabled);
    mnList.ForEachMNShared(/*onlyValid=*/true,
                           [&vpMasternodesShuffled](const auto& dmn) { vpMasternodesShuffled.emplace_back(dmn); });

    // shuffle pointers
    Shuffle(vpMasternodesShuffled.begin(), vpMasternodesShuffled.end(), FastRandomContext());

    // loop through - using direct O(1) lookup instead of creating a set copy
    for (const auto& dmn : vpMasternodesShuffled) {
        if (m_mn_metaman.IsUsedMasternode(dmn->proTxHash)) {
            continue;
        }

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- found, masternode=%s\n", __func__,
                         dmn->proTxHash.ToString());
        return dmn;
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- failed\n", __func__);
    return nullptr;
}

static int WinnersToSkip()
{
    return (Params().NetworkIDString() == CBaseChainParams::DEVNET ||
            Params().NetworkIDString() == CBaseChainParams::REGTEST)
            ? 1 : 8;
}

bool CCoinJoinClientSession::SelectRebalanceInputs(int nTargetDenom, bool fPromotion, std::vector<CTxDSIn>& vecTxDSInRet)
{
    vecTxDSInRet.clear();
    m_vecRebalanceInputs.clear();

    if (fPromotion) {
        // Promotion: select 10 fully-mixed coins of the smaller denomination
        auto vecCoins = m_wallet->SelectFullyMixedForPromotion(nTargetDenom, CoinJoin::PROMOTION_RATIO);
        if (static_cast<int>(vecCoins.size()) < CoinJoin::PROMOTION_RATIO) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Not enough fully-mixed coins for promotion\n", __func__);
            return false;
        }
        // Convert COutPoints to CTxDSIn
        LOCK(m_wallet->cs_wallet);
        for (const auto& outpoint : vecCoins) {
            const auto it = m_wallet->mapWallet.find(outpoint.hash);
            if (it == m_wallet->mapWallet.end()) continue;
            const wallet::CWalletTx& wtx = it->second;
            if (outpoint.n >= wtx.tx->vout.size()) {
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- invalid outpoint index %u for tx %s\n", __func__, outpoint.n, outpoint.hash.ToString());
                continue;
            }
            vecTxDSInRet.emplace_back(CTxIn(outpoint), wtx.tx->vout[outpoint.n].scriptPubKey,
                                      m_wallet->GetRealOutpointCoinJoinRounds(outpoint));
        }
        if (static_cast<int>(vecTxDSInRet.size()) < CoinJoin::PROMOTION_RATIO) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Failed to build promotion inputs\n", __func__);
            vecTxDSInRet.clear();
            return false;
        }
    } else {
        // Demotion: select 1 fully-mixed coin of the larger adjacent denomination. Like
        // promotion inputs, the coin's history must already be protected: the demotion's
        // 1:10 shape publicly clusters its outputs, which therefore start mixing over
        // (see GetRealOutpointCoinJoinRounds), and only a fully-mixed input is worth that.
        const int nLargerDenom = CoinJoin::GetLargerAdjacentDenom(nTargetDenom);
        if (nLargerDenom == 0) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- No larger adjacent denom for demotion\n", __func__);
            return false;
        }
        if (!m_wallet->SelectTxDSInsByDenomination(nLargerDenom, CoinJoin::DenominationToAmount(nLargerDenom), vecTxDSInRet, CoinType::ONLY_FULLY_MIXED)) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- Couldn't find coin for demotion\n", __func__);
            return false;
        }
        // Keep only 1 input for demotion
        vecTxDSInRet.resize(1);
    }

    // Lock the selected coins immediately to prevent races with other concurrent CoinJoin
    // sessions and record them so UnlockCoins() releases them on any failure path
    LOCK(m_wallet->cs_wallet);
    for (const auto& txdsin : vecTxDSInRet) {
        m_wallet->LockCoin(txdsin.prevout);
        m_vecRebalanceInputs.push_back(txdsin.prevout);
        vecOutPointLocked.push_back(txdsin.prevout);
    }
    return true;
}

void CCoinJoinClientSession::UnlockRebalanceInputs()
{
    if (m_vecRebalanceInputs.empty()) return;
    {
        LOCK(m_wallet->cs_wallet);
        for (const auto& outpoint : m_vecRebalanceInputs) {
            m_wallet->UnlockCoin(outpoint);
        }
    }
    vecOutPointLocked.erase(std::remove_if(vecOutPointLocked.begin(), vecOutPointLocked.end(),
                                           [&](const COutPoint& outpoint) {
                                               return std::find(m_vecRebalanceInputs.begin(), m_vecRebalanceInputs.end(),
                                                                outpoint) != m_vecRebalanceInputs.end();
                                           }),
                            vecOutPointLocked.end());
    m_vecRebalanceInputs.clear();
}

bool CCoinJoinClientSession::JoinExistingQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman,
                                                int nTargetDenom, bool fPromotion, bool fDemotion)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return false;

    // Promotion and demotion are mutually exclusive
    assert(!(fPromotion && fDemotion));

    // For promotion/demotion select and lock the inputs up front: the selection only depends
    // on the target denomination, not on the queue, so there is no need to re-select per queue
    std::vector<CTxDSIn> vecRebalanceTxDSIn;
    const bool fRebalance = (fPromotion || fDemotion) && nTargetDenom != 0;
    if (fRebalance && !SelectRebalanceInputs(nTargetDenom, fPromotion, vecRebalanceTxDSIn)) {
        strAutoDenomResult = _("Can't mix: no compatible inputs found!");
        return false;
    }

    const auto mnList = m_dmnman.GetListAtChainTip();
    const int nWeightedMnCount = mnList.GetCounts().m_valid_weighted;

    // Look through the queues and see if anything matches
    CCoinJoinQueue dsq;
    while (m_clientman.GetQueueItemAndTry(dsq, fRebalance ? nTargetDenom : 0)) {
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

        if (!fRebalance) {
            // Standard mixing: try to match their denominations if possible
            if (!m_wallet->SelectTxDSInsByDenomination(dsq.nDenom, nBalanceNeedsAnonymized, vecTxDSInTmp)) {
                WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- Couldn't match denomination %d (%s)\n", dsq.nDenom, CoinJoin::DenominationToString(dsq.nDenom));
                continue;
            }
        }

        m_mn_metaman.AddUsedMasternode(dmn->proTxHash);

        if (connman.IsMasternodeOrDisconnectRequested(dmn->pdmnState->netInfo->GetPrimary())) {
            WalletCJLogPrint(m_wallet, /* Continued */
                             "CCoinJoinClientSession::JoinExistingQueue -- skipping connection, masternode=%s\n", dmn->proTxHash.ToString());
            continue;
        }

        nSessionDenom = dsq.nDenom;
        mixingMasternode = dmn;
        // Declare which side of the session denomination we will occupy so the masternode can
        // tell whether the session can still cover both sides (rebalance sessions only)
        const uint8_t nDsaFlags = fPromotion ? CCoinJoinAccept::FLAG_PROMOTION
                                             : fDemotion ? CCoinJoinAccept::FLAG_DEMOTION : uint8_t{0};
        pendingDsaRequest = CPendingDsaRequest(dmn->proTxHash,
                                               CCoinJoinAccept(nSessionDenom, txMyCollateral, nDsaFlags));
        connman.AddPendingMasternode(dmn->proTxHash);
        SetState(POOL_STATE_QUEUE);
        nTimeLastSuccessfulStep = GetTime();
        // Set promotion/demotion session state; the rebalance inputs (if any) were already
        // selected, locked and recorded by SelectRebalanceInputs above
        m_fPromotion = fPromotion;
        m_fDemotion = fDemotion;
        if (!fRebalance) m_vecRebalanceInputs.clear();

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::JoinExistingQueue -- pending %s connection, masternode=%s, nSessionDenom=%d (%s), %d inputs\n",
                         fPromotion ? "PROMOTION" : fDemotion ? "DEMOTION" : "mixing",
                         dmn->proTxHash.ToString(), nSessionDenom.load(), CoinJoin::DenominationToString(nSessionDenom),
                         vecTxDSInTmp.size() + m_vecRebalanceInputs.size());
        strAutoDenomResult = _("Trying to connect…");
        return true;
    }
    strAutoDenomResult = _("Failed to find mixing queue to join");
    UnlockRebalanceInputs();
    return false;
}

bool CCoinJoinClientSession::StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman)
{
    assert(m_mn_metaman.IsValid());

    if (!CCoinJoinClientOptions::IsEnabled()) return false;
    if (nBalanceNeedsAnonymized <= 0) return false;

    int nTries = 0;
    const auto mnList = m_dmnman.GetListAtChainTip();
    const auto mnCounts = mnList.GetCounts();
    const int nMnCount = mnCounts.enabled();
    const int nWeightedMnCount = mnCounts.m_valid_weighted;

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
        auto dmn = GetRandomNotUsedMasternode();
        if (!dmn) {
            strAutoDenomResult = _("Can't find random Masternode.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- %s\n", strAutoDenomResult.original);
            return false;
        }

        m_mn_metaman.AddUsedMasternode(dmn->proTxHash);

        // skip next mn payments winners
        if (dmn->pdmnState->nLastPaidHeight + nWeightedMnCount < mnList.GetHeight() + WinnersToSkip()) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- skipping winner, masternode=%s\n", dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        if (m_mn_metaman.IsMixingThresholdExceeded(dmn->proTxHash, nMnCount)) {
            WalletCJLogPrint(m_wallet, /* Continued */
                             "CCoinJoinClientSession::StartNewQueue -- too early to mix with node masternode=%s\n",
                             dmn->proTxHash.ToString());
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
        WalletCJLogPrint(/* Continued */
                         m_wallet,
                         "CCoinJoinClientSession::StartNewQueue -- pending connection, masternode=%s, nSessionDenom=%d "
                         "(%s)\n",
                         dmn->proTxHash.ToString(), nSessionDenom.load(), CoinJoin::DenominationToString(nSessionDenom));
        strAutoDenomResult = _("Trying to connect…");
        return true;
    }
    strAutoDenomResult = _("Failed to start a new mixing queue");
    return false;
}

bool CCoinJoinClientSession::StartNewQueue(CAmount nBalanceNeedsAnonymized, CConnman& connman,
                                           int nTargetDenom, bool fPromotion, bool fDemotion)
{
    assert(m_mn_metaman.IsValid());

    if (!CCoinJoinClientOptions::IsEnabled()) return false;
    if (nTargetDenom == 0) return false;

    // Promotion and demotion are mutually exclusive, and this overload needs one of them
    assert(!(fPromotion && fDemotion));
    if (!fPromotion && !fDemotion) return false;

    // For promotion/demotion, verify we have the required coins before starting a queue.
    // SelectRebalanceInputs locks them and records them in vecOutPointLocked/m_vecRebalanceInputs.
    std::vector<CTxDSIn> vecTxDSInTmp;
    if (!SelectRebalanceInputs(nTargetDenom, fPromotion, vecTxDSInTmp)) {
        return false;
    }

    int nTries = 0;
    const auto mnList = m_dmnman.GetListAtChainTip();
    const auto mnCounts = mnList.GetCounts();
    const int nMnCount = mnCounts.enabled();
    const int nWeightedMnCount = mnCounts.m_valid_weighted;

    while (nTries < 10) {
        auto dmn = GetRandomNotUsedMasternode();
        if (!dmn) {
            strAutoDenomResult = _("Can't find random Masternode.");
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- %s\n", strAutoDenomResult.original);
            UnlockRebalanceInputs();
            return false;
        }

        m_mn_metaman.AddUsedMasternode(dmn->proTxHash);

        // skip next mn payments winners
        if (dmn->pdmnState->nLastPaidHeight + nWeightedMnCount < mnList.GetHeight() + WinnersToSkip()) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- skipping winner, masternode=%s\n", dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        if (m_mn_metaman.IsMixingThresholdExceeded(dmn->proTxHash, nMnCount)) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- too early to mix with node masternode=%s\n",
                             dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        if (connman.IsMasternodeOrDisconnectRequested(dmn->pdmnState->netInfo->GetPrimary())) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- skipping connection, masternode=%s\n",
                             dmn->proTxHash.ToString());
            nTries++;
            continue;
        }

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- attempting %s connection, masternode=%s, tries=%d\n",
                         fPromotion ? "PROMOTION" : "DEMOTION", dmn->proTxHash.ToString(), nTries);

        nSessionDenom = nTargetDenom;
        mixingMasternode = dmn;
        connman.AddPendingMasternode(dmn->proTxHash);
        // This overload always starts a promotion/demotion session - declare which side of the
        // session denomination we will occupy so the masternode can track session coverage
        pendingDsaRequest = CPendingDsaRequest(dmn->proTxHash,
                                               CCoinJoinAccept(nSessionDenom, txMyCollateral,
                                                               fPromotion ? CCoinJoinAccept::FLAG_PROMOTION
                                                                          : CCoinJoinAccept::FLAG_DEMOTION));
        SetState(POOL_STATE_QUEUE);
        nTimeLastSuccessfulStep = GetTime();

        // Store promotion/demotion state; the inputs were already selected, locked and
        // recorded in m_vecRebalanceInputs by SelectRebalanceInputs above
        m_fPromotion = fPromotion;
        m_fDemotion = fDemotion;

        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::StartNewQueue -- pending %s connection, masternode=%s, nSessionDenom=%d (%s), %zu inputs\n",
                         fPromotion ? "PROMOTION" : "DEMOTION",
                         dmn->proTxHash.ToString(), nSessionDenom.load(), CoinJoin::DenominationToString(nSessionDenom),
                         m_vecRebalanceInputs.size());
        strAutoDenomResult = _("Trying to connect…");
        return true;
    }
    strAutoDenomResult = _("Failed to start a new mixing queue");
    UnlockRebalanceInputs();
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
        UnlockCoins();
        WITH_LOCK(cs_coinjoin, SetNull());
        return false;
    }

    bool fMnTooOldForRebalance{false};
    bool fDone = connman.ForNode(mn_addr, [this, &connman, &fMnTooOldForRebalance](CNode* pnode) {
        // Post-V24: a rebalance dsa carries a version-gated flags field the masternode must
        // understand - and the masternode must be able to host a rebalance-capable session.
        // If it negotiated an older protocol, abort and let DoAutomaticDenominating retry
        // with another masternode instead of silently mixing without a reserved slot.
        if (pendingDsaRequest.GetDSA().IsRebalance() && pnode->GetCommonVersion() < COINJOIN_REBALANCE_VERSION) {
            fMnTooOldForRebalance = true;
            return false;
        }
        WalletCJLogPrint(m_wallet, "-- processing dsa queue for addr=%s\n", pnode->addr.ToStringAddrPort());
        nTimeLastSuccessfulStep = GetTime();
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSACCEPT, pendingDsaRequest.GetDSA()));
        return true;
    });

    if (fMnTooOldForRebalance) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- masternode too old for a rebalance session, masternode=%s\n",
                         __func__, pendingDsaRequest.GetProTxHash().ToString());
        UnlockCoins();
        WITH_LOCK(cs_coinjoin, SetNull());
        return false;
    }

    if (fDone) {
        pendingDsaRequest = CPendingDsaRequest();
    } else if (pendingDsaRequest.IsExpired()) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- failed to connect, masternode=%s\n", __func__,
                         pendingDsaRequest.GetProTxHash().ToString());
        UnlockCoins();
        WITH_LOCK(cs_coinjoin, SetNull());
    }

    return fDone;
}

void CCoinJoinClientManager::ProcessPendingDsaRequest(CConnman& connman)
{
    AssertLockNotHeld(cs_deqsessions);
    LOCK(cs_deqsessions);
    for (auto& session : deqSessions) {
        session.ProcessPendingDsaRequest(connman);
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

bool CCoinJoinClientManager::GetQueueItemAndTry(CCoinJoinQueue& dsq, int nDenomFilter) const
{
    return m_queueman && m_queueman->GetQueueItemAndTry(dsq, nDenomFilter);
}

bool CCoinJoinClientSession::SubmitDenominate(CConnman& connman)
{
    LOCK(m_wallet->cs_wallet);

    std::string strError;
    std::vector<std::pair<CTxDSIn, CTxOut> > vecPSInOutPairsTmp;

    // Post-V24: Handle promotion/demotion entries
    if (m_fPromotion || m_fDemotion) {
        const bool fPrepared = m_fPromotion ? PreparePromotionEntry(strError, vecPSInOutPairsTmp)
                                            : PrepareDemotionEntry(strError, vecPSInOutPairsTmp);
        if (fPrepared) {
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- %s entry prepared, sending\n",
                             m_fPromotion ? "Promotion" : "Demotion");
            return SendDenominate(vecPSInOutPairsTmp, connman);
        }
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- Prepare%sEntry failed: %s\n",
                         m_fPromotion ? "Promotion" : "Demotion", strError);
        strAutoDenomResult = Untranslated(strError);
        // The rebalance inputs were locked back when the session was queued; release them and
        // reset the session right away instead of keeping them locked until CheckTimeout()
        UnlockCoins();
        keyHolderStorage.ReturnAll();
        WITH_LOCK(cs_coinjoin, SetNull());
        return false;
    }

    // Standard 1:1 mixing
    std::vector<CTxDSIn> vecTxDSIn;

    if (!SelectDenominate(strError, vecTxDSIn)) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::SubmitDenominate -- SelectDenominate failed, error: %s\n", strError);
        return false;
    }

    std::vector<std::pair<int, size_t> > vecInputsByRounds;

    for (const auto i : util::irange(CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds())) {
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

    WalletCJLogPrint(m_wallet, "vecInputsByRounds for denom %d\n", nSessionDenom.load());
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

bool CCoinJoinClientSession::PreparePromotionEntry(std::string& strErrorRet, std::vector<std::pair<CTxDSIn, CTxOut>>& vecPSInOutPairsRet)
{
    AssertLockHeld(m_wallet->cs_wallet);

    vecPSInOutPairsRet.clear();

    if (m_vecRebalanceInputs.size() != static_cast<size_t>(CoinJoin::PROMOTION_RATIO)) {
        strErrorRet = strprintf("Invalid promotion input count: %d (expected %d)", m_vecRebalanceInputs.size(), CoinJoin::PROMOTION_RATIO);
        return false;
    }

    // Session denom is the smaller denom (inputs), get the larger adjacent denom for output
    const int nLargerDenom = CoinJoin::GetLargerAdjacentDenom(nSessionDenom);
    if (nLargerDenom == 0) {
        strErrorRet = "No larger adjacent denomination for promotion";
        return false;
    }
    const CAmount nLargerAmount = CoinJoin::DenominationToAmount(nLargerDenom);

    // Create 10 inputs from stored promotion inputs
    for (const auto& outpoint : m_vecRebalanceInputs) {
        const auto it = m_wallet->mapWallet.find(outpoint.hash);
        if (it == m_wallet->mapWallet.end()) {
            strErrorRet = "Promotion input not found in wallet";
            return false;
        }
        const wallet::CWalletTx& wtx = it->second;
        if (outpoint.n >= wtx.tx->vout.size()) {
            strErrorRet = "Invalid promotion input index";
            return false;
        }

        // Validate the UTXO is still spendable
        if (m_wallet->IsSpent(outpoint)) {
            strErrorRet = "Promotion input has been spent";
            return false;
        }

        CTxDSIn txdsin(CTxIn(outpoint), wtx.tx->vout[outpoint.n].scriptPubKey,
                       m_wallet->GetRealOutpointCoinJoinRounds(outpoint));

        // Pair every input with an empty placeholder output - SendDenominate filters
        // empty outputs, so only the real larger-denom output set below is submitted
        vecPSInOutPairsRet.emplace_back(txdsin, CTxOut());
    }

    // Set the single real output (larger denomination) on the last pair
    CScript scriptDenom = keyHolderStorage.AddKey(m_wallet.get());
    if (!vecPSInOutPairsRet.empty()) {
        vecPSInOutPairsRet.back().second = CTxOut(nLargerAmount, scriptDenom);
    }

    // NOTE: all inputs were locked and recorded in vecOutPointLocked by SelectRebalanceInputs

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::PreparePromotionEntry -- Prepared %d inputs for promotion to %s\n",
                     vecPSInOutPairsRet.size(), CoinJoin::DenominationToString(nLargerDenom));

    return true;
}

bool CCoinJoinClientSession::PrepareDemotionEntry(std::string& strErrorRet, std::vector<std::pair<CTxDSIn, CTxOut>>& vecPSInOutPairsRet)
{
    AssertLockHeld(m_wallet->cs_wallet);

    vecPSInOutPairsRet.clear();

    if (m_vecRebalanceInputs.size() != 1) {
        strErrorRet = strprintf("Invalid demotion input count: %d (expected 1)", m_vecRebalanceInputs.size());
        return false;
    }

    // Session denom is the smaller denom (outputs)
    const CAmount nSmallerAmount = CoinJoin::DenominationToAmount(nSessionDenom);
    const int nLargerDenom = CoinJoin::GetLargerAdjacentDenom(nSessionDenom);
    if (nLargerDenom == 0) {
        strErrorRet = "No larger adjacent denomination for demotion";
        return false;
    }

    // Get the single input (larger denom)
    const COutPoint& outpoint = m_vecRebalanceInputs[0];
    const auto it = m_wallet->mapWallet.find(outpoint.hash);
    if (it == m_wallet->mapWallet.end()) {
        strErrorRet = "Demotion input not found in wallet";
        return false;
    }
    const wallet::CWalletTx& wtx = it->second;
    if (outpoint.n >= wtx.tx->vout.size()) {
        strErrorRet = "Invalid demotion input index";
        return false;
    }

    // Validate the UTXO is still spendable
    if (m_wallet->IsSpent(outpoint)) {
        strErrorRet = "Demotion input has been spent";
        return false;
    }

    CTxDSIn txdsin(CTxIn(outpoint), wtx.tx->vout[outpoint.n].scriptPubKey,
                   m_wallet->GetRealOutpointCoinJoinRounds(outpoint));

    // Create 10 outputs of smaller denomination
    // For demotion: 1 input, 10 outputs
    // The first pair has the real input, subsequent pairs have empty inputs
    for (int i = 0; i < CoinJoin::PROMOTION_RATIO; ++i) {
        CScript scriptDenom = keyHolderStorage.AddKey(m_wallet.get());
        CTxOut txout(nSmallerAmount, scriptDenom);

        if (i == 0) {
            // First entry has the real input
            vecPSInOutPairsRet.emplace_back(txdsin, txout);
        } else {
            // Subsequent entries have empty inputs (will be filtered out when building entry)
            vecPSInOutPairsRet.emplace_back(CTxDSIn(), txout);
        }
    }

    // NOTE: the input was locked and recorded in vecOutPointLocked by SelectRebalanceInputs

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::PrepareDemotionEntry -- Prepared 1 input for demotion to %d x %s\n",
                     CoinJoin::PROMOTION_RATIO, CoinJoin::DenominationToString(nSessionDenom));

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
    if (std::ranges::any_of(vecTally, [&](const auto& item) EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet) {
            return MakeCollateralAmounts(item, false);
        })) {
        return true;
    }

    // There should be at least some denominated funds we should be able to break in pieces to continue mixing
    if (std::ranges::any_of(vecTally, [&](const auto& item) EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet) {
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
    std::vector<COutput> vCoins{AvailableCoinsListUnspent(*m_wallet, &coin_control).All()};
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
        ReserveDestination reserveDest(m_wallet.get());
        auto dest_opt = reserveDest.GetReservedDestination(true);
        assert(dest_opt); // should never fail, as we just unlocked
        scriptChange = GetScriptForDestination(*dest_opt);
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
                                                     strFunc, static_cast<float>(nDenomValue) / COIN, static_cast<float>(nBalanceToDenominate) / COIN, nOutputs, txBuilder.ToString());
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
                                                 __func__, static_cast<float>(nDenomValue) / COIN, static_cast<float>(nBalanceToDenominate) / COIN, nOutputs, txBuilder.ToString());
                } else {
                    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - Error: AddOutput failed for nDenomValue: %f, nBalanceToDenominate: %f, nOutputs: %d, %s\n",
                                                 __func__, static_cast<float>(nDenomValue) / COIN, static_cast<float>(nBalanceToDenominate) / COIN, nOutputs, txBuilder.ToString());
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
                                             __func__, static_cast<float>(denom) / COIN, count, static_cast<float>(nBalanceToDenominate) / COIN, txBuilder.ToString());
                break;
            }
            WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 1 - FINISHED - nDenomValue: %f, count: %d, nBalanceToDenominate: %f, %s\n",
                                         __func__, static_cast<float>(denom) / COIN, count, static_cast<float>(nBalanceToDenominate) / COIN, txBuilder.ToString());
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
                                         __func__, static_cast<float>(nBalanceToDenominate) / COIN, static_cast<float>(nDenomValue) / COIN, denomsToCreateValue, denomsToCreateBal);
            auto it = mapDenomCount.find(nDenomValue);
            for (const auto i : util::irange(denomsToCreate)) {
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
                                             __func__, static_cast<float>(nDenomValue) / COIN, static_cast<float>(nBalanceToDenominate) / COIN, nOutputs, txBuilder.ToString());
                if (txBuilder.CountOutputs() >= COINJOIN_DENOM_OUTPUTS_THRESHOLD) break;
            }
            if (txBuilder.CountOutputs() >= COINJOIN_DENOM_OUTPUTS_THRESHOLD) break;
        }
    }

    WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 3 - nBalanceToDenominate: %f, %s\n", __func__, static_cast<float>(nBalanceToDenominate) / COIN, txBuilder.ToString());

    for (const auto& [denom, count] : mapDenomCount) {
        WalletCJLogPrint(m_wallet, "CCoinJoinClientSession::%s -- 3 - DONE - nDenomValue: %f, count: %d\n", __func__, static_cast<float>(denom) / COIN, count);
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

void CCoinJoinClientManager::DoMaintenance(ChainstateManager& chainman, CConnman& connman, const CTxMemPool& mempool)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return;

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
        obj.pushKV("addrs_core_p2p", mixingMasternode->pdmnState->netInfo->ToJson(NetInfoPurpose::CORE_P2P));
    }
    obj.pushKV("denomination", ValueFromAmount(CoinJoin::DenominationToAmount(nSessionDenom)));
    obj.pushKV("state", GetStateString());
    obj.pushKV("entries_count", GetEntriesCount());
}

UniValue CCoinJoinClientManager::getJsonInfo() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("running", isMixing());
    obj.pushKV("pending_inputs", static_cast<int64_t>(GetPendingObservationCount()));

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
    return obj;
}

bool CCoinJoinClientManager::ShouldPromote(int nSmallerDenom, int nLargerDenom, const wallet::CoinJoinDenomCounts& counts)
{
    // Validate denominations are adjacent
    if (!CoinJoin::AreAdjacentDenominations(nSmallerDenom, nLargerDenom)) {
        return false;
    }

    const int idxSmaller = CoinJoin::GetDenominationIndex(nSmallerDenom);
    const int idxLarger = CoinJoin::GetDenominationIndex(nLargerDenom);

    return CoinJoin::ShouldPromoteDenoms(counts.total[idxSmaller], counts.total[idxLarger],
                                         counts.fully_mixed[idxSmaller], CCoinJoinClientOptions::GetDenomsGoal());
}

bool CCoinJoinClientManager::ShouldDemote(int nLargerDenom, int nSmallerDenom, const wallet::CoinJoinDenomCounts& counts)
{
    // Validate denominations are adjacent
    if (!CoinJoin::AreAdjacentDenominations(nLargerDenom, nSmallerDenom)) {
        return false;
    }

    const int idxLarger = CoinJoin::GetDenominationIndex(nLargerDenom);
    const int idxSmaller = CoinJoin::GetDenominationIndex(nSmallerDenom);

    return CoinJoin::ShouldDemoteDenoms(counts.total[idxLarger], counts.total[idxSmaller],
                                        counts.fully_mixed[idxLarger], CCoinJoinClientOptions::GetDenomsGoal());
}

