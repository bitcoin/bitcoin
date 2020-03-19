// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privatesend/privatesend.h>

#include <masternode/activemasternode.h>
#include <consensus/validation.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <messagesigner.h>
#include <netmessagemaker.h>
#include <script/sign.h>
#include <txmempool.h>
#include <util.h>
#include <utilmoneystr.h>
#include <validation.h>

#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_chainlocks.h>

#include <string>

bool CPrivateSendEntry::AddScriptSig(const CTxIn& txin)
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

uint256 CPrivateSendQueue::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CPrivateSendQueue::Sign()
{
    if (!fMasternodeMode) return false;


    uint256 hash = GetSignatureHash();
    CBLSSignature sig = activeMasternodeInfo.blsKeyOperator->Sign(hash);
    if (!sig.IsValid()) {
        return false;
    }
    sig.GetBuf(vchSig);

    return true;
}

bool CPrivateSendQueue::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    uint256 hash = GetSignatureHash();

    CBLSSignature sig;
    sig.SetBuf(vchSig);
    if (!sig.IsValid() || !sig.VerifyInsecure(blsPubKey, hash)) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSendQueue::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CPrivateSendQueue::Relay(CConnman& connman)
{
    connman.ForEachNode([&connman, this](CNode* pnode) {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());
        if (pnode->nVersion >= MIN_PRIVATESEND_PEER_PROTO_VERSION && pnode->fSendDSQueue) {
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSQUEUE, (*this)));
        }
    });
    return true;
}

bool CPrivateSendQueue::IsTimeOutOfBounds() const
{
    return GetAdjustedTime() - nTime > PRIVATESEND_QUEUE_TIMEOUT || nTime - GetAdjustedTime() > PRIVATESEND_QUEUE_TIMEOUT;
}

uint256 CPrivateSendBroadcastTx::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CPrivateSendBroadcastTx::Sign()
{
    if (!fMasternodeMode) return false;

    uint256 hash = GetSignatureHash();

    CBLSSignature sig = activeMasternodeInfo.blsKeyOperator->Sign(hash);
    if (!sig.IsValid()) {
        return false;
    }
    sig.GetBuf(vchSig);

    return true;
}

bool CPrivateSendBroadcastTx::CheckSignature(const CBLSPublicKey& blsPubKey) const
{
    uint256 hash = GetSignatureHash();

    CBLSSignature sig;
    sig.SetBuf(vchSig);
    if (!sig.IsValid() || !sig.VerifyInsecure(blsPubKey, hash)) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSendBroadcastTx::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }

    return true;
}

bool CPrivateSendBroadcastTx::IsExpired(const CBlockIndex* pindex)
{
    // expire confirmed DSTXes after ~1h since confirmation or chainlocked confirmation
    if (nConfirmedHeight == -1 || pindex->nHeight < nConfirmedHeight) return false; // not mined yet
    if (pindex->nHeight - nConfirmedHeight > 24) return true; // mined more then an hour ago
    return llmq::chainLocksHandler->HasChainLock(pindex->nHeight, *pindex->phashBlock);
}

bool CPrivateSendBroadcastTx::IsValidStructure()
{
    // some trivial checks only
    if (tx->vin.size() != tx->vout.size()) {
        return false;
    }
    if (tx->vin.size() < CPrivateSend::GetMinPoolParticipants()) {
        return false;
    }
    if (tx->vin.size() > CPrivateSend::GetMaxPoolParticipants() * PRIVATESEND_ENTRY_MAX_SIZE) {
        return false;
    }
    for (const auto& out : tx->vout) {
        if (!CPrivateSend::IsDenominatedAmount(out.nValue)) {
            return false;
        }
        if (!out.scriptPubKey.IsPayToPublicKeyHash()) {
            return false;
        }
    }
    return true;
}

void CPrivateSendBaseSession::SetNull()
{
    // Both sides
    LOCK(cs_privatesend);
    nState = POOL_STATE_IDLE;
    nSessionID = 0;
    nSessionDenom = 0;
    vecEntries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    nTimeLastSuccessfulStep = GetTime();
}

void CPrivateSendBaseManager::SetNull()
{
    LOCK(cs_vecqueue);
    vecPrivateSendQueue.clear();
}

