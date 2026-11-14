// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/coinjoin.h>

#include <bls/bls.h>
#include <chainlock/chainlock.h>
#include <evo/deterministicmns.h>
#include <instantsend/instantsend.h>
#include <util/helpers.h>

#include <chain.h>
#include <chainparams.h>
#include <deploymentstatus.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <tinyformat.h>

#include <ranges>
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
uint256 CCoinJoinQueue::GetHash() const { return SerializeHash(*this, SER_NETWORK, PROTOCOL_VERSION); }

bool CCoinJoinQueue::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    if (!CBLSSignature(Span{vchSig}, false).VerifyInsecure(blsPubKey, GetSignatureHash(), false)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinQueue::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CCoinJoinQueue::IsTimeOutOfBounds(NodeSeconds current_time) const
{
    const auto queue_time{Time()};
    if (current_time < NodeSeconds{} || queue_time < NodeSeconds{}) return true;
    return std::chrono::abs(current_time - queue_time) > std::chrono::seconds{COINJOIN_QUEUE_TIMEOUT};
}

bool CCoinJoinQueue::IsTimeOutOfBounds() const
{
    return IsTimeOutOfBounds(std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime()));
}

[[nodiscard]] std::string CCoinJoinQueue::ToString() const
{
    return strprintf("nDenom=%d, nTime=%lld, fReady=%s, fTried=%s, masternode=%s",
        nDenom, nTime, util::to_string(fReady), util::to_string(fTried), masternodeOutpoint.ToStringShort());
}

uint256 CCoinJoinBroadcastTx::GetSignatureHash() const
{
    return SerializeHash(*this, SER_GETHASH, PROTOCOL_VERSION);
}

