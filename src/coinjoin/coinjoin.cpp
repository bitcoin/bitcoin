// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/coinjoin.h>

#include <bls/bls.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <governance/common.h>
#include <llmq/chainlocks.h>
#include <llmq/instantsend.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <messagesigner.h>
#include <netmessagemaker.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

#include <tinyformat.h>
#include <string>

constexpr static CAmount DEFAULT_MAX_RAW_TX_FEE{COIN / 10};

bool CCoinJoinEntry::AddScriptSig(const CTxIn& txin)
{
    for (auto& txdsin : vecTxDSIn) {
        if (txdsin.prevout == txin.prevout && txdsin.nSequence == txin.nSequence) {
            if (txdsin.fHasSig) return false;

            txdsin.scriptSig = txin.scriptSig;
            txdsin.fHasSig = true;

            return true;
        }
    }

    return false;
}

uint256 CCoinJoinQueue::GetSignatureHash() const
{
    return SerializeHash(*this, SER_GETHASH, PROTOCOL_VERSION);
}

bool CCoinJoinQueue::Sign()
{
    if (!fMasternodeMode) return false;

    uint256 hash = GetSignatureHash();
    CBLSSignature sig = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.blsKeyOperator->Sign(hash, false));
    if (!sig.IsValid()) {
        return false;
    }
    vchSig = sig.ToByteVector(false);

    return true;
}

bool CCoinJoinQueue::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    if (!CBLSSignature(Span{vchSig}).VerifyInsecure(blsPubKey, GetSignatureHash(), false)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinQueue::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CCoinJoinQueue::Relay(CConnman& connman)
{
    connman.ForEachNode([&connman, this](CNode* pnode) {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());
        if (pnode->fSendDSQueue) {
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSQUEUE, (*this)));
        }
    });
    return true;
}

bool CCoinJoinQueue::IsTimeOutOfBounds(int64_t current_time) const
{
    return current_time - nTime > COINJOIN_QUEUE_TIMEOUT ||
           nTime - current_time > COINJOIN_QUEUE_TIMEOUT;
}

[[nodiscard]] std::string CCoinJoinQueue::ToString() const
{
    return strprintf("nDenom=%d, nTime=%lld, fReady=%s, fTried=%s, masternode=%s",
        nDenom, nTime, fReady ? "true" : "false", fTried ? "true" : "false", masternodeOutpoint.ToStringShort());
}

uint256 CCoinJoinBroadcastTx::GetSignatureHash() const
{
    return SerializeHash(*this, SER_GETHASH, PROTOCOL_VERSION);
}

bool CCoinJoinBroadcastTx::Sign()
{
    if (!fMasternodeMode) return false;

    uint256 hash = GetSignatureHash();
    CBLSSignature sig = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.blsKeyOperator->Sign(hash, false));
    if (!sig.IsValid()) {
        return false;
    }
    vchSig = sig.ToByteVector(false);

    return true;
}

bool CCoinJoinBroadcastTx::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    if (!CBLSSignature(Span{vchSig}).VerifyInsecure(blsPubKey, GetSignatureHash(), false)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBroadcastTx::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CCoinJoinBroadcastTx::IsExpired(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler) const
{
    // expire confirmed DSTXes after ~1h since confirmation or chainlocked confirmation
    if (!nConfirmedHeight.has_value() || pindex->nHeight < *nConfirmedHeight) return false; // not mined yet
    if (pindex->nHeight - *nConfirmedHeight > 24) return true; // mined more than an hour ago
    return clhandler.HasChainLock(pindex->nHeight, *pindex->phashBlock);
}

bool CCoinJoinBroadcastTx::IsValidStructure() const
{
    // some trivial checks only
    if (masternodeOutpoint.IsNull() && m_protxHash.IsNull()) {
        return false;
    }
    if (tx->vin.size() != tx->vout.size()) {
        return false;
    }
    if (tx->vin.size() < size_t(CoinJoin::GetMinPoolParticipants())) {
        return false;
    }
    if (tx->vin.size() > CoinJoin::GetMaxPoolParticipants() * COINJOIN_ENTRY_MAX_SIZE) {
        return false;
    }
    return ranges::all_of(tx->vout, [] (const auto& txOut){
        return CoinJoin::IsDenominatedAmount(txOut.nValue) && txOut.scriptPubKey.IsPayToPublicKeyHash();
    });
}

void CCoinJoinBaseSession::SetNull()
{
    // Both sides
    AssertLockHeld(cs_coinjoin);
    nState = POOL_STATE_IDLE;
    nSessionID = 0;
    nSessionDenom = 0;
    vecEntries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    nTimeLastSuccessfulStep = GetTime();
}

void CCoinJoinBaseManager::SetNull()
{
    LOCK(cs_vecqueue);
    vecCoinJoinQueue.clear();
}

void CCoinJoinBaseManager::CheckQueue()
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return; // it's ok to fail here, we run this quite frequently

    // check mixing queue objects for timeouts
    auto it = vecCoinJoinQueue.begin();
    while (it != vecCoinJoinQueue.end()) {
        if (it->IsTimeOutOfBounds()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseManager::%s -- Removing a queue (%s)\n", __func__, it->ToString());
            it = vecCoinJoinQueue.erase(it);
        } else {
            ++it;
        }
    }
}