void CPrivateSendBaseManager::CheckQueue()
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return; // it's ok to fail here, we run this quite frequently

    // check mixing queue objects for timeouts
    auto it = vecPrivateSendQueue.begin();
    while (it != vecPrivateSendQueue.end()) {
        if ((*it).IsTimeOutOfBounds()) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseManager::%s -- Removing a queue (%s)\n", __func__, (*it).ToString());
            it = vecPrivateSendQueue.erase(it);
        } else {
            ++it;
        }
    }
}

bool CPrivateSendBaseManager::GetQueueItemAndTry(CPrivateSendQueue& dsqRet)
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if (!lockDS) return false; // it's ok to fail here, we run this quite frequently

    for (auto& dsq : vecPrivateSendQueue) {
        // only try each queue once
        if (dsq.fTried || dsq.IsTimeOutOfBounds()) continue;
        dsq.fTried = true;
        dsqRet = dsq;
        return true;
    }

    return false;
}

std::string CPrivateSendBaseSession::GetStateString() const
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
    case POOL_STATE_SUCCESS:
        return "SUCCESS";
    default:
        return "UNKNOWN";
    }
}

bool CPrivateSendBaseSession::IsValidInOuts(const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout, PoolMessage& nMessageIDRet, bool* fConsumeCollateralRet) const
{
    std::set<CScript> setScripPubKeys;
    nMessageIDRet = MSG_NOERR;
    if (fConsumeCollateralRet) *fConsumeCollateralRet = false;

    if (vin.size() != vout.size()) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::%s -- ERROR: inputs vs outputs size mismatch! %d vs %d\n", __func__, vin.size(), vout.size());
        nMessageIDRet = ERR_SIZE_MISMATCH;
        if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
        return false;
    }

    auto checkTxOut = [&](const CTxOut& txout) {
        int nDenom = CPrivateSend::AmountToDenomination(txout.nValue);
        if (nDenom != nSessionDenom) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::IsValidInOuts -- ERROR: incompatible denom %d (%s) != nSessionDenom %d (%s)\n",
                    nDenom, CPrivateSend::DenominationToString(nDenom), nSessionDenom, CPrivateSend::DenominationToString(nSessionDenom));
            nMessageIDRet = ERR_DENOM;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        if (!txout.scriptPubKey.IsPayToPublicKeyHash()) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::IsValidInOuts -- ERROR: invalid script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
            nMessageIDRet = ERR_INVALID_SCRIPT;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        if (!setScripPubKeys.insert(txout.scriptPubKey).second) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::IsValidInOuts -- ERROR: already have this script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
            nMessageIDRet = ERR_ALREADY_HAVE;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }
        // IsPayToPublicKeyHash() above already checks for scriptPubKey size,
        // no need to double check, hence no usage of ERR_NON_STANDARD_PUBKEY
        return true;
    };

    CAmount nFees{0};

    for (const auto& txout : vout) {
        if (!checkTxOut(txout)) {
            return false;
        }
        nFees -= txout.nValue;
    }

    CCoinsViewMemPool viewMemPool(pcoinsTip.get(), mempool);

    for (const auto& txin : vin) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::%s -- txin=%s\n", __func__, txin.ToString());

        if (txin.prevout.IsNull()) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::%s -- ERROR: invalid input!\n", __func__);
            nMessageIDRet = ERR_INVALID_INPUT;
            if (fConsumeCollateralRet) *fConsumeCollateralRet = true;
            return false;
        }

        Coin coin;
        if (!viewMemPool.GetCoin(txin.prevout, coin) || coin.IsSpent() ||
            (coin.nHeight == MEMPOOL_HEIGHT && !llmq::quorumInstantSendManager->IsLocked(txin.prevout.hash))) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::%s -- ERROR: missing, spent or non-locked mempool input! txin=%s\n", __func__, txin.ToString());
            nMessageIDRet = ERR_MISSING_TX;
            return false;
        }

        if (!checkTxOut(coin.out)) {
            return false;
        }

        nFees += coin.out.nValue;
    }

    // The same size and denom for inputs and outputs ensures their total value is also the same,
    // no need to double check. If not, we are doing something wrong, bail out.
    if (nFees != 0) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSendBaseSession::%s -- ERROR: non-zero fees! fees: %lld\n", __func__, nFees);
        nMessageIDRet = ERR_FEES;
        return false;
    }

    return true;
}