bool CCoinJoinBroadcastTx::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    if (!CBLSSignature(Span{vchSig}, false).VerifyInsecure(blsPubKey, GetSignatureHash(), false)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBroadcastTx::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CCoinJoinBroadcastTx::IsExpired(const CBlockIndex* pindex, const chainlock::Chainlocks& clhandler) const
{
    // expire confirmed DSTXes after ~1h since confirmation or chainlocked confirmation
    if (!nConfirmedHeight.has_value() || pindex->nHeight < *nConfirmedHeight) return false; // not mined yet
    if (pindex->nHeight - *nConfirmedHeight > 24) return true; // mined more than an hour ago
    return clhandler.HasChainLock(pindex->nHeight, *pindex->phashBlock);
}

bool CCoinJoinBroadcastTx::IsValidStructure(const CBlockIndex* pindex, const ChainstateManager& chainman,
                                            bool* fPossiblyValidPostV24Ret) const
{
    if (fPossiblyValidPostV24Ret) *fPossiblyValidPostV24Ret = false;

    // some trivial checks only, activation-independent ones first
    if (masternodeOutpoint.IsNull() && m_protxHash.IsNull()) {
        return false;
    }

    if (tx->vin.size() < static_cast<size_t>(CoinJoin::GetMinPoolParticipants())) {
        return false;
    }

    if (!std::ranges::all_of(tx->vout, [](const auto& txOut) {
        return CoinJoin::IsDenominatedAmount(txOut.nValue) && txOut.scriptPubKey.IsPayToPublicKeyHash();
    })) {
        return false;
    }

    // Post-V24: allow unbalanced counts (promotion/demotion) and up to 200 inputs
    // (20 participants * 10 inputs for promotions)
    // Pre-V24: require balanced input/output counts (1:1 mixing only), max 180 inputs
    // (20 participants * 9 entries)
    // Note: For post-V24 unbalanced transactions (promotion/demotion), value sum validation
    // (inputs == outputs) requires UTXO access and is performed in IsValidInOuts() when the
    // transaction is processed.
    const size_t nMaxInputsPreV24 = CoinJoin::GetMaxPoolParticipants() * COINJOIN_ENTRY_MAX_SIZE;
    const size_t nMaxInOutsPostV24 = CoinJoin::GetMaxPoolParticipants() * CoinJoin::PROMOTION_RATIO;
    const bool fValidPreV24 = tx->vin.size() == tx->vout.size() && tx->vin.size() <= nMaxInputsPreV24;
    // Post-V24 the input/output counts no longer bound each other one-to-one, but they still
    // bound each other: an all-promotion transaction carries at most PROMOTION_RATIO inputs per
    // output and an all-demotion one the mirror image, so neither side may exceed PROMOTION_RATIO
    // times the other. Beyond that, each promotion contributes PROMOTION_RATIO - 1 more inputs
    // than outputs and each demotion the mirror, while standard entries contribute equally to
    // both sides, so the difference between the sides of a transaction composable from valid
    // entries is always a multiple of that step.
    const size_t nSideDiff = tx->vin.size() > tx->vout.size() ? tx->vin.size() - tx->vout.size()
                                                             : tx->vout.size() - tx->vin.size();
    const bool fValidPostV24 = tx->vin.size() <= nMaxInOutsPostV24 && tx->vout.size() <= nMaxInOutsPostV24 &&
                               tx->vin.size() <= tx->vout.size() * static_cast<size_t>(CoinJoin::PROMOTION_RATIO) &&
                               tx->vout.size() <= tx->vin.size() * static_cast<size_t>(CoinJoin::PROMOTION_RATIO) &&
                               nSideDiff % static_cast<size_t>(CoinJoin::PROMOTION_RATIO - 1) == 0;

    const bool fV24Active = pindex && DeploymentActiveAt(*pindex, chainman, Consensus::DEPLOYMENT_V24);
    if (fV24Active) {
        return fValidPostV24;
    }

    // Pre-V24: report whether the tx would be valid on a post-V24 tip so callers can avoid
    // fully punishing relayers whose tip is ahead of ours around the activation boundary
    if (!fValidPreV24 && fValidPostV24 && fPossiblyValidPostV24Ret) {
        *fPossiblyValidPostV24Ret = true;
    }
    return fValidPreV24;
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

void CoinJoinQueueManager::SetNull()
{
    LOCK(cs_vecqueue);
    vecCoinJoinQueue.clear();
}

void CoinJoinQueueManager::CheckQueue()
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return; // it's ok to fail here, we run this quite frequently

    // check mixing queue objects for timeouts
    auto it = vecCoinJoinQueue.begin();
    while (it != vecCoinJoinQueue.end()) {
        if (it->IsTimeOutOfBounds()) {
            LogPrint(BCLog::COINJOIN, "CoinJoinQueueManager::%s -- Removing a queue (%s)\n", __func__, it->ToString());
            it = vecCoinJoinQueue.erase(it);
        } else {
            ++it;
        }
    }
}

std::optional<bool> CoinJoinQueueManager::TryHasQueueFromMasternode(const COutPoint& outpoint) const
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return std::nullopt;
    return std::ranges::any_of(vecCoinJoinQueue, [&outpoint](const auto& q) { return q.masternodeOutpoint == outpoint; });
}

std::optional<bool> CoinJoinQueueManager::TryCheckDuplicate(const CCoinJoinQueue& dsq) const
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return std::nullopt;
    for (const auto& q : vecCoinJoinQueue) {
        if (q == dsq) return true;
        if (q.fReady == dsq.fReady && q.masternodeOutpoint == dsq.masternodeOutpoint) return true;
    }
    return false;
}

bool CoinJoinQueueManager::TryAddQueue(CCoinJoinQueue dsq)
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return false;
    vecCoinJoinQueue.push_back(std::move(dsq));
    return true;
}

