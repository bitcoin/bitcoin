// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "privatesend.h"

#include "activemasternode.h"
#include "consensus/validation.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netmessagemaker.h"
#include "script/sign.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"

#include <string>

bool CDarkSendEntry::AddScriptSig(const CTxIn& txin)
{
    for (auto& txdsin : vecTxDSIn) {
        if(txdsin.prevout == txin.prevout && txdsin.nSequence == txin.nSequence) {
            if(txdsin.fHasSig) return false;

            txdsin.scriptSig = txin.scriptSig;
            txdsin.fHasSig = true;

            return true;
        }
    }

    return false;
}

uint256 CDarksendQueue::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CDarksendQueue::Sign()
{
    if(!fMasternodeMode) return false;

    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, activeMasternodeInfo.keyOperator, vchSig)) {
            LogPrintf("CDarksendQueue::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, activeMasternodeInfo.keyIDOperator, vchSig, strError)) {
            LogPrintf("CDarksendQueue::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = CTxIn(masternodeOutpoint).ToString() +
                        std::to_string(nDenom) +
                        std::to_string(nTime) +
                        std::to_string(fReady);

        if(!CMessageSigner::SignMessage(strMessage, vchSig, activeMasternodeInfo.keyOperator)) {
            LogPrintf("CDarksendQueue::Sign -- SignMessage() failed, %s\n", ToString());
            return false;
        }

        if(!CMessageSigner::VerifyMessage(activeMasternodeInfo.keyIDOperator, vchSig, strMessage, strError)) {
            LogPrintf("CDarksendQueue::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CDarksendQueue::CheckSignature(const CKeyID& keyIDOperator) const
{
    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, keyIDOperator, vchSig, strError)) {
            // we don't care about queues with old signature format
            LogPrintf("CDarksendQueue::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = CTxIn(masternodeOutpoint).ToString() +
                        std::to_string(nDenom) +
                        std::to_string(nTime) +
                        std::to_string(fReady);

        if(!CMessageSigner::VerifyMessage(keyIDOperator, vchSig, strMessage, strError)) {
            LogPrintf("CDarksendQueue::CheckSignature -- Got bad Masternode queue signature: %s; error: %s\n", ToString(), strError);
            return false;
        }
    }

    return true;
}

bool CDarksendQueue::Relay(CConnman& connman)
{
    connman.ForEachNode([&connman, this](CNode* pnode) {
        CNetMsgMaker msgMaker(pnode->GetSendVersion());
        if (pnode->nVersion >= MIN_PRIVATESEND_PEER_PROTO_VERSION)
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::DSQUEUE, (*this)));
    });
    return true;
}

uint256 CDarksendBroadcastTx::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CDarksendBroadcastTx::Sign()
{
    if(!fMasternodeMode) return false;

    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, activeMasternodeInfo.keyOperator, vchSig)) {
            LogPrintf("CDarksendBroadcastTx::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, activeMasternodeInfo.keyIDOperator, vchSig, strError)) {
            LogPrintf("CDarksendBroadcastTx::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = tx->GetHash().ToString() + std::to_string(sigTime);

        if(!CMessageSigner::SignMessage(strMessage, vchSig, activeMasternodeInfo.keyOperator)) {
            LogPrintf("CDarksendBroadcastTx::Sign -- SignMessage() failed\n");
            return false;
        }

        if(!CMessageSigner::VerifyMessage(activeMasternodeInfo.keyIDOperator, vchSig, strMessage, strError)) {
            LogPrintf("CDarksendBroadcastTx::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CDarksendBroadcastTx::CheckSignature(const CKeyID& keyIDOperator) const
{
    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, keyIDOperator, vchSig, strError)) {
            // we don't care about dstxes with old signature format
            LogPrintf("CDarksendBroadcastTx::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = tx->GetHash().ToString() + std::to_string(sigTime);

        if(!CMessageSigner::VerifyMessage(keyIDOperator, vchSig, strMessage, strError)) {
            LogPrintf("CDarksendBroadcastTx::CheckSignature -- Got bad dstx signature, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CDarksendBroadcastTx::IsExpired(int nHeight)
{
    // expire confirmed DSTXes after ~1h since confirmation
    return (nConfirmedHeight != -1) && (nHeight - nConfirmedHeight > 24);
}

void CPrivateSendBaseSession::SetNull()
{
    // Both sides
    LOCK(cs_darksend);
    nState = POOL_STATE_IDLE;
    nSessionID = 0;
    nSessionDenom = 0;
    nSessionInputCount = 0;
    vecEntries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    nTimeLastSuccessfulStep = GetTime();
}

void CPrivateSendBaseManager::SetNull()
{
    LOCK(cs_vecqueue);
    vecDarksendQueue.clear();
}

void CPrivateSendBaseManager::CheckQueue()
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if(!lockDS) return; // it's ok to fail here, we run this quite frequently

    // check mixing queue objects for timeouts
    std::vector<CDarksendQueue>::iterator it = vecDarksendQueue.begin();
    while(it != vecDarksendQueue.end()) {
        if((*it).IsExpired()) {
            LogPrint("privatesend", "CPrivateSendBaseManager::%s -- Removing expired queue (%s)\n", __func__, (*it).ToString());
            it = vecDarksendQueue.erase(it);
        } else ++it;
    }
}

bool CPrivateSendBaseManager::GetQueueItemAndTry(CDarksendQueue& dsqRet)
{
    TRY_LOCK(cs_vecqueue, lockDS);
    if(!lockDS) return false; // it's ok to fail here, we run this quite frequently

    for (auto& dsq : vecDarksendQueue) {
        // only try each queue once
        if(dsq.fTried || dsq.IsExpired()) continue;
        dsq.fTried = true;
        dsqRet = dsq;
        return true;
    }

    return false;
}

std::string CPrivateSendBaseSession::GetStateString() const
{
    switch(nState) {
        case POOL_STATE_IDLE:                   return "IDLE";
        case POOL_STATE_QUEUE:                  return "QUEUE";
        case POOL_STATE_ACCEPTING_ENTRIES:      return "ACCEPTING_ENTRIES";
        case POOL_STATE_SIGNING:                return "SIGNING";
        case POOL_STATE_ERROR:                  return "ERROR";
        case POOL_STATE_SUCCESS:                return "SUCCESS";
        default:                                return "UNKNOWN";
    }
}

// Definitions for static data members
std::vector<CAmount> CPrivateSend::vecStandardDenominations;
std::map<uint256, CDarksendBroadcastTx> CPrivateSend::mapDSTX;
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
    vecStandardDenominations.push_back( (10       * COIN)+10000 );
    vecStandardDenominations.push_back( (1        * COIN)+1000 );
    vecStandardDenominations.push_back( (.1       * COIN)+100 );
    vecStandardDenominations.push_back( (.01      * COIN)+10 );
    /* Disabled till we need them
    vecStandardDenominations.push_back( (.001     * COIN)+1 );
    */
}

// check to make sure the collateral provided by the client is valid
bool CPrivateSend::IsCollateralValid(const CTransaction& txCollateral)
{
    if(txCollateral.vout.empty()) return false;
    if(txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;

    for (const auto& txout : txCollateral.vout) {
        nValueOut += txout.nValue;

        bool fAllowData = mnpayments.GetMinMasternodePaymentsProto() > 70208;
        if(!txout.scriptPubKey.IsPayToPublicKeyHash() && !(fAllowData && txout.scriptPubKey.IsUnspendable())) {
            LogPrintf ("CPrivateSend::IsCollateralValid -- Invalid Script, txCollateral=%s", txCollateral.ToString());
            return false;
        }
    }

    for (const auto& txin : txCollateral.vin) {
        Coin coin;
        if(!GetUTXOCoin(txin.prevout, coin)) {
            LogPrint("privatesend", "CPrivateSend::IsCollateralValid -- Unknown inputs in collateral transaction, txCollateral=%s", txCollateral.ToString());
            return false;
        }
        nValueIn += coin.out.nValue;
    }

    //collateral transactions are required to pay out a small fee to the miners
    if(nValueIn - nValueOut < GetCollateralAmount()) {
        LogPrint("privatesend", "CPrivateSend::IsCollateralValid -- did not include enough fees in transaction: fees: %d, txCollateral=%s", nValueOut - nValueIn, txCollateral.ToString());
        return false;
    }

    LogPrint("privatesend", "CPrivateSend::IsCollateralValid -- %s", txCollateral.ToString());

    {
        LOCK(cs_main);
        CValidationState validationState;
        if(!AcceptToMemoryPool(mempool, validationState, MakeTransactionRef(txCollateral), false, NULL, NULL, false, maxTxFee, true)) {
            LogPrint("privatesend", "CPrivateSend::IsCollateralValid -- didn't pass AcceptToMemoryPool()\n");
            return false;
        }
    }

    return true;
}

bool CPrivateSend::IsCollateralAmount(CAmount nInputAmount)
{
    if (mnpayments.GetMinMasternodePaymentsProto() > 70208) {
        // collateral input can be anything between 1x and "max" (including both)
        return (nInputAmount >= GetCollateralAmount() && nInputAmount <= GetMaxCollateralAmount());
    } else { // <= 70208
        // collateral input can be anything between 2x and "max" (including both)
        return (nInputAmount >= GetCollateralAmount() * 2 && nInputAmount <= GetMaxCollateralAmount());
    }
}

/*  Create a nice string to show the denominations
    Function returns as follows (for 4 denominations):
        ( bit on if present )
        bit 0           - 10
        bit 1           - 1
        bit 2           - .1
        bit 3           - .01
        bit 4 and so on - out-of-bounds
        none of above   - non-denom
*/
std::string CPrivateSend::GetDenominationsToString(int nDenom)
{
    std::string strDenom = "";
    int nMaxDenoms = vecStandardDenominations.size();

    if(nDenom >= (1 << nMaxDenoms)) {
        return "out-of-bounds";
    }

    for (int i = 0; i < nMaxDenoms; ++i) {
        if(nDenom & (1 << i)) {
            strDenom += (strDenom.empty() ? "" : "+") + FormatMoney(vecStandardDenominations[i]);
        }
    }

    if(strDenom.empty()) {
        return "non-denom";
    }

    return strDenom;
}

/*  Return a bitshifted integer representing the denominations in this list
    Function returns as follows (for 4 denominations):
        ( bit on if present )
        10        - bit 0
        1         - bit 1
        .1        - bit 2
        .01       - bit 3
        non-denom - 0, all bits off
*/
int CPrivateSend::GetDenominations(const std::vector<CTxOut>& vecTxOut, bool fSingleRandomDenom)
{
    std::vector<std::pair<CAmount, int> > vecDenomUsed;

    // make a list of denominations, with zero uses
    for (const auto& nDenomValue : vecStandardDenominations)
        vecDenomUsed.push_back(std::make_pair(nDenomValue, 0));

    // look for denominations and update uses to 1
    for (const auto& txout : vecTxOut) {
        bool found = false;
        for (auto& s : vecDenomUsed) {
            if(txout.nValue == s.first) {
                s.second = 1;
                found = true;
            }
        }
        if(!found) return 0;
    }

    int nDenom = 0;
    int c = 0;
    // if the denomination is used, shift the bit on
    for (const auto& s : vecDenomUsed) {
        int bit = (fSingleRandomDenom ? GetRandInt(2) : 1) & s.second;
        nDenom |= bit << c++;
        if(fSingleRandomDenom && bit) break; // use just one random denomination
    }

    return nDenom;
}

bool CPrivateSend::GetDenominationsBits(int nDenom, std::vector<int> &vecBitsRet)
{
    // ( bit on if present, 4 denominations example )
    // bit 0 - 100DASH+1
    // bit 1 - 10DASH+1
    // bit 2 - 1DASH+1
    // bit 3 - .1DASH+1

    int nMaxDenoms = vecStandardDenominations.size();

    if(nDenom >= (1 << nMaxDenoms)) return false;

    vecBitsRet.clear();

    for (int i = 0; i < nMaxDenoms; ++i) {
        if(nDenom & (1 << i)) {
            vecBitsRet.push_back(i);
        }
    }

    return !vecBitsRet.empty();
}

int CPrivateSend::GetDenominationsByAmounts(const std::vector<CAmount>& vecAmount)
{
    CScript scriptTmp = CScript();
    std::vector<CTxOut> vecTxOut;

    for (auto it = vecAmount.rbegin(); it != vecAmount.rend(); ++it) {
        CTxOut txout((*it), scriptTmp);
        vecTxOut.push_back(txout);
    }

    return GetDenominations(vecTxOut, true);
}

bool CPrivateSend::IsDenominatedAmount(CAmount nInputAmount)
{
    for (const auto& nDenomValue : vecStandardDenominations)
        if(nInputAmount == nDenomValue)
            return true;
    return false;
}

std::string CPrivateSend::GetMessageByID(PoolMessage nMessageID)
{
    switch (nMessageID) {
        case ERR_ALREADY_HAVE:          return _("Already have that input.");
        case ERR_DENOM:                 return _("No matching denominations found for mixing.");
        case ERR_ENTRIES_FULL:          return _("Entries are full.");
        case ERR_EXISTING_TX:           return _("Not compatible with existing transactions.");
        case ERR_FEES:                  return _("Transaction fees are too high.");
        case ERR_INVALID_COLLATERAL:    return _("Collateral not valid.");
        case ERR_INVALID_INPUT:         return _("Input is not valid.");
        case ERR_INVALID_SCRIPT:        return _("Invalid script detected.");
        case ERR_INVALID_TX:            return _("Transaction not valid.");
        case ERR_MAXIMUM:               return _("Entry exceeds maximum size.");
        case ERR_MN_LIST:               return _("Not in the Masternode list.");
        case ERR_MODE:                  return _("Incompatible mode.");
        case ERR_NON_STANDARD_PUBKEY:   return _("Non-standard public key detected.");
        case ERR_NOT_A_MN:              return _("This is not a Masternode."); // not used
        case ERR_QUEUE_FULL:            return _("Masternode queue is full.");
        case ERR_RECENT:                return _("Last PrivateSend was too recent.");
        case ERR_SESSION:               return _("Session not complete!");
        case ERR_MISSING_TX:            return _("Missing input transaction information.");
        case ERR_VERSION:               return _("Incompatible version.");
        case MSG_NOERR:                 return _("No errors detected.");
        case MSG_SUCCESS:               return _("Transaction created successfully.");
        case MSG_ENTRIES_ADDED:         return _("Your entries added successfully.");
        case ERR_INVALID_INPUT_COUNT:   return _("Invalid input count.");
        default:                        return _("Unknown response.");
    }
}

void CPrivateSend::AddDSTX(const CDarksendBroadcastTx& dstx)
{
    LOCK(cs_mapdstx);
    mapDSTX.insert(std::make_pair(dstx.tx->GetHash(), dstx));
}

CDarksendBroadcastTx CPrivateSend::GetDSTX(const uint256& hash)
{
    LOCK(cs_mapdstx);
    auto it = mapDSTX.find(hash);
    return (it == mapDSTX.end()) ? CDarksendBroadcastTx() : it->second;
}

void CPrivateSend::CheckDSTXes(int nHeight)
{
    LOCK(cs_mapdstx);
    std::map<uint256, CDarksendBroadcastTx>::iterator it = mapDSTX.begin();
    while(it != mapDSTX.end()) {
        if (it->second.IsExpired(nHeight)) {
            mapDSTX.erase(it++);
        } else {
            ++it;
        }
    }
    LogPrint("privatesend", "CPrivateSend::CheckDSTXes -- mapDSTX.size()=%llu\n", mapDSTX.size());
}

void CPrivateSend::UpdatedBlockTip(const CBlockIndex *pindex)
{
    if(pindex && !fLiteMode && masternodeSync.IsMasternodeListSynced()) {
        CheckDSTXes(pindex->nHeight);
    }
}

void CPrivateSend::SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, int posInBlock)
{
    if (tx.IsCoinBase()) return;

    LOCK2(cs_main, cs_mapdstx);

    uint256 txHash = tx.GetHash();
    if (!mapDSTX.count(txHash)) return;

    // When tx is 0-confirmed or conflicted, posInBlock is SYNC_TRANSACTION_NOT_IN_BLOCK and nConfirmedHeight should be set to -1
    mapDSTX[txHash].SetConfirmedHeight(posInBlock == CMainSignals::SYNC_TRANSACTION_NOT_IN_BLOCK ? -1 : pindex->nHeight);
    LogPrint("privatesend", "CPrivateSend::SyncTransaction -- txid=%s\n", txHash.ToString());
}