// Definitions for static data members
std::vector<CAmount> CPrivateSend::vecStandardDenominations;
std::map<uint256, CPrivateSendBroadcastTx> CPrivateSend::mapDSTX;
CCriticalSection CPrivateSend::cs_mapdstx;

void CPrivateSend::InitStandardDenominations()
{
    vecStandardDenominations.clear();
    /* Denominations

        A note about convertibility. Within mixing pools, each denomination
        is convertible to another.

        For example:
        1DRK+1000 == (.1DRK+100)*10
        10DRK+10000 == (1DRK+1000)*10
    */
    /* Disabled
    vecStandardDenominations.push_back( (100      * COIN)+100000 );
    */
    vecStandardDenominations.push_back((10 * COIN) + 10000);
    vecStandardDenominations.push_back((1 * COIN) + 1000);
    vecStandardDenominations.push_back((.1 * COIN) + 100);
    vecStandardDenominations.push_back((.01 * COIN) + 10);
    vecStandardDenominations.push_back((.001 * COIN) + 1);
}

// check to make sure the collateral provided by the client is valid
bool CPrivateSend::IsCollateralValid(const CTransaction& txCollateral)
{
    if (txCollateral.vout.empty()) return false;
    if (txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;

    for (const auto& txout : txCollateral.vout) {
        nValueOut += txout.nValue;

        if (!txout.scriptPubKey.IsPayToPublicKeyHash() && !txout.scriptPubKey.IsUnspendable()) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- Invalid Script, txCollateral=%s", txCollateral.ToString());
            return false;
        }
    }

    for (const auto& txin : txCollateral.vin) {
        Coin coin;
        auto mempoolTx = mempool.get(txin.prevout.hash);
        if (mempoolTx != nullptr) {
            if (mempool.isSpent(txin.prevout) || !llmq::quorumInstantSendManager->IsLocked(txin.prevout.hash)) {
                LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- spent or non-locked mempool input! txin=%s\n", txin.ToString());
                return false;
            }
            nValueIn += mempoolTx->vout[txin.prevout.n].nValue;
        } else if (GetUTXOCoin(txin.prevout, coin)) {
            nValueIn += coin.out.nValue;
        } else {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- Unknown inputs in collateral transaction, txCollateral=%s", txCollateral.ToString());
            return false;
        }
    }

    //collateral transactions are required to pay out a small fee to the miners
    if (nValueIn - nValueOut < GetCollateralAmount()) {
        LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- did not include enough fees in transaction: fees: %d, txCollateral=%s", nValueOut - nValueIn, txCollateral.ToString());
        return false;
    }

    LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- %s", txCollateral.ToString());

    {
        LOCK(cs_main);
        CValidationState validationState;
        if (!AcceptToMemoryPool(mempool, validationState, MakeTransactionRef(txCollateral), nullptr /* pfMissingInputs */, false /* bypass_limits */, maxTxFee /* nAbsurdFee */, true /* fDryRun */)) {
            LogPrint(BCLog::PRIVATESEND, "CPrivateSend::IsCollateralValid -- didn't pass AcceptToMemoryPool()\n");
            return false;
        }
    }

    return true;
}

bool CPrivateSend::IsCollateralAmount(CAmount nInputAmount)
{
    // collateral input can be anything between 1x and "max" (including both)
    return (nInputAmount >= GetCollateralAmount() && nInputAmount <= GetMaxCollateralAmount());
}

/*
    Return a bitshifted integer representing a denomination in vecStandardDenominations
    or 0 if none was found
*/
int CPrivateSend::AmountToDenomination(CAmount nInputAmount)
{
    for (size_t i = 0; i < vecStandardDenominations.size(); ++i) {
        if (nInputAmount == vecStandardDenominations[i]) {
            return 1 << i;
        }
    }
    return 0;
}