bool CoinJoinQueueManager::GetQueueItemAndTry(CCoinJoinQueue& dsqRet, int nDenomFilter)
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return false; // it's ok to fail here, we run this quite frequently

    for (auto& dsq : vecCoinJoinQueue) {
        // only try each queue once
        if (dsq.fTried || dsq.IsTimeOutOfBounds()) continue;
        // skip before marking as tried: a queue we never looked at must stay available
        if (nDenomFilter != 0 && dsq.nDenom != nDenomFilter) continue;
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

bool CoinJoin::IsPromotionDemotionActive(const ChainstateManager& chainman, bool fNextBlock)
{
    LOCK(::cs_main);
    const CBlockIndex* pindex = chainman.ActiveChain().Tip();
    if (pindex == nullptr) return false;
    return fNextBlock ? DeploymentActiveAfter(pindex, chainman, Consensus::DEPLOYMENT_V24)
                      : DeploymentActiveAt(*pindex, chainman, Consensus::DEPLOYMENT_V24);
}

bool CCoinJoinBaseSession::IsValidInOuts(Chainstate& active_chainstate, const llmq::CInstantSendManager& isman,
                                         const CTxMemPool& mempool, const std::vector<CTxIn>& vin,
                                         const std::vector<CTxOut>& vout, int session_denom, bool fAllowRebalanceShapes,
                                         PoolMessage& nMessageIDRet, bool* fConsumeCollateralRet, bool fFinalTx,
                                         CoinJoin::SessionDenomCounts* pDenomCountsRet)
{
    std::set<CScript> setScripPubKeys;
    nMessageIDRet = MSG_NOERR;
    if (fConsumeCollateralRet) *fConsumeCollateralRet = false;

    // Determine entry type based on input/output counts
    // Standard: N inputs, N outputs (same denom)
    // Promotion: PROMOTION_RATIO inputs of session denom, 1 output of larger adjacent denom
    // Demotion: 1 input of larger adjacent denom, PROMOTION_RATIO outputs of session denom
    // FinalTx: aggregate of the above across all participants - per-entry shapes don't apply
    enum class EntryType { STANDARD, PROMOTION, DEMOTION, FINAL_TX, INVALID };
    EntryType entryType = EntryType::STANDARD;

    if (fFinalTx && fAllowRebalanceShapes) {
        entryType = EntryType::FINAL_TX;
    } else if (vin.size() == vout.size()) {
        entryType = EntryType::STANDARD;
    } else if (fAllowRebalanceShapes) {
        if (vin.size() == static_cast<size_t>(CoinJoin::PROMOTION_RATIO) && vout.size() == 1) {
            entryType = EntryType::PROMOTION;
        } else if (vin.size() == 1 && vout.size() == static_cast<size_t>(CoinJoin::PROMOTION_RATIO)) {
            entryType = EntryType::DEMOTION;
        } else {
            entryType = EntryType::INVALID;
        }
    } else {
        // Pre-V24: only standard entries allowed
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: inputs vs outputs size mismatch! %d vs %d\n", __func__, vin.size(), vout.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
        return false;
    }

    if (entryType == EntryType::INVALID) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: invalid entry structure! %d inputs, %d outputs\n", __func__, vin.size(), vout.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
        return false;
    }

    // Validate promotion/demotion entries using dedicated validators
    // and determine expected denominations for UTXO input validation
    int nExpectedInputDenom = session_denom;
    int nExpectedOutputDenom = session_denom;
    // Final tx only: promotion outputs and demotion inputs use the larger adjacent
    // denomination, so it is allowed in addition to the session denomination.
    // 0 (never matched) for per-entry validation or when session_denom is the largest denom.
    int nLargerDenom{0};

    if (entryType == EntryType::PROMOTION) {
        if (!CoinJoin::ValidatePromotionEntry(vin, vout, session_denom, nMessageIDRet)) {
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        nExpectedOutputDenom = CoinJoin::GetLargerAdjacentDenom(session_denom);
    } else if (entryType == EntryType::DEMOTION) {
        if (!CoinJoin::ValidateDemotionEntry(vin, vout, session_denom, nMessageIDRet)) {
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        nExpectedInputDenom = CoinJoin::GetLargerAdjacentDenom(session_denom);
    } else if (entryType == EntryType::FINAL_TX) {
        nLargerDenom = CoinJoin::GetLargerAdjacentDenom(session_denom);
    }

    auto checkTxOut = [&](const CTxOut& txout, int nExpectedDenom) {
        const int nDenom = CoinJoin::AmountToDenomination(txout.nValue);

        if (nDenom != nExpectedDenom && (nLargerDenom == 0 || nDenom != nLargerDenom)) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::IsValidInOuts -- ERROR: incompatible denom %d (%s) != expected %d (%s)\n",
                    nDenom, CoinJoin::DenominationToString(nDenom), nExpectedDenom, CoinJoin::DenominationToString(nExpectedDenom));
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
        // Check for duplicate scripts across all inputs and outputs (privacy requirement)
        if (!setScripPubKeys.insert(txout.scriptPubKey).second) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::IsValidInOuts -- ERROR: already have this script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
            nMessageIDRet = ERR_ALREADY_HAVE;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        return true;
    };

    CAmount nFees{0};
    size_t nLargerInputs{0};
    size_t nLargerOutputs{0};

    for (const auto& txout : vout) {
        if (!checkTxOut(txout, nExpectedOutputDenom)) {
            return false;
        }
        if (nLargerDenom != 0 && CoinJoin::AmountToDenomination(txout.nValue) == nLargerDenom) {
            ++nLargerOutputs;
        }
        nFees -= txout.nValue;
    }

    LOCK(::cs_main);
    CCoinsViewMemPool viewMemPool(&active_chainstate.CoinsTip(), mempool);

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
            (coin.nHeight == MEMPOOL_HEIGHT && !isman.IsLocked(txin.prevout.hash))) {
            LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: missing, spent or non-locked mempool input! txin=%s\n", __func__, txin.ToString());
            nMessageIDRet = ERR_MISSING_TX;
            return false;
        }

        if (!checkTxOut(coin.out, nExpectedInputDenom)) {
            return false;
        }

        if (nLargerDenom != 0 && CoinJoin::AmountToDenomination(coin.out.nValue) == nLargerDenom) {
            ++nLargerInputs;
        }

        nFees += coin.out.nValue;
    }

    // Value sum must match: inputs == outputs (no fees in CoinJoin)
    // This holds for standard mixing (same denom) and promotion/demotion (value preserved)
    if (nFees != 0) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: non-zero fees! fees: %lld\n", __func__, nFees);
        nMessageIDRet = ERR_FEES;
        return false;
    }

    if (entryType == EntryType::FINAL_TX &&
        !CoinJoin::ValidateFinalTxComposition(vin.size(), vout.size(), nLargerInputs, nLargerOutputs)) {
        LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- ERROR: inconsistent final tx composition! %d/%d total, %d/%d larger denom inputs/outputs\n",
                 __func__, vin.size(), vout.size(), nLargerInputs, nLargerOutputs);
        nMessageIDRet = ERR_SIZE_MISMATCH;
        return false;
    }

    if (pDenomCountsRet) {
        pDenomCountsRet->inputs = vin.size() - nLargerInputs;
        pDenomCountsRet->outputs = vout.size() - nLargerOutputs;
    }

    LogPrint(BCLog::COINJOIN, "CCoinJoinBaseSession::%s -- Valid %s entry: %d inputs, %d outputs\n",
             __func__,
             entryType == EntryType::PROMOTION   ? "PROMOTION" :
             entryType == EntryType::DEMOTION    ? "DEMOTION" :
             entryType == EntryType::FINAL_TX    ? "FINAL_TX" :
                                                   "STANDARD",
             vin.size(), vout.size());

    return true;
}

