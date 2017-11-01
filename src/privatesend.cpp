// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "privatesend.h"

#include "activemasternode.h"
#include "consensus/validation.h"
#include "governance.h"
#include "init.h"
#include "instantx.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "script/sign.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"

#include <boost/lexical_cast.hpp>

CDarkSendEntry::CDarkSendEntry(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut, const CTransaction& txCollateral) :
    txCollateral(txCollateral), addr(CService())
{
    BOOST_FOREACH(CTxIn txin, vecTxIn)
        vecTxDSIn.push_back(txin);
    BOOST_FOREACH(CTxOut txout, vecTxOut)
        vecTxDSOut.push_back(txout);
}

bool CDarkSendEntry::AddScriptSig(const CTxIn& txin)
{
    BOOST_FOREACH(CTxDSIn& txdsin, vecTxDSIn) {
        if(txdsin.prevout == txin.prevout && txdsin.nSequence == txin.nSequence) {
            if(txdsin.fHasSig) return false;

            txdsin.scriptSig = txin.scriptSig;
            txdsin.prevPubKey = txin.prevPubKey;
            txdsin.fHasSig = true;

            return true;
        }
    }

    return false;
}

bool CDarksendQueue::Sign()
{
    if(!fMasterNode) return false;

    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nDenom) + boost::lexical_cast<std::string>(nTime) + boost::lexical_cast<std::string>(fReady);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, activeMasternode.keyMasternode)) {
        LogPrintf("CDarksendQueue::Sign -- SignMessage() failed, %s\n", ToString());
        return false;
    }

    return CheckSignature(activeMasternode.pubKeyMasternode);
}

bool CDarksendQueue::CheckSignature(const CPubKey& pubKeyMasternode)
{
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nDenom) + boost::lexical_cast<std::string>(nTime) + boost::lexical_cast<std::string>(fReady);
    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CDarksendQueue::CheckSignature -- Got bad Masternode queue signature: %s; error: %s\n", ToString(), strError);
        return false;
    }

    return true;
}

bool CDarksendQueue::Relay(CConnman& connman)
{
    std::vector<CNode*> vNodesCopy = connman.CopyNodeVector();
    BOOST_FOREACH(CNode* pnode, vNodesCopy)
        if(pnode->nVersion >= MIN_PRIVATESEND_PEER_PROTO_VERSION)
            connman.PushMessage(pnode, NetMsgType::DSQUEUE, (*this));

    connman.ReleaseNodeVector(vNodesCopy);
    return true;
}

bool CDarksendBroadcastTx::Sign()
{
    if(!fMasterNode) return false;

    std::string strMessage = tx.GetHash().ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, activeMasternode.keyMasternode)) {
        LogPrintf("CDarksendBroadcastTx::Sign -- SignMessage() failed\n");
        return false;
    }

    return CheckSignature(activeMasternode.pubKeyMasternode);
}

bool CDarksendBroadcastTx::CheckSignature(const CPubKey& pubKeyMasternode)
{
    std::string strMessage = tx.GetHash().ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CDarksendBroadcastTx::CheckSignature -- Got bad dstx signature, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CDarksendBroadcastTx::IsExpired(int nHeight)
{
    // expire confirmed DSTXes after ~1h since confirmation
    return (nConfirmedHeight != -1) && (nHeight - nConfirmedHeight > 24);
}

void CPrivateSendBase::SetNull()
{
    // Both sides
    nState = POOL_STATE_IDLE;
    nSessionID = 0;
    nSessionDenom = 0;
    vecEntries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    nTimeLastSuccessfulStep = GetTimeMillis();
}

std::string CPrivateSendBase::GetStateString() const
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

        A note about convertability. Within mixing pools, each denomination
        is convertable to another.

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

    BOOST_FOREACH(const CTxOut txout, txCollateral.vout) {
        nValueOut += txout.nValue;

        if(!txout.scriptPubKey.IsNormalPaymentScript()) {
            LogPrintf ("CPrivateSend::IsCollateralValid -- Invalid Script, txCollateral=%s", txCollateral.ToString());
            return false;
        }
    }

    BOOST_FOREACH(const CTxIn txin, txCollateral.vin) {
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
        if(!AcceptToMemoryPool(mempool, validationState, txCollateral, false, NULL, false, true, true)) {
            LogPrint("privatesend", "CPrivateSend::IsCollateralValid -- didn't pass AcceptToMemoryPool()\n");
            return false;
        }
    }

    return true;
}