bool CCoinJoinBaseManager::GetQueueItemAndTry(CCoinJoinQueue& dsqRet)
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return false; // it's ok to fail here, we run this quite frequently

    for (auto& dsq : vecCoinJoinQueue) {
        // only try each queue once
        if (dsq.fTried || dsq.IsTimeOutOfBounds()) continue;
        dsq.fTried = true;
        dsqRet = dsq;
        return true;
    }

    return false;
}

std::string CCoinJoinBaseSession::GetStateString() const
{
    switch (nState) {
    case POOL_STATE_IDLE:
        return "IDLE";
    case POOL_STATE_QUEUE:
        return "QUEUE";
    case POOL_STATE_ACCEPTING_ENTRIES:
        return "ACCEPTING_ENTRIES";
    case POOL_STATE_SIGNING:
        return "SIGNING";
    case POOL_STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

bool CCoinJoinBaseSession::IsValidInOuts(const CTxMemPool& mempool, const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout, PoolMessage& nMessageIDRet, bool* fConsumeCollateralRet) const
{
    std::set<CScript> setScripPubKeys;
    nMessageIDRet = MSG_NOERR;
    if (fConsumeCollateralRet) *fConsumeCollateralRet = false;

    if (vin.size() != vout.size()) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: inputs vs outputs size mismatch! %d vs %d\n", __func__, vin.size(), vout.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
        return false;
    }

    auto checkTxOut = [&](const CTxOut& txout) {
        if (int nDenom = CoinJoin::AmountToDenomination(txout.nValue); nDenom != nSessionDenom) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::IsValidInOuts -- ERROR: incompatible denom %d (%s) != nSessionDenom %d (%s)\n",
                    nDenom, CoinJoin::DenominationToString(nDenom), nSessionDenom, CoinJoin::DenominationToString(nSessionDenom));
            nMessageIDRet = ERR_DENOM;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        if (!txout.scriptPubKey.IsPayToPublicKeyHash()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::IsValidInOuts -- ERROR: invalid script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
            nMessageIDRet = ERR_INVALID_SCRIPT;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        if (!setScripPubKeys.insert(txout.scriptPubKey).second) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::IsValidInOuts -- ERROR: already have this script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
            nMessageIDRet = ERR_ALREADY_HAVE;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        // IsPayToPublicKeyHash() above already checks for scriptPubKey size,
        // no need to double-check, hence no usage of ERR_NON_STANDARD_PUBKEY
        return true;
    };

    CAmount nFees{0};

    for (const auto& txout : vout) {
        if (!checkTxOut(txout)) {
            return false;
        }
        nFees -= txout.nValue;
    }

    CCoinsViewMemPool viewMemPool(WITH_LOCK(cs_main, return &::ChainstateActive().CoinsTip()), mempool);

    for (const auto& txin : vin) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- txin=%s\n", __func__, txin.ToString());

        if (txin.prevout.IsNull()) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: invalid input!\n", __func__);
            nMessageIDRet = ERR_INVALID_INPUT;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }

        Coin coin;
        if (!viewMemPool.GetCoin(txin.prevout, coin) || coin.IsSpent() ||
            (coin.nHeight == MEMPOOL_HEIGHT && !llmq::quorumInstantSendManager->IsLocked(txin.prevout.hash))) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: missing, spent or non-locked mempool input! txin=%s\n", __func__, txin.ToString());
            nMessageIDRet = ERR_MISSING_TX;
            return false;
        }

        if (!checkTxOut(coin.out)) {
            return false;
        }

        nFees += coin.out.nValue;
    }

    // The same size and denom for inputs and outputs ensures their total value is also the same,
    // no need to double-check. If not, we are doing something wrong, bail out.
    if (nFees != 0) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: non-zero fees! fees: %lld\n", __func__, nFees);
        nMessageIDRet = ERR_FEES;
        return false;
    }

    return true;
}