// Responsibility for checking fee sanity is moved from the mempool to the client (BroadcastTransaction)
// but CoinJoin still requires ATMP with fee sanity checks so we need to implement them separately
bool ATMPIfSaneFee(ChainstateManager& chainman, const CTransactionRef& tx, bool test_accept)
{
    AssertLockHeld(::cs_main);

    const MempoolAcceptResult result = chainman.ProcessTransaction(tx, /*test_accept=*/true);
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
    return chainman.ProcessTransaction(tx, test_accept).m_result_type == MempoolAcceptResult::ResultType::VALID;
}

// check to make sure the collateral provided by the client is valid
bool CoinJoin::IsCollateralValid(ChainstateManager& chainman, const llmq::CInstantSendManager& isman,
                                 const CTxMemPool& mempool, const CTransaction& txCollateral)
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

    LOCK(::cs_main);
    CCoinsViewMemPool viewMemPool(&chainman.ActiveChainstate().CoinsTip(), mempool);

    for (const auto& txin : txCollateral.vin) {
        Coin coin;
        if (!viewMemPool.GetCoin(txin.prevout, coin) || coin.IsSpent() ||
            (coin.nHeight == MEMPOOL_HEIGHT && !isman.IsLocked(txin.prevout.hash))) {
            LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- missing, spent or non-locked mempool input! txin=%s\n", txin.ToString());
            return false;
        }
        nValueIn += coin.out.nValue;
    }

    //collateral transactions are required to pay out a small fee to the miners
    if (nValueIn - nValueOut < GetCollateralAmount()) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- did not include enough fees in transaction: fees: %d, txCollateral=%s", nValueOut - nValueIn, txCollateral.ToString()); /* Continued */
        return false;
    }

    LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- %s", txCollateral.ToString()); /* Continued */

    if (!ATMPIfSaneFee(chainman, MakeTransactionRef(txCollateral), /*test_accept=*/true)) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::IsCollateralValid -- didn't pass ATMPIfSaneFee()\n");
        return false;
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