/*  Create a nice string to show the denominations
    Function returns as follows (for 4 denominations):
        ( bit on if present )
        bit 0           - 100
        bit 1           - 10
        bit 2           - 1
        bit 3           - .1
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

int CPrivateSend::GetDenominations(const std::vector<CTxDSOut>& vecTxDSOut)
{
    std::vector<CTxOut> vecTxOut;

    BOOST_FOREACH(CTxDSOut out, vecTxDSOut)
        vecTxOut.push_back(out);

    return GetDenominations(vecTxOut);
}

/*  Return a bitshifted integer representing the denominations in this list
    Function returns as follows (for 4 denominations):
        ( bit on if present )
        100       - bit 0
        10        - bit 1
        1         - bit 2
        .1        - bit 3
        non-denom - 0, all bits off
*/
int CPrivateSend::GetDenominations(const std::vector<CTxOut>& vecTxOut, bool fSingleRandomDenom)
{
    std::vector<std::pair<CAmount, int> > vecDenomUsed;

    // make a list of denominations, with zero uses
    BOOST_FOREACH(CAmount nDenomValue, vecStandardDenominations)
        vecDenomUsed.push_back(std::make_pair(nDenomValue, 0));

    // look for denominations and update uses to 1
    BOOST_FOREACH(CTxOut txout, vecTxOut) {
        bool found = false;
        BOOST_FOREACH (PAIRTYPE(CAmount, int)& s, vecDenomUsed) {
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
    BOOST_FOREACH (PAIRTYPE(CAmount, int)& s, vecDenomUsed) {
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

    BOOST_REVERSE_FOREACH(CAmount nAmount, vecAmount) {
        CTxOut txout(nAmount, scriptTmp);
        vecTxOut.push_back(txout);
    }

    return GetDenominations(vecTxOut, true);
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
        default:                        return _("Unknown response.");
    }
}

void CPrivateSend::AddDSTX(const CDarksendBroadcastTx& dstx)
{
    LOCK(cs_mapdstx);
    mapDSTX.insert(std::make_pair(dstx.tx.GetHash(), dstx));
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

void CPrivateSend::SyncTransaction(const CTransaction& tx, const CBlock* pblock)
{
    if (tx.IsCoinBase()) return;

    LOCK2(cs_main, cs_mapdstx);

    uint256 txHash = tx.GetHash();
    if (!mapDSTX.count(txHash)) return;

    // When tx is 0-confirmed or conflicted, pblock is NULL and nConfirmedHeight should be set to -1
    CBlockIndex* pblockindex = NULL;
    if(pblock) {
        uint256 blockHash = pblock->GetHash();
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if(mi == mapBlockIndex.end() || !mi->second) {
            // shouldn't happen
            LogPrint("privatesend", "CPrivateSendClient::SyncTransaction -- Failed to find block %s\n", blockHash.ToString());
            return;
        }
        pblockindex = mi->second;
    }
    mapDSTX[txHash].SetConfirmedHeight(pblockindex ? pblockindex->nHeight : -1);
    LogPrint("privatesend", "CPrivateSendClient::SyncTransaction -- txid=%s\n", txHash.ToString());
}

//TODO: Rename/move to core
void ThreadCheckPrivateSend(CConnman& connman)
{
    if(fLiteMode) return; // disable all Dash specific functionality

    static bool fOneThread;
    if(fOneThread) return;
    fOneThread = true;

    // Make this thread recognisable as the PrivateSend thread
    RenameThread("dash-ps");

    unsigned int nTick = 0;

    while (true)
    {
        MilliSleep(1000);

        // try to sync from all available nodes, one step at a time
        masternodeSync.ProcessTick(connman);

        if(masternodeSync.IsBlockchainSynced() && !ShutdownRequested()) {

            nTick++;

            // make sure to check all masternodes first
            mnodeman.Check();

            // check if we should activate or ping every few minutes,
            // slightly postpone first run to give net thread a chance to connect to some peers
            if(nTick % MASTERNODE_MIN_MNP_SECONDS == 15)
                activeMasternode.ManageState(connman);

            if(nTick % 60 == 0) {
                mnodeman.ProcessMasternodeConnections(connman);
                mnodeman.CheckAndRemove(connman);
                mnpayments.CheckAndRemove();
                instantsend.CheckAndRemove();
            }
            if(fMasterNode && (nTick % (60 * 5) == 0)) {
                mnodeman.DoFullVerificationStep(connman);
            }

            if(nTick % (60 * 5) == 0) {
                governance.DoMaintenance(connman);
            }
        }
    }
}