/*
    Returns:
    - one of standard denominations from vecStandardDenominations based on the provided bitshifted integer
    - 0 for non-initialized sessions (nDenom = 0)
    - a value below 0 if an error occured while converting from one to another
*/
CAmount CPrivateSend::DenominationToAmount(int nDenom)
{
    if (nDenom == 0) {
        // not initialized
        return 0;
    }

    size_t nMaxDenoms = vecStandardDenominations.size();

    if (nDenom >= (1 << nMaxDenoms) || nDenom < 0) {
        // out of bounds
        return -1;
    }

    if ((nDenom & (nDenom - 1)) != 0) {
        // non-denom
        return -2;
    }

    CAmount nDenomAmount{-3};

    for (size_t i = 0; i < nMaxDenoms; ++i) {
        if (nDenom & (1 << i)) {
            nDenomAmount = vecStandardDenominations[i];
            break;
        }
    }

    return nDenomAmount;
}

/*
    Same as DenominationToAmount but returns a string representation
*/
std::string CPrivateSend::DenominationToString(int nDenom)
{
    CAmount nDenomAmount = DenominationToAmount(nDenom);

    switch (nDenomAmount) {
        case  0: return "N/A";
        case -1: return "out-of-bounds";
        case -2: return "non-denom";
        case -3: return "to-amount-error";
        default: return ValueFromAmount(nDenomAmount).getValStr();
    }

    // shouldn't happen
    return "to-string-error";
}

bool CPrivateSend::IsDenominatedAmount(CAmount nInputAmount)
{
    return AmountToDenomination(nInputAmount) > 0;
}

bool CPrivateSend::IsValidDenomination(int nDenom)
{
    return DenominationToAmount(nDenom) > 0;
}

std::string CPrivateSend::GetMessageByID(PoolMessage nMessageID)
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
        return _("Last PrivateSend was too recent.");
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

void CPrivateSend::AddDSTX(const CPrivateSendBroadcastTx& dstx)
{
    LOCK(cs_mapdstx);
    mapDSTX.insert(std::make_pair(dstx.tx->GetHash(), dstx));
}

CPrivateSendBroadcastTx CPrivateSend::GetDSTX(const uint256& hash)
{
    LOCK(cs_mapdstx);
    auto it = mapDSTX.find(hash);
    return (it == mapDSTX.end()) ? CPrivateSendBroadcastTx() : it->second;
}

void CPrivateSend::CheckDSTXes(const CBlockIndex* pindex)
{
    LOCK(cs_mapdstx);
    auto it = mapDSTX.begin();
    while (it != mapDSTX.end()) {
        if (it->second.IsExpired(pindex)) {
            mapDSTX.erase(it++);
        } else {
            ++it;
        }
    }
    LogPrint(BCLog::PRIVATESEND, "CPrivateSend::CheckDSTXes -- mapDSTX.size()=%llu\n", mapDSTX.size());
}

void CPrivateSend::UpdatedBlockTip(const CBlockIndex* pindex)
{
    if (pindex && masternodeSync.IsBlockchainSynced()) {
        CheckDSTXes(pindex);
    }
}

void CPrivateSend::NotifyChainLock(const CBlockIndex* pindex)
{
    if (pindex && masternodeSync.IsBlockchainSynced()) {
        CheckDSTXes(pindex);
    }
}

void CPrivateSend::UpdateDSTXConfirmedHeight(const CTransactionRef& tx, int nHeight)
{
    AssertLockHeld(cs_mapdstx);

    auto it = mapDSTX.find(tx->GetHash());
    if (it == mapDSTX.end()) {
        return;
    }

    it->second.SetConfirmedHeight(nHeight);
    LogPrint(BCLog::PRIVATESEND, "CPrivateSend::%s -- txid=%s, nHeight=%d\n", __func__, tx->GetHash().ToString(), nHeight);
}

void CPrivateSend::TransactionAddedToMempool(const CTransactionRef& tx)
{
    LOCK(cs_mapdstx);
    UpdateDSTXConfirmedHeight(tx, -1);
}

void CPrivateSend::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted)
{
    LOCK(cs_mapdstx);
    for (const auto& tx : vtxConflicted) {
        UpdateDSTXConfirmedHeight(tx, -1);
    }

    for (const auto& tx : pblock->vtx) {
        UpdateDSTXConfirmedHeight(tx, pindex->nHeight);
    }
}

void CPrivateSend::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    LOCK(cs_mapdstx);
    for (const auto& tx : pblock->vtx) {
        UpdateDSTXConfirmedHeight(tx, -1);
    }
}