CDSTXManager::CDSTXManager(const chainlock::Chainlocks& chainlocks) :
    m_chainlocks{chainlocks}
{
}
CDSTXManager::~CDSTXManager() = default;

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

bool CDSTXManager::IsTxExpired(const CCoinJoinBroadcastTx& tx, const CBlockIndex* pindex) const
{
    return tx.IsExpired(pindex, m_chainlocks);
}

void CDSTXManager::CheckDSTXes(const CBlockIndex* pindex)
{
    AssertLockNotHeld(cs_mapdstx);
    LOCK(cs_mapdstx);
    auto it = mapDSTX.begin();
    while (it != mapDSTX.end()) {
        if (IsTxExpired(it->second, pindex)) {
            mapDSTX.erase(it++);
        } else {
            ++it;
        }
    }
    LogPrint(BCLog::COINJOIN, "CoinJoin::CheckDSTXes -- mapDSTX.size()=%llu\n", mapDSTX.size());
}

void CDSTXManager::UpdatedBlockTip(const CBlockIndex* pindex)
{
    if (pindex) {
        CheckDSTXes(pindex);
    }
}

void CDSTXManager::NotifyChainLock(const CBlockIndex* pindex)
{
    if (pindex) {
        CheckDSTXes(pindex);
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

bool CoinJoin::ValidatePromotionEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut,
                                       int nSessionDenom, PoolMessage& nMessageIDRet)
{
    // Promotion: 10 inputs of smaller denom → 1 output of larger denom
    // Session denom is the smaller denom (inputs)
    nMessageIDRet = MSG_NOERR;

    // Check input count
    if (vecTxIn.size() != static_cast<size_t>(PROMOTION_RATIO)) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidatePromotionEntry -- ERROR: wrong input count %zu, expected %d\n",
                vecTxIn.size(), PROMOTION_RATIO);
        nMessageIDRet = ERR_SIZE_MISMATCH;
        return false;
    }

    // Check output count
    if (vecTxOut.size() != 1) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidatePromotionEntry -- ERROR: wrong output count %zu, expected 1\n",
                vecTxOut.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        return false;
    }

    // Get the larger adjacent denomination
    const int nLargerDenom = GetLargerAdjacentDenom(nSessionDenom);
    if (nLargerDenom == 0) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidatePromotionEntry -- ERROR: no larger adjacent denom for %s\n",
                DenominationToString(nSessionDenom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // Validate output is at larger denomination
    const int nOutputDenom = AmountToDenomination(vecTxOut[0].nValue);
    if (nOutputDenom != nLargerDenom) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidatePromotionEntry -- ERROR: output denom %s != expected %s\n",
                DenominationToString(nOutputDenom), DenominationToString(nLargerDenom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // Validate output is P2PKH
    if (!vecTxOut[0].scriptPubKey.IsPayToPublicKeyHash()) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidatePromotionEntry -- ERROR: output is not P2PKH\n");
        nMessageIDRet = ERR_INVALID_SCRIPT;
        return false;
    }

    return true;
}