// Responsibility for checking fee sanity is moved from the mempool to the client (BroadcastTransaction)
// but CoinJoin still requires ATMP with fee sanity checks so we need to implement them separately
bool ATMPIfSaneFee(CChainState& active_chainstate, CTxMemPool& pool, const CTransactionRef &tx, bool test_accept) {
    AssertLockHeld(cs_main);

    const MempoolAcceptResult result = AcceptToMemoryPool(active_chainstate, pool, tx, /* bypass_limits */ false, /* test_accept */ true);
    if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
        /* Fetch fee and fast-fail if ATMP fails regardless */
        return false;
    } else if (result.m_base_fees.value() > DEFAULT_MAX_RAW_TX_FEE) {
        /* Check fee against fixed upper limit */
        return false;
    } else if (test_accept) {
        /* Don't re-run ATMP if only doing test run */
        return true;
    }
    return AcceptToMemoryPool(active_chainstate, pool, tx, /* bypass_limits */ false, test_accept).m_result_type == MempoolAcceptResult::ResultType::VALID;
}

// check to make sure the collateral provided by the client is valid
bool CoinJoin::IsCollateralValid(CTxMemPool& mempool, const CTransaction& txCollateral)
{
    if (txCollateral.vout.empty()) return false;
    if (txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;

    for (const auto& txout : txCollateral.vout) {
        nValueOut += txout.nValue;

        if (!txout.scriptPubKey.IsPayToPublicKeyHash() && !txout.scriptPubKey.IsUnspendable()) {
            LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- Invalid Script, txCollateral=%s", txCollateral.ToString()); /* Continued */
            return false;
        }
    }

    for (const auto& txin : txCollateral.vin) {
        Coin coin;
        auto mempoolTx = mempool.get(txin.prevout.hash);
        if (mempoolTx != nullptr) {
            if (mempool.isSpent(txin.prevout) || !llmq::quorumInstantSendManager->IsLocked(txin.prevout.hash)) {
                LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- spent or non-locked mempool input! txin=%s\n", txin.ToString());
                return false;
            }
            nValueIn += mempoolTx->vout[txin.prevout.n].nValue;
        } else if (GetUTXOCoin(txin.prevout, coin)) {
            nValueIn += coin.out.nValue;
        } else {
            LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- Unknown inputs in collateral transaction, txCollateral=%s", txCollateral.ToString()); /* Continued */
            return false;
        }
    }

    //collateral transactions are required to pay out a small fee to the miners
    if (nValueIn - nValueOut < GetCollateralAmount()) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- did not include enough fees in transaction: fees: %d, txCollateral=%s", nValueOut - nValueIn, txCollateral.ToString()); /* Continued */
        return false;
    }

    LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- %s", txCollateral.ToString()); /* Continued */

    {
        LOCK(cs_main);
        if (!ATMPIfSaneFee(::ChainstateActive(), mempool, MakeTransactionRef(txCollateral), /*test_accept=*/true)) {
            LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- didn't pass AcceptToMemoryPool()\n");
            return false;
        }
    }

    return true;
}