bool CoinJoin::ValidateDemotionEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut,
                                      int nSessionDenom, PoolMessage& nMessageIDRet)
{
    // Demotion: 1 input of larger denom → 10 outputs of smaller denom
    // Session denom is the smaller denom (outputs)
    nMessageIDRet = MSG_NOERR;

    // Check input count
    if (vecTxIn.size() != 1) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidateDemotionEntry -- ERROR: wrong input count %zu, expected 1\n",
                vecTxIn.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        return false;
    }

    // Check output count
    if (vecTxOut.size() != static_cast<size_t>(PROMOTION_RATIO)) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidateDemotionEntry -- ERROR: wrong output count %zu, expected %d\n",
                vecTxOut.size(), PROMOTION_RATIO);
        nMessageIDRet = ERR_SIZE_MISMATCH;
        return false;
    }

    // The input has to come from the larger adjacent denomination
    if (GetLargerAdjacentDenom(nSessionDenom) == 0) {
        LogPrint(BCLog::COINJOIN, "CoinJoin::ValidateDemotionEntry -- ERROR: no larger adjacent denom for %s\n",
                DenominationToString(nSessionDenom));
        nMessageIDRet = ERR_DENOM;
        return false;
    }

    // Validate all outputs are at session denomination and P2PKH
    for (const auto& txout : vecTxOut) {
        const int nDenom = AmountToDenomination(txout.nValue);
        if (nDenom != nSessionDenom) {
            LogPrint(BCLog::COINJOIN, "CoinJoin::ValidateDemotionEntry -- ERROR: output denom %s != session denom %s\n",
                    DenominationToString(nDenom), DenominationToString(nSessionDenom));
            nMessageIDRet = ERR_DENOM;
            return false;
        }
        if (!txout.scriptPubKey.IsPayToPublicKeyHash()) {
            LogPrint(BCLog::COINJOIN, "CoinJoin::ValidateDemotionEntry -- ERROR: output is not P2PKH\n");
            nMessageIDRet = ERR_INVALID_SCRIPT;
            return false;
        }
    }

    return true;
}

bool CoinJoin::ValidateFinalTxComposition(size_t nTotalInputs, size_t nTotalOutputs,
                                          size_t nLargerInputs, size_t nLargerOutputs)
{
    // Aggregate consistency: every larger-denom output consumes PROMOTION_RATIO session-denom
    // inputs (promotion) and every larger-denom input produces PROMOTION_RATIO session-denom
    // outputs (demotion); the remainder on both sides are standard 1:1 inputs/outputs and
    // must not be negative. Together with the zero-fee check in IsValidInOuts() this
    // guarantees the final tx is composable from valid standard/promotion/demotion entries.
    if (nLargerInputs > nTotalInputs || nLargerOutputs > nTotalOutputs) return false;
    const size_t nSessionInputs = nTotalInputs - nLargerInputs;
    const size_t nSessionOutputs = nTotalOutputs - nLargerOutputs;
    return nSessionInputs >= nLargerOutputs * size_t(PROMOTION_RATIO) &&
           nSessionOutputs >= nLargerInputs * size_t(PROMOTION_RATIO);
}

CoinJoin::DSQueueValidation CoinJoin::ValidateDSQueue(CCoinJoinQueue& dsq, const CDeterministicMNList& mn_list,
                                                      const std::function<bool(const CCoinJoinQueue&)>& is_duplicate)
{
    if (!IsValidDenomination(dsq.nDenom)) {
        LogPrint(BCLog::COINJOIN, "DSQUEUE -- invalid denomination %d\n", dsq.nDenom);
        return {.misbehavior = 10};
    }

    if (dsq.masternodeOutpoint.IsNull() && dsq.m_protxHash.IsNull()) {
        return {.misbehavior = 100};
    }

    if (dsq.masternodeOutpoint.IsNull()) {
        if (auto dmn = mn_list.GetValidMN(dsq.m_protxHash)) {
            dsq.masternodeOutpoint = dmn->collateralOutpoint;
        } else {
            return {.misbehavior = 10};
        }
    }

    if (is_duplicate(dsq)) return {};

    LogPrint(BCLog::COINJOIN, "DSQUEUE -- %s new\n", dsq.ToString());

    if (dsq.IsTimeOutOfBounds()) return {};

    auto dmn = mn_list.GetValidMNByCollateral(dsq.masternodeOutpoint);
    if (!dmn) return {};

    if (dsq.m_protxHash.IsNull()) {
        dsq.m_protxHash = dmn->proTxHash;
    }

    if (!dsq.CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
        return {.misbehavior = 10};
    }

    return {.dmn = std::move(dmn)};
}