bilingual_str CoinJoin::GetMessageByID(PoolMessage nMessageID)
{
    switch (nMessageID) {
    case ERR_ALREADY_HAVE:
        return _("Already have that input.");
    case ERR_DENOM:
        return _("No matching denominations found for mixing.");
    case ERR_ENTRIES_FULL:
        return _("Entries are full.");
    case ERR_EXISTING_TX:
        return _("Not compatible with existing transactions.");
    case ERR_FEES:
        return _("Transaction fees are too high.");
    case ERR_INVALID_COLLATERAL:
        return _("Collateral not valid.");
    case ERR_INVALID_INPUT:
        return _("Input is not valid.");
    case ERR_INVALID_SCRIPT:
        return _("Invalid script detected.");
    case ERR_INVALID_TX:
        return _("Transaction not valid.");
    case ERR_MAXIMUM:
        return _("Entry exceeds maximum size.");
    case ERR_MN_LIST:
        return _("Not in the Masternode list.");
    case ERR_MODE:
        return _("Incompatible mode.");
    case ERR_QUEUE_FULL:
        return _("Masternode queue is full.");
    case ERR_RECENT:
        return _("Last queue was created too recently.");
    case ERR_SESSION:
        return _("Session not complete!");
    case ERR_MISSING_TX:
        return _("Missing input transaction information.");
    case ERR_VERSION:
        return _("Incompatible version.");
    case MSG_NOERR:
        return _("No errors detected.");
    case MSG_SUCCESS:
        return _("Transaction created successfully.");
    case MSG_ENTRIES_ADDED:
        return _("Your entries added successfully.");
    case ERR_SIZE_MISMATCH:
        return _("Inputs vs outputs size mismatch.");
    case ERR_NON_STANDARD_PUBKEY:
    case ERR_NOT_A_MN:
    default:
        return _("Unknown response.");
    }
}

// Definitions for static data members
std::unique_ptr<CDSTXManager> dstxManager;

void CDSTXManager::AddDSTX(const CCoinJoinBroadcastTx& dstx)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    mapDSTX.insert(std::make_pair(dstx.tx->GetHash(), dstx));
}

CCoinJoinBroadcastTx CDSTXManager::GetDSTX(const uint256& hash)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    auto it = mapDSTX.find(hash);
    return (it == mapDSTX.end()) ? CCoinJoinBroadcastTx() : it->second;
}

void CDSTXManager::CheckDSTXes(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    auto it = mapDSTX.begin();
    while (it != mapDSTX.end()) {
        if (it->second.IsExpired(pindex, clhandler)) {
            mapDSTX.erase(it++);
        } else {
            ++it;
        }
    }
    LogPrint(BCLog::COINJOIN, "CoinJoin::CheckDSTXes -- mapDSTX.size()=%llu\n", mapDSTX.size());
}

void CDSTXManager::UpdatedBlockTip(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler, const CMasternodeSync& mn_sync)
{
    if (pindex && mn_sync.IsBlockchainSynced()) {
        CheckDSTXes(pindex, clhandler);
    }
}

void CDSTXManager::NotifyChainLock(const CBlockIndex* pindex, const llmq::CChainLocksHandler& clhandler, const CMasternodeSync& mn_sync)
{
    if (pindex && mn_sync.IsBlockchainSynced()) {
        CheckDSTXes(pindex, clhandler);
    }
}

void CDSTXManager::UpdateDSTXConfirmedHeight(const CTransactionRef& tx, std::optional<int> nHeight)
{
    AssertLockHeld(cs_mapdstx);

    auto it = mapDSTX.find(tx->GetHash());
    if (it == mapDSTX.end()) {
        return;
    }

    it->second.SetConfirmedHeight(nHeight);
    LogPrint(BCLog::COINJOIN, "CDSTXManager::%s -- txid=%s, nHeight=%d\n", __func__, tx->GetHash().ToString(), nHeight.value_or(-1));
}

void CDSTXManager::TransactionAddedToMempool(const CTransactionRef& tx)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    UpdateDSTXConfirmedHeight(tx, std::nullopt);
}

void CDSTXManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);

    for (const auto& tx : pblock->vtx) {
        UpdateDSTXConfirmedHeight(tx, pindex->nHeight);
    }
}

void CDSTXManager::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex*)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    for (const auto& tx : pblock->vtx) {
        UpdateDSTXConfirmedHeight(tx, std::nullopt);
    }
}

int CoinJoin::GetMinPoolParticipants() { return Params().PoolMinParticipants(); }
int CoinJoin::GetMaxPoolParticipants() { return Params().PoolMaxParticipants(); }
