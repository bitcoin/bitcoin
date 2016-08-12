// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "coincontrol.h"
#include "consensus/validation.h"
#include "darksend.h"
#include "init.h"
#include "instantx.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "script/sign.h"
#include "txmempool.h"
#include "util.h"

#include <boost/lexical_cast.hpp>

int nPrivateSendRounds = DEFAULT_PRIVATESEND_ROUNDS;
int nPrivateSendAmount = DEFAULT_PRIVATESEND_AMOUNT;
int nLiquidityProvider = 0;
bool fEnablePrivateSend = false;
bool fPrivateSendMultiSession = DEFAULT_PRIVATESEND_MULTISESSION;

CDarksendPool darkSendPool;
CDarkSendSigner darkSendSigner;
std::map<uint256, CDarksendBroadcastTx> mapDarksendBroadcastTxes;
std::vector<CAmount> vecPrivateSendDenominations;

void CDarksendPool::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; // ignore all Dash related functionality
    if(!masternodeSync.IsBlockchainSynced()) return;

    if(strCommand == NetMsgType::DSACCEPT) {

        int nErrorID;

        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            nErrorID = ERR_VERSION;
            LogPrintf("CDarksendPool::ProcessMessage -- DSACCEPT -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        if(!fMasterNode) {
            nErrorID = ERR_NOT_A_MN;
            LogPrintf("CDarksendPool::ProcessMessage -- DSACCEPT -- not a Masternode!\n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        int nDenom;
        CTransaction txCollateral;
        vRecv >> nDenom >> txCollateral;

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if(pmn == NULL) {
            nErrorID = ERR_MN_LIST;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        if(nSessionUsers == 0 && pmn->nLastDsq != 0 &&
            pmn->nLastDsq + mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount)
        {
            LogPrintf("CDarksendPool::ProcessMessage -- DSACCEPT -- last dsq too recent, must wait: addr=%s\n", pfrom->addr.ToString());
            nErrorID = ERR_RECENT;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        if(IsDenomCompatibleWithSession(nDenom, txCollateral, nErrorID)) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSACCEPT -- is compatible, please submit!\n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_ACCEPTED, nErrorID);
            return;
        } else {
            LogPrintf("CDarksendPool::ProcessMessage -- DSACCEPT -- not compatible with existing transactions!\n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

    } else if(strCommand == NetMsgType::DSQUEUE) {
        TRY_LOCK(cs_darksend, lockRecv);
        if(!lockRecv) return;

        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSQUEUE -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            return;
        }

        CDarksendQueue dsq;
        vRecv >> dsq;

        CService addr;
        if(!dsq.GetAddress(addr) || !dsq.CheckSignature() || dsq.IsExpired()) return;

        CMasternode* pmn = mnodeman.Find(dsq.vin);
        if(pmn == NULL) return;

        // if the queue is ready, submit if we can
        if(dsq.fReady) {
            if(!pSubmittedToMasternode) return;
            if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)addr) {
                LogPrintf("CDarksendPool::ProcessMessage -- DSQUEUE -- message doesn't match current Masternode: pSubmittedToMasternode=%s, addr=%s\n", pSubmittedToMasternode->addr.ToString(), addr.ToString());
                return;
            }

            if(nState == POOL_STATE_QUEUE) {
                LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSQUEUE -- PrivateSend queue is ready on masternode %s\n", addr.ToString());
                PrepareDenominate();
            }
        } else {
            BOOST_FOREACH(CDarksendQueue q, vecDarksendQueue)
                if(q.vin == dsq.vin) return;

            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSQUEUE -- nLastDsq: %d  threshold: %d  nDsqCount: %d\n", pmn->nLastDsq, pmn->nLastDsq + mnodeman.size()/5, mnodeman.nDsqCount);
            //don't allow a few nodes to dominate the queuing process
            if(pmn->nLastDsq != 0 &&
                pmn->nLastDsq + mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount) {
                LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSQUEUE -- Masternode %s is sending too many dsq messages\n", pmn->addr.ToString());
                return;
            }
            mnodeman.nDsqCount++;
            pmn->nLastDsq = mnodeman.nDsqCount;
            pmn->allowFreeTx = true;

            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSQUEUE -- new PrivateSend queue object from masternode %s\n", addr.ToString());
            vecDarksendQueue.push_back(dsq);
            dsq.Relay();
            dsq.nTime = GetTime();
        }

    } else if(strCommand == NetMsgType::DSVIN) {
        int nErrorID;

        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            nErrorID = ERR_VERSION;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        if(!fMasterNode) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- not a Masternode!\n");
            nErrorID = ERR_NOT_A_MN;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        std::vector<CTxIn> vecTxIn;
        CAmount nAmount;
        CTransaction txCollateral;
        std::vector<CTxOut> vecTxOut;
        vRecv >> vecTxIn >> nAmount >> txCollateral >> vecTxOut;

        //do we have enough users in the current session?
        if(nSessionUsers < GetMaxPoolTransactions()) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- session not complete!\n");
            nErrorID = ERR_SESSION;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        //do we have the same denominations as the current session?
        if(!IsOutputsCompatibleWithSessionDenom(vecTxOut)) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- not compatible with existing transactions!\n");
            nErrorID = ERR_EXISTING_TX;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
            return;
        }

        //check it like a transaction
        {
            CAmount nValueIn = 0;
            CAmount nValueOut = 0;
            bool fMissingTx = false;

            CMutableTransaction tx;

            BOOST_FOREACH(const CTxOut txout, vecTxOut) {
                nValueOut += txout.nValue;
                tx.vout.push_back(txout);

                if(txout.scriptPubKey.size() != 25) {
                    LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- non-standard pubkey detected! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
                    nErrorID = ERR_NON_STANDARD_PUBKEY;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                    return;
                }
                if(!txout.scriptPubKey.IsNormalPaymentScript()) {
                    LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- invalid script! scriptPubKey=%s\n", ScriptToAsmStr(txout.scriptPubKey));
                    nErrorID = ERR_INVALID_SCRIPT;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                    return;
                }
            }

            BOOST_FOREACH(const CTxIn txin, vecTxIn) {
                tx.vin.push_back(txin);

                LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSVIN -- txin=%s\n", txin.ToString());

                CTransaction txPrev;
                uint256 hash;
                if(GetTransaction(txin.prevout.hash, txPrev, Params().GetConsensus(), hash, true)) {
                    if(txPrev.vout.size() > txin.prevout.n)
                        nValueIn += txPrev.vout[txin.prevout.n].nValue;
                } else {
                    fMissingTx = true;
                }
            }

            if(nValueIn > DARKSEND_POOL_MAX) {
                LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- more than PrivateSend pool max! nValueIn: %lld, tx=%s", nValueIn, tx.ToString());
                nErrorID = ERR_MAXIMUM;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                return;
            }

            if(fMissingTx) {
                LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- missing input! tx=%s", tx.ToString());
                nErrorID = ERR_MISSING_TX;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                return;
            }

            // Allow lowest denom (at max) as a a fee. Normally shouldn't happen though.
            // TODO: Or do not allow fees at all?
            if(nValueIn - nValueOut > vecPrivateSendDenominations.back()) {
                LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- fees are too high! fees: %lld, tx=%s", nValueIn - nValueOut, tx.ToString());
                nErrorID = ERR_FEES;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                return;
            }

            {
                LOCK(cs_main);
                CValidationState validationState;
                mempool.PrioritiseTransaction(tx.GetHash(), tx.GetHash().ToString(), 1000, 0.1*COIN);
                if(!AcceptToMemoryPool(mempool, validationState, CTransaction(tx), false, NULL, false, true, true)) {
                    LogPrintf("CDarksendPool::ProcessMessage -- DSVIN -- transaction not valid! tx=%s", tx.ToString());
                    nErrorID = ERR_INVALID_TX;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
                    return;
                }
            }
        }

        if(AddEntry(vecTxIn, nAmount, txCollateral, vecTxOut, nErrorID)) {
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_ACCEPTED, nErrorID);
            CheckPool();

            RelayStatus(MASTERNODE_RESET);
        } else {
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, nErrorID);
        }

    } else if(strCommand == NetMsgType::DSSTATUSUPDATE) {
        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSSTATUSUPDATE -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr) {
            //LogPrintf("CDarksendPool::ProcessMessage -- DSSTATUSUPDATE -- message doesn't match current Masternode: pSubmittedToMasternode %s addr %s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int nMsgSessionID;
        int nMsgState;
        int nMsgEntriesCount;
        int nMsgAccepted;
        int nMsgErrorID;
        vRecv >> nMsgSessionID >> nMsgState >> nMsgEntriesCount >> nMsgAccepted >> nMsgErrorID;

        LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSSTATUSUPDATE -- nMsgState: %d  nEntriesCount: %d  nMsgAccepted: %d  error: %s\n", nMsgState, nEntriesCount, nMsgAccepted, GetMessageByID(nMsgErrorID));

        if((nMsgAccepted != 1 && nMsgAccepted != 0) && nSessionID != nMsgSessionID) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSSTATUSUPDATE -- message doesn't match current PrivateSend session: nSessionID: %d nMsgSessionID: %d\n", nSessionID, nMsgSessionID);
            return;
        }

        UpdatePoolStateOnClient(nMsgState, nMsgEntriesCount, nMsgAccepted, nMsgErrorID, nMsgSessionID);

    } else if(strCommand == NetMsgType::DSSIGNFINALTX) {

        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSSIGNFINALTX -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            return;
        }

        std::vector<CTxIn> vecTxIn;
        vRecv >> vecTxIn;

        bool fSuccess = true;
        int nTxInIndex = 0;
        int nTxInsCount = (int)vecTxIn.size();

        BOOST_FOREACH(const CTxIn txin, vecTxIn) {
            nTxInIndex++;
            if(!AddScriptSig(txin)) {
                fSuccess = false;
                break;
            }
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSSIGNFINALTX -- AddScriptSig() %d/%d success\n", nTxInIndex, nTxInsCount);
        }

        if(fSuccess) {
            CheckPool();
            RelayStatus(MASTERNODE_RESET);
        } else {
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSSIGNFINALTX -- AddScriptSig() failed at %d/%d, session: %d\n", nTxInIndex, nTxInsCount, nSessionID);
        }
    } else if(strCommand == NetMsgType::DSFINALTX) {
        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSFINALTX -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr) {
            //LogPrintf("CDarksendPool::ProcessMessage -- DSFINALTX -- message doesn't match current Masternode: pSubmittedToMasternode %s addr %s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int nMsgSessionID;
        CTransaction txNew;
        vRecv >> nMsgSessionID >> txNew;

        if(nSessionID != nMsgSessionID) {
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSFINALTX -- message doesn't match current PrivateSend session: nSessionID: %d  nMsgSessionID: %d\n", nSessionID, nMsgSessionID);
            return;
        }

        //check to see if input is spent already? (and probably not confirmed)
        SignFinalTransaction(txNew, pfrom);

    } else if(strCommand == NetMsgType::DSCOMPLETE) {

        if(pfrom->nVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) {
            LogPrintf("CDarksendPool::ProcessMessage -- DSCOMPLETE -- incompatible version! nVersion: %d\n", pfrom->nVersion);
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr) {
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSCOMPLETE -- message doesn't match current Masternode: pSubmittedToMasternode=%s  addr=%s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int  nMsgSessionID;
        bool fMsgError;
        int  nMsgErrorID;
        vRecv >> nMsgSessionID >> fMsgError >> nMsgErrorID;

        if(nSessionID != nMsgSessionID) {
            LogPrint("privatesend", "CDarksendPool::ProcessMessage -- DSCOMPLETE -- message doesn't match current PrivateSend session: nSessionID: %d  nMsgSessionID: %d\n", darkSendPool.nSessionID, nMsgSessionID);
            return;
        }

        CompletedTransaction(fMsgError, nMsgErrorID);
    }
}

void CDarksendPool::InitDenominations()
{
    vecPrivateSendDenominations.clear();
    /* Denominations

        A note about convertability. Within mixing pools, each denomination
        is convertable to another.

        For example:
        1DRK+1000 == (.1DRK+100)*10
        10DRK+10000 == (1DRK+1000)*10
    */
    vecPrivateSendDenominations.push_back( (100      * COIN)+100000 );
    vecPrivateSendDenominations.push_back( (10       * COIN)+10000 );
    vecPrivateSendDenominations.push_back( (1        * COIN)+1000 );
    vecPrivateSendDenominations.push_back( (.1       * COIN)+100 );
    /* Disabled till we need them
    vecPrivateSendDenominations.push_back( (.01      * COIN)+10 );
    vecPrivateSendDenominations.push_back( (.001     * COIN)+1 );
    */
}

void CDarksendPool::ResetPool()
{
    nCachedLastSuccessBlock = 0;
    txMyCollateral = CMutableTransaction();
    vecMasternodesUsed.clear();
    UnlockCoins();
    SetNull();
}

void CDarksendPool::SetNull()
{
    // MN side
    nSessionUsers = 0;
    vecSessionCollateral.clear();

    // Client side
    nEntriesCount = 0;
    fLastEntryAccepted = false;
    nAcceptedEntriesCount = 0;
    fSessionFoundMasternode = false;

    // Both sides
    nState = POOL_STATE_IDLE;
    nSessionID = 0;
    nSessionDenom = 0;
    vecEntries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    nLastTimeChanged = GetTimeMillis();
}

//
// Unlock coins after mixing fails or succeeds
//
void CDarksendPool::UnlockCoins()
{
    while(true) {
        TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
        if(!lockWallet) {MilliSleep(50); continue;}
        BOOST_FOREACH(COutPoint outpoint, vecOutPointLocked)
            pwalletMain->UnlockCoin(outpoint);
        break;
    }

    vecOutPointLocked.clear();
}

std::string CDarksendPool::GetStatus()
{
    static int nStatusMessageProgress = 0;
    nStatusMessageProgress += 10;
    std::string strSuffix = "";

    if((pCurrentBlockIndex && pCurrentBlockIndex->nHeight - nCachedLastSuccessBlock < nMinBlockSpacing) || !masternodeSync.IsBlockchainSynced())
        return strAutoDenomResult;

    switch(nState) {
        case POOL_STATE_IDLE:
            return _("PrivateSend is idle.");
        case POOL_STATE_ACCEPTING_ENTRIES:
            if(nEntriesCount == 0) {
                nStatusMessageProgress = 0;
                return strAutoDenomResult;
            } else if(fLastEntryAccepted) {
                if(nStatusMessageProgress % 10 > 8) {
                    fLastEntryAccepted = false;
                    nStatusMessageProgress = 0;
                }
                return _("PrivateSend request complete:") + " " + _("Your transaction was accepted into the pool!");
            } else {
                if(     nStatusMessageProgress % 70 <= 40) return strprintf(_("Submitted following entries to masternode: %u / %d"), nEntriesCount, GetMaxPoolTransactions());
                else if(nStatusMessageProgress % 70 <= 50) strSuffix = ".";
                else if(nStatusMessageProgress % 70 <= 60) strSuffix = "..";
                else if(nStatusMessageProgress % 70 <= 70) strSuffix = "...";
                return strprintf(_("Submitted to masternode, waiting for more entries ( %u / %d ) %s"), nEntriesCount, GetMaxPoolTransactions(), strSuffix);
            }
        case POOL_STATE_SIGNING:
            if(     nStatusMessageProgress % 70 <= 40) return _("Found enough users, signing ...");
            else if(nStatusMessageProgress % 70 <= 50) strSuffix = ".";
            else if(nStatusMessageProgress % 70 <= 60) strSuffix = "..";
            else if(nStatusMessageProgress % 70 <= 70) strSuffix = "...";
            return strprintf(_("Found enough users, signing ( waiting %s )"), strSuffix);
        case POOL_STATE_TRANSMISSION:
            return _("Transmitting final transaction.");
        case POOL_STATE_FINALIZE_TRANSACTION:
            return _("Finalizing transaction.");
        case POOL_STATE_ERROR:
            return _("PrivateSend request incomplete:") + " " + strLastMessage + " " + _("Will retry...");
        case POOL_STATE_SUCCESS:
            return _("PrivateSend request complete:") + " " + strLastMessage;
        case POOL_STATE_QUEUE:
            if(     nStatusMessageProgress % 70 <= 30) strSuffix = ".";
            else if(nStatusMessageProgress % 70 <= 50) strSuffix = "..";
            else if(nStatusMessageProgress % 70 <= 70) strSuffix = "...";
            return strprintf(_("Submitted to masternode, waiting in queue %s"), strSuffix);;
       default:
            return strprintf(_("Unknown state: id = %u"), nState);
    }
}

//
// Check the mixing progress and send client updates if a Masternode
//
void CDarksendPool::CheckPool()
{
    if(fMasterNode) {
        LogPrint("privatesend", "CDarksendPool::CheckPool -- entries count %lu\n", GetEntriesCount());

        // If entries is full, then move on to the next phase
        if(nState == POOL_STATE_ACCEPTING_ENTRIES && GetEntriesCount() >= GetMaxPoolTransactions()) {
            LogPrint("privatesend", "CDarksendPool::CheckPool -- TRYING TRANSACTION\n");
            SetState(POOL_STATE_FINALIZE_TRANSACTION);
        }
    }

    // create the finalized transaction for distribution to the clients
    if(nState == POOL_STATE_FINALIZE_TRANSACTION) {
        LogPrint("privatesend", "CDarksendPool::CheckPool -- FINALIZE TRANSACTIONS\n");
        SetState(POOL_STATE_SIGNING);

        if(!fMasterNode) return;

        CMutableTransaction txNew;

        // make our new transaction
        for(int i = 0; i < GetEntriesCount(); i++) {
            BOOST_FOREACH(const CTxDSOut& txdsout, vecEntries[i].vecTxDSOut)
                txNew.vout.push_back(txdsout);

            BOOST_FOREACH(const CTxDSIn& txdsin, vecEntries[i].vecTxDSIn)
                txNew.vin.push_back(txdsin);
        }

        // BIP69 https://github.com/kristovatlas/bips/blob/master/bip-0069.mediawiki
        sort(txNew.vin.begin(), txNew.vin.end());
        sort(txNew.vout.begin(), txNew.vout.end());

        finalMutableTransaction = txNew;
        LogPrint("privatesend", "CDarksendPool::CheckPool -- finalMutableTransaction=%s", txNew.ToString());

        // request signatures from clients
        RelayFinalTransaction(finalMutableTransaction);
    }

    // If we have all of the signatures, try to compile the transaction
    if(fMasterNode && nState == POOL_STATE_SIGNING && IsSignaturesComplete()) {
        LogPrint("privatesend", "CDarksendPool::CheckPool -- SIGNING\n");
        SetState(POOL_STATE_TRANSMISSION);
        CheckFinalTransaction();
    }

    // reset if we're here for 10 seconds
    if((nState == POOL_STATE_ERROR || nState == POOL_STATE_SUCCESS) && GetTimeMillis() - nLastTimeChanged >= 10000) {
        LogPrint("privatesend", "CDarksendPool::CheckPool -- timeout, RESETTING\n");
        UnlockCoins();
        SetNull();
        if(fMasterNode) RelayStatus(MASTERNODE_RESET);
    }
}

void CDarksendPool::CheckFinalTransaction()
{
    if(!fMasterNode) return; // check and relay final tx only on masternode

    CTransaction finalTransaction = CTransaction(finalMutableTransaction);

    LogPrint("privatesend", "CDarksendPool::CheckFinalTransaction -- finalTransaction=%s", finalTransaction.ToString());

    {
        // See if the transaction is valid
        TRY_LOCK(cs_main, lockMain);
        CValidationState validationState;
        mempool.PrioritiseTransaction(finalTransaction.GetHash(), finalTransaction.GetHash().ToString(), 1000, 0.1*COIN);
        if(!lockMain || !AcceptToMemoryPool(mempool, validationState, finalTransaction, false, NULL, false, true, true))
        {
            LogPrintf("CDarksendPool::CheckFinalTransaction -- AcceptToMemoryPool() error: Transaction not valid\n");
            SetNull();

            // not much we can do in this case
            SetState(POOL_STATE_ACCEPTING_ENTRIES);
            RelayCompletedTransaction(true, ERR_INVALID_TX);
            return;
        }
    }

    LogPrintf("CDarksendPool::CheckFinalTransaction -- CREATING DSTX\n");

    // create and sign masternode dstx transaction
    if(!mapDarksendBroadcastTxes.count(finalTransaction.GetHash())) {
        CDarksendBroadcastTx dstx(finalTransaction, activeMasternode.vin, GetAdjustedTime());
        dstx.Sign();
        mapDarksendBroadcastTxes.insert(std::make_pair(finalTransaction.GetHash(), dstx));
    }

    LogPrintf("CDarksendPool::CheckFinalTransaction -- TRANSMITTING DSTX\n");

    CInv inv(MSG_DSTX, finalTransaction.GetHash());
    RelayInv(inv);

    // Tell the clients it was successful
    RelayCompletedTransaction(false, MSG_SUCCESS);

    // Randomly charge clients
    ChargeRandomFees();

    // Reset
    LogPrint("privatesend", "CDarksendPool::CheckFinalTransaction -- COMPLETED -- RESETTING\n");
    SetNull();
    RelayStatus(MASTERNODE_RESET);
}

//
// Charge clients a fee if they're abusive
//
// Why bother? Darksend uses collateral to ensure abuse to the process is kept to a minimum.
// The submission and signing stages in Darksend are completely separate. In the cases where
// a client submits a transaction then refused to sign, there must be a cost. Otherwise they
// would be able to do this over and over again and bring the mixing to a hault.
//
// How does this work? Messages to Masternodes come in via NetMsgType::DSVIN, these require a valid collateral
// transaction for the client to be able to enter the pool. This transaction is kept by the Masternode
// until the transaction is either complete or fails.
//
void CDarksendPool::ChargeFees()
{
    if(!fMasterNode) return;

    //we don't need to charge collateral for every offence.
    int nOffences = 0;
    int r = insecure_rand()%100;
    if(r > 33) return;

    if(nState == POOL_STATE_ACCEPTING_ENTRIES) {
        BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
            bool fFound = false;
            BOOST_FOREACH(const CDarkSendEntry& entry, vecEntries)
                if(entry.txCollateral == txCollateral)
                    fFound = true;

            // This queue entry didn't send us the promised transaction
            if(!fFound) {
                LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't send transaction). Found offence.\n");
                nOffences++;
            }
        }
    }

    if(nState == POOL_STATE_SIGNING) {
        // who didn't sign?
        BOOST_FOREACH(const CDarkSendEntry entry, vecEntries) {
            BOOST_FOREACH(const CTxDSIn txdsin, entry.vecTxDSIn) {
                if(!txdsin.fHasSig) {
                    LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't sign). Found offence\n");
                    nOffences++;
                }
            }
        }
    }

    //pick random client to charge
    r = insecure_rand()%100;
    int nTarget = 0;

    //mostly offending?
    if(nOffences >= Params().PoolMaxTransactions() - 1 && r > 33) return;

    //everyone is an offender? That's not right
    if(nOffences >= Params().PoolMaxTransactions()) return;

    //charge one of the offenders randomly
    if(nOffences > 1) nTarget = 50;

    if(nState == POOL_STATE_ACCEPTING_ENTRIES) {
        BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
            bool fFound = false;
            BOOST_FOREACH(const CDarkSendEntry& entry, vecEntries)
                if(entry.txCollateral == txCollateral)
                    fFound = true;

            // This queue entry didn't send us the promised transaction
            if(!fFound && r > nTarget) {
                LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't send transaction). charging fees.\n");

                CWalletTx wtxCollateral = CWalletTx(pwalletMain, txCollateral);

                // Broadcast
                // This must not fail. The transaction has already been signed and recorded.
                if(!wtxCollateral.AcceptToMemoryPool(true)) {
                    LogPrintf("CDarksendPool::ChargeFees -- Error: Transaction not valid\n");
                    return;
                }
                wtxCollateral.RelayWalletTransaction();
                return;
            }
        }
    }

    if(nState == POOL_STATE_SIGNING) {
        // who didn't sign?
        BOOST_FOREACH(const CDarkSendEntry entry, vecEntries) {
            BOOST_FOREACH(const CTxDSIn txdsin, entry.vecTxDSIn) {
                if(!txdsin.fHasSig && r > nTarget) {
                    LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't sign). charging fees.\n");

                    CWalletTx wtxCollateral = CWalletTx(pwalletMain, entry.txCollateral);

                    // Broadcast
                    if(!wtxCollateral.AcceptToMemoryPool(false)) {
                        // This must not fail. The transaction has already been signed and recorded.
                        LogPrintf("CDarksendPool::ChargeFees -- Error: Transaction not valid\n");
                    }
                    wtxCollateral.RelayWalletTransaction();
                    return;
                }
            }
        }
    }
}

/*
    Charge the collateral randomly.
    Mixing is completely free, to pay miners we randomly pay the collateral of users.

    Collateral Fee Charges:

    Being that mixing has "no fees" we need to have some kind of cost associated
    with using it to stop abuse. Otherwise it could serve as an attack vector and
    allow endless transaction that would bloat Dash and make it unusable. To
    stop these kinds of attacks 1 in 10 successful transactions are charged. This
    adds up to a cost of 0.001DRK per transaction on average.
*/
void CDarksendPool::ChargeRandomFees()
{
    if(!fMasterNode) return;

    BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
        int r = insecure_rand()%100;

        if(r > 10) return;

        LogPrintf("CDarksendPool::ChargeRandomFees -- charging random fees, collateral, txCollateral=%s", txCollateral.ToString());

        CWalletTx wtxCollateral = CWalletTx(pwalletMain, txCollateral);

        // This must not fail. The transaction has already been signed and recorded.
        if(!wtxCollateral.AcceptToMemoryPool(true)) {
            LogPrintf("CDarksendPool::ChargeRandomFees -- Error: Transaction not valid, txCollateral=%s", txCollateral.ToString());
            return;
        }

        // Broadcast
        wtxCollateral.RelayWalletTransaction();
    }
}

//
// Check for various timeouts (queue objects, mixing, etc)
//
void CDarksendPool::CheckTimeout()
{
    // check mixing queue objects for timeouts
    int c = 0;
    std::vector<CDarksendQueue>::iterator it = vecDarksendQueue.begin();
    while(it != vecDarksendQueue.end()) {
        if((*it).IsExpired()) {
            LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Removing expired queue entry: %d\n", c);
            it = vecDarksendQueue.erase(it);
        } else ++it;
        c++;
    }

    if(!fEnablePrivateSend && !fMasterNode) return;

    // catching hanging sessions
    if(!fMasterNode) {
        switch(nState) {
            case POOL_STATE_TRANSMISSION:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Session complete -- Running CheckPool\n");
                CheckPool();
                break;
            case POOL_STATE_ERROR:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Pool error -- Running CheckPool\n");
                CheckPool();
                break;
            case POOL_STATE_SUCCESS:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Pool success -- Running CheckPool\n");
                CheckPool();
                break;
        }
    }

    int nLagTime = 0;
    if(!fMasterNode) nLagTime = 10000; //if we're the client, give the server a few extra seconds before resetting.

    if(nState == POOL_STATE_ACCEPTING_ENTRIES || nState == POOL_STATE_QUEUE) {
        c = 0;

        // check for a timeout and reset if needed
        std::vector<CDarkSendEntry>::iterator it2 = vecEntries.begin();
        while(it2 != vecEntries.end()) {
            if((*it2).IsExpired()) {
                LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Removing expired entry: %d\n", c);
                it2 = vecEntries.erase(it2);
                if(GetEntriesCount() == 0) {
                    UnlockCoins();
                    SetNull();
                }
                if(fMasterNode)
                    RelayStatus(MASTERNODE_RESET);
            } else ++it2;
            c++;
        }

        if(GetTimeMillis() - nLastTimeChanged >= DARKSEND_QUEUE_TIMEOUT*1000 + nLagTime) {
            UnlockCoins();
            SetNull();
        }
    } else if(GetTimeMillis() - nLastTimeChanged >= DARKSEND_QUEUE_TIMEOUT*1000 + nLagTime) {
        LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Session timed out (%ds) -- resetting\n", DARKSEND_QUEUE_TIMEOUT);
        UnlockCoins();
        SetNull();

        SetState(POOL_STATE_ERROR);
        strLastMessage = _("Session timed out.");
    }

    if(nState == POOL_STATE_SIGNING && GetTimeMillis() - nLastTimeChanged >= DARKSEND_SIGNING_TIMEOUT*1000 + nLagTime) {
        LogPrint("privatesend", "CDarksendPool::CheckTimeout -- Session timed out (%ds) -- restting\n", DARKSEND_SIGNING_TIMEOUT);
        ChargeFees();
        UnlockCoins();
        SetNull();

        SetState(POOL_STATE_ERROR);
        strLastMessage = _("Signing timed out.");
    }
}

/*
    Check to see if we're ready for submissions from clients
    After receiving multiple dsa messages, the queue will switch to "accepting entries"
    which is the active state right before merging the transaction
*/
void CDarksendPool::CheckForCompleteQueue()
{
    if(!fEnablePrivateSend && !fMasterNode) return;

    if(nState == POOL_STATE_QUEUE && nSessionUsers == GetMaxPoolTransactions()) {
        SetState(POOL_STATE_ACCEPTING_ENTRIES);

        CDarksendQueue dsq(nSessionDenom, activeMasternode.vin, GetTime(), true);
        dsq.Sign();
        dsq.Relay();
    }
}

// check to see if the signature is valid
bool CDarksendPool::IsSignatureValid(const CScript& scriptSig, const CTxIn& txin)
{
    CMutableTransaction txNew;
    txNew.vin.clear();
    txNew.vout.clear();

    int i = 0;
    int nTxInIndex = -1;
    CScript sigPubKey = CScript();

    BOOST_FOREACH(CDarkSendEntry& entry, vecEntries) {

        BOOST_FOREACH(const CTxDSOut& txdsout, entry.vecTxDSOut)
            txNew.vout.push_back(txdsout);

        BOOST_FOREACH(const CTxDSIn& txdsin, entry.vecTxDSIn) {
            txNew.vin.push_back(txdsin);

            if(txdsin == txin) {
                nTxInIndex = i;
                sigPubKey = txdsin.prevPubKey;
            }
            i++;
        }
    }

    if(nTxInIndex >= 0) { //might have to do this one input at a time?
        txNew.vin[nTxInIndex].scriptSig = scriptSig;
        LogPrint("privatesend", "CDarksendPool::IsSignatureValid -- Sign with sig %s\n", ScriptToAsmStr(scriptSig).substr(0,24));
        if(!VerifyScript(txNew.vin[nTxInIndex].scriptSig, sigPubKey, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, MutableTransactionSignatureChecker(&txNew, nTxInIndex))) {
            LogPrint("privatesend", "CDarksendPool::IsSignatureValid -- Signing -- Error signing input %d\n", nTxInIndex);
            return false;
        }
    }

    LogPrint("privatesend", "CDarksendPool::IsSignatureValid -- Signing -- Successfully validated input\n");
    return true;
}

// check to make sure the collateral provided by the client is valid
bool CDarksendPool::IsCollateralValid(const CTransaction& txCollateral)
{
    if(txCollateral.vout.size() < 1) return false;
    if(txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;
    bool fMissingTx = false;

    BOOST_FOREACH(const CTxOut txout, txCollateral.vout) {
        nValueOut += txout.nValue;

        if(!txout.scriptPubKey.IsNormalPaymentScript()) {
            LogPrintf ("CDarksendPool::IsCollateralValid -- Invalid Script, txCollateral=%s", txCollateral.ToString());
            return false;
        }
    }

    BOOST_FOREACH(const CTxIn txin, txCollateral.vin) {
        CTransaction txPrev;
        uint256 hash;
        if(GetTransaction(txin.prevout.hash, txPrev, Params().GetConsensus(), hash, true)) {
            if(txPrev.vout.size() > txin.prevout.n)
                nValueIn += txPrev.vout[txin.prevout.n].nValue;
        } else {
            fMissingTx = true;
        }
    }

    if(fMissingTx) {
        LogPrint("privatesend", "CDarksendPool::IsCollateralValid -- Unknown inputs in collateral transaction, txCollateral=%s", txCollateral.ToString());
        return false;
    }

    //collateral transactions are required to pay out DARKSEND_COLLATERAL as a fee to the miners
    if(nValueIn - nValueOut < DARKSEND_COLLATERAL) {
        LogPrint("privatesend", "CDarksendPool::IsCollateralValid -- did not include enough fees in transaction: fees: %d, txCollateral=%s", nValueOut - nValueIn, txCollateral.ToString());
        return false;
    }

    LogPrint("privatesend", "CDarksendPool::IsCollateralValid -- %s", txCollateral.ToString());

    {
        LOCK(cs_main);
        CValidationState validationState;
        if(!AcceptToMemoryPool(mempool, validationState, txCollateral, false, NULL, false, true, true)) {
            LogPrint("privatesend", "CDarksendPool::IsCollateralValid -- didn't pass AcceptToMemoryPool()\n");
            return false;
        }
    }

    return true;
}


//
// Add a clients transaction to the pool
//
bool CDarksendPool::AddEntry(const std::vector<CTxIn>& vecTxInNew, const CAmount nAmount, const CTransaction& txCollateral, const std::vector<CTxOut>& vecTxOutNew, int& nErrorID)
{
    if(!fMasterNode) return false;

    BOOST_FOREACH(CTxIn txin, vecTxInNew) {
        if(txin.prevout.IsNull() || nAmount < 0) {
            LogPrint("privatesend", "CDarksendPool::AddEntry -- input not valid!\n");
            nErrorID = ERR_INVALID_INPUT;
            nSessionUsers--;
            return false;
        }
    }

    if(!IsCollateralValid(txCollateral)) {
        LogPrint("privatesend", "CDarksendPool::AddEntry -- collateral not valid!\n");
        nErrorID = ERR_INVALID_COLLATERAL;
        nSessionUsers--;
        return false;
    }

    if(GetEntriesCount() >= GetMaxPoolTransactions()) {
        LogPrint("privatesend", "CDarksendPool::AddEntry -- entries is full!\n");
        nErrorID = ERR_ENTRIES_FULL;
        nSessionUsers--;
        return false;
    }

    BOOST_FOREACH(CTxIn txin, vecTxInNew) {
        LogPrint("privatesend", "looking for txin -- %s\n", txin.ToString());
        BOOST_FOREACH(const CDarkSendEntry& entry, vecEntries) {
            BOOST_FOREACH(const CTxDSIn& txdsin, entry.vecTxDSIn) {
                if((CTxIn)txdsin == txin) {
                    LogPrint("privatesend", "CDarksendPool::AddEntry -- found in txin\n");
                    nErrorID = ERR_ALREADY_HAVE;
                    nSessionUsers--;
                    return false;
                }
            }
        }
    }

    CDarkSendEntry entry;
    entry.Add(vecTxInNew, nAmount, txCollateral, vecTxOutNew);
    vecEntries.push_back(entry);

    LogPrint("privatesend", "CDarksendPool::AddEntry -- adding entry %s\n", vecTxInNew[0].ToString());
    nErrorID = MSG_ENTRIES_ADDED;

    return true;
}

bool CDarksendPool::AddScriptSig(const CTxIn& txinNew)
{
    LogPrint("privatesend", "CDarksendPool::AddScriptSig -- new sig, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0,24));

    BOOST_FOREACH(const CDarkSendEntry& entry, vecEntries) {
        BOOST_FOREACH(const CTxDSIn& txdsin, entry.vecTxDSIn) {
            if(txdsin.scriptSig == txinNew.scriptSig) {
                LogPrint("privatesend", "CDarksendPool::AddScriptSig -- already exists\n");
                return false;
            }
        }
    }

    if(!IsSignatureValid(txinNew.scriptSig, txinNew)) {
        LogPrint("privatesend", "CDarksendPool::AddScriptSig -- Invalid Sig\n");
        return false;
    }

    LogPrint("privatesend", "CDarksendPool::AddScriptSig -- scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig));

    BOOST_FOREACH(CTxIn& txin, finalMutableTransaction.vin) {
        if(txinNew.prevout == txin.prevout && txin.nSequence == txinNew.nSequence) {
            txin.scriptSig = txinNew.scriptSig;
            txin.prevPubKey = txinNew.prevPubKey;
            LogPrint("privatesend", "CDarksendPool::AddScriptSig -- adding to finalMutableTransaction, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0,24));
        }
    }
    for(int i = 0; i < GetEntriesCount(); i++) {
        if(vecEntries[i].AddScriptSig(txinNew)) {
            LogPrint("privatesend", "CDarksendPool::AddScriptSig -- adding to entries, scriptSig=%s\n", ScriptToAsmStr(txinNew.scriptSig).substr(0,24));
            return true;
        }
    }

    LogPrintf("CDarksendPool::AddScriptSig -- Couldn't set sig!\n" );
    return false;
}

// Check to make sure everything is signed
bool CDarksendPool::IsSignaturesComplete()
{
    BOOST_FOREACH(const CDarkSendEntry& entry, vecEntries)
        BOOST_FOREACH(const CTxDSIn& txdsin, entry.vecTxDSIn)
            if(!txdsin.fHasSig) return false;

    return true;
}

//
// Execute a mixing denomination via a Masternode.
// This is only ran from clients
//
void CDarksendPool::SendDarksendDenominate(const std::vector<CTxIn>& vecTxIn, const std::vector<CTxOut>& vecTxOut, CAmount nAmount)
{
    if(fMasterNode) {
        LogPrintf("CDarksendPool::SendDarksendDenominate -- PrivateSend from a Masternode is not supported currently.\n");
        return;
    }

    if(txMyCollateral == CMutableTransaction()) {
        LogPrintf ("CDarksendPool:SendDarksendDenominate -- PrivateSend collateral not set");
        return;
    }

    // lock the funds we're going to use
    BOOST_FOREACH(CTxIn txin, txMyCollateral.vin)
        vecOutPointLocked.push_back(txin.prevout);

    BOOST_FOREACH(CTxIn txin, vecTxIn)
        vecOutPointLocked.push_back(txin.prevout);

    //BOOST_FOREACH(CTxOut o, vecTxOut)
    //    LogPrintf(" vecTxOut - %s\n", o.ToString());


    // we should already be connected to a Masternode
    if(!fSessionFoundMasternode) {
        LogPrintf("CDarksendPool::SendDarksendDenominate -- No Masternode has been selected yet.\n");
        UnlockCoins();
        SetNull();
        return;
    }

    if(!CheckDiskSpace()) {
        UnlockCoins();
        SetNull();
        fEnablePrivateSend = false;
        LogPrintf("CDarksendPool::SendDarksendDenominate -- Not enough disk space, disabling PrivateSend.\n");
        return;
    }

    SetState(POOL_STATE_ACCEPTING_ENTRIES);
    strLastMessage = "";

    LogPrintf("CDarksendPool::SendDarksendDenominate -- Added transaction to pool.\n");

    //check it against the memory pool to make sure it's valid
    {
        CValidationState validationState;
        CMutableTransaction tx;

        BOOST_FOREACH(const CTxIn& txin, vecTxIn) {
            LogPrint("privatesend", "CDarksendPool::SendDarksendDenominate -- txin=%s\n", txin.ToString());
            tx.vin.push_back(txin);
        }

        BOOST_FOREACH(const CTxOut& txout, vecTxOut) {
            LogPrint("privatesend", "CDarksendPool::SendDarksendDenominate -- txout=%s\n", txout.ToString());
            tx.vout.push_back(txout);
        }

        LogPrintf("CDarksendPool::SendDarksendDenominate -- Submitting partial tx %s", tx.ToString());

        mempool.PrioritiseTransaction(tx.GetHash(), tx.GetHash().ToString(), 1000, 0.1*COIN);
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain || !AcceptToMemoryPool(mempool, validationState, CTransaction(tx), false, NULL, false, true, true)) {
            LogPrintf("CDarksendPool::SendDarksendDenominate -- AcceptToMemoryPool() failed! tx=%s", tx.ToString());
            UnlockCoins();
            SetNull();
            return;
        }
    }

    // store our entry for later use
    CDarkSendEntry entry;
    entry.Add(vecTxIn, nAmount, txMyCollateral, vecTxOut);
    vecEntries.push_back(entry);

    RelayIn(vecEntries[0].vecTxDSIn, vecEntries[0].nAmount, txMyCollateral, vecEntries[0].vecTxDSOut);
    CheckPool();
}

// Incoming message from Masternode updating the progress of mixing
//    nAcceptedEntriesCountNew:
//        -1 mean's it'n not a "transaction accepted/not accepted" message, just a standard update
//         0 means transaction was not accepted
//         1 means transaction was accepted
bool CDarksendPool::UpdatePoolStateOnClient(int nStateNew, int nEntriesCountNew, int nAcceptedEntriesCountNew, int& nErrorID, int nSessionIDNew)
{
    if(fMasterNode) return false;
    if(nState == POOL_STATE_ERROR || nState == POOL_STATE_SUCCESS) return false;

    SetState(nStateNew);
    nEntriesCount = nEntriesCountNew;

    strAutoDenomResult = _("Masternode:") + " " + GetMessageByID(nErrorID);

    if(nAcceptedEntriesCountNew != -1) {
        fLastEntryAccepted = nAcceptedEntriesCountNew;
        nAcceptedEntriesCount += nAcceptedEntriesCountNew;
        if(nAcceptedEntriesCountNew == 0) {
            SetState(POOL_STATE_ERROR);
            strLastMessage = GetMessageByID(nErrorID);
        }

        if(nAcceptedEntriesCountNew == 1 && nSessionIDNew != 0) {
            nSessionID = nSessionIDNew;
            LogPrintf("CDarksendPool::UpdatePoolStateOnClient -- set nSessionID to %d\n", nSessionID);
            fSessionFoundMasternode = true;
        }
    }

    if(nStateNew == POOL_STATE_ACCEPTING_ENTRIES) {
        if(nAcceptedEntriesCountNew == 1) {
            LogPrintf("CDarksendPool::UpdatePoolStateOnClient -- entry accepted!\n");
            fSessionFoundMasternode = true;
            //wait for other users. Masternode will report when ready
            SetState(POOL_STATE_QUEUE);
        } else if(nAcceptedEntriesCountNew == 0 && nSessionID == 0 && !fSessionFoundMasternode) {
            LogPrintf("CDarksendPool::UpdatePoolStateOnClient -- entry not accepted by Masternode\n");
            UnlockCoins();
            SetState(POOL_STATE_ACCEPTING_ENTRIES);
            DoAutomaticDenominating(); //try another Masternode
        }
        if(fSessionFoundMasternode) return true;
    }

    return true;
}

//
// After we receive the finalized transaction from the Masternode, we must
// check it to make sure it's what we want, then sign it if we agree.
// If we refuse to sign, it's possible we'll be charged collateral
//
bool CDarksendPool::SignFinalTransaction(const CTransaction& finalTransactionNew, CNode* node)
{
    if(fMasterNode) return false;

    finalMutableTransaction = finalTransactionNew;
    LogPrintf("CDarksendPool::SignFinalTransaction -- %s", finalMutableTransaction.ToString());

    std::vector<CTxIn> sigs;

    //make sure my inputs/outputs are present, otherwise refuse to sign
    BOOST_FOREACH(const CDarkSendEntry entry, vecEntries) {
        BOOST_FOREACH(const CTxDSIn txdsin, entry.vecTxDSIn) {
            /* Sign my transaction and all outputs */
            int nMyInputIndex = -1;
            CScript prevPubKey = CScript();
            CTxIn txin = CTxIn();

            for(unsigned int i = 0; i < finalMutableTransaction.vin.size(); i++) {
                if(finalMutableTransaction.vin[i] == txdsin) {
                    nMyInputIndex = i;
                    prevPubKey = txdsin.prevPubKey;
                    txin = txdsin;
                }
            }

            if(nMyInputIndex >= 0) { //might have to do this one input at a time?
                int nFoundOutputsCount = 0;
                CAmount nValue1 = 0;
                CAmount nValue2 = 0;

                for(unsigned int i = 0; i < finalMutableTransaction.vout.size(); i++) {
                    BOOST_FOREACH(const CTxOut& txout, entry.vecTxDSOut) {
                        if(finalMutableTransaction.vout[i] == txout) {
                            nFoundOutputsCount++;
                            nValue1 += finalMutableTransaction.vout[i].nValue;
                        }
                    }
                }

                BOOST_FOREACH(const CTxOut txout, entry.vecTxDSOut)
                    nValue2 += txout.nValue;

                int nTargetOuputsCount = entry.vecTxDSOut.size();
                if(nFoundOutputsCount < nTargetOuputsCount || nValue1 != nValue2) {
                    // in this case, something went wrong and we'll refuse to sign. It's possible we'll be charged collateral. But that's
                    // better then signing if the transaction doesn't look like what we wanted.
                    LogPrintf("CDarksendPool::SignFinalTransaction -- My entries are not correct! Refusing to sign: nFoundOutputsCount: %d, nTargetOuputsCount: %d\n", nFoundOutputsCount, nTargetOuputsCount);
                    UnlockCoins();
                    SetNull();

                    return false;
                }

                const CKeyStore& keystore = *pwalletMain;

                LogPrint("privatesend", "CDarksendPool::SignFinalTransaction -- Signing my input %i\n", nMyInputIndex);
                if(!SignSignature(keystore, prevPubKey, finalMutableTransaction, nMyInputIndex, int(SIGHASH_ALL|SIGHASH_ANYONECANPAY))) { // changes scriptSig
                    LogPrint("privatesend", "CDarksendPool::SignFinalTransaction -- Unable to sign my own transaction!\n");
                    // not sure what to do here, it will timeout...?
                }

                sigs.push_back(finalMutableTransaction.vin[nMyInputIndex]);
                LogPrint("privatesend", "CDarksendPool::SignFinalTransaction -- nMyInputIndex: %d, sigs.size(): %d, scriptSig=%s\n", nMyInputIndex, (int)sigs.size(), ScriptToAsmStr(finalMutableTransaction.vin[nMyInputIndex].scriptSig));
            }

        }

        LogPrint("privatesend", "CDarksendPool::SignFinalTransaction -- finalMutableTransaction=%s", finalMutableTransaction.ToString());
    }

    // push all of our signatures to the Masternode
    if(sigs.size() > 0 && node != NULL)
        node->PushMessage(NetMsgType::DSSIGNFINALTX, sigs);

    return true;
}

void CDarksendPool::NewBlock()
{
    static int64_t nTimeNewBlockReceived = 0;

    //we we're processing lots of blocks, we'll just leave
    if(GetTime() - nTimeNewBlockReceived < 10) return;
    nTimeNewBlockReceived = GetTime();
    LogPrint("privatesend", "CDarksendPool::NewBlock\n");

    CheckTimeout();
}

// mixing transaction was completed (failed or successful)
void CDarksendPool::CompletedTransaction(bool fError, int nErrorID)
{
    if(fMasterNode) return;

    if(fError) {
        LogPrintf("CompletedTransaction -- error\n");
        SetState(POOL_STATE_ERROR);

        CheckPool();
        UnlockCoins();
        SetNull();
    } else {
        LogPrintf("CompletedTransaction -- success\n");
        SetState(POOL_STATE_SUCCESS);

        UnlockCoins();
        SetNull();

        // To avoid race conditions, we'll only let DS run once per block
        nCachedLastSuccessBlock = pCurrentBlockIndex->nHeight;
    }
    strLastMessage = GetMessageByID(nErrorID);
}

//
// Passively run mixing in the background to anonymize funds based on the given configuration.
//
bool CDarksendPool::DoAutomaticDenominating(bool fDryRun)
{
    if(!fEnablePrivateSend || fMasterNode || !pCurrentBlockIndex) return false;
    if(!pwalletMain || pwalletMain->IsLocked()) return false;
    if(nState == POOL_STATE_ERROR || nState == POOL_STATE_SUCCESS) return false;

    switch(nWalletBackups) {
        case 0:
            LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- Automatic backups disabled, no mixing available.\n");
            strAutoDenomResult = _("Automatic backups disabled") + ", " + _("no mixing available.");
            fEnablePrivateSend = false; // stop mixing
            pwalletMain->nKeysLeftSinceAutoBackup = 0; // no backup, no "keys since last backup"
            return false;
        case -1:
            // Automatic backup failed, nothing else we can do until user fixes the issue manually.
            // There is no way to bring user attention in daemon mode so we just update status and
            // keep spaming if debug is on.
            LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- ERROR! Failed to create automatic backup.\n");
            strAutoDenomResult = _("ERROR! Failed to create automatic backup") + ", " + _("see debug.log for details.");
            return false;
        case -2:
            // We were able to create automatic backup but keypool was not replenished because wallet is locked.
            // There is no way to bring user attention in daemon mode so we just update status and
            // keep spaming if debug is on.
            LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- WARNING! Failed to create replenish keypool, please unlock your wallet to do so.\n");
            strAutoDenomResult = _("WARNING! Failed to replenish keypool, please unlock your wallet to do so.") + ", " + _("see debug.log for details.");
            return false;
    }

    if(pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_STOP) {
        // We should never get here via mixing itself but probably smth else is still actively using keypool
        LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- Very low number of keys left: %d, no mixing available.\n", pwalletMain->nKeysLeftSinceAutoBackup);
        strAutoDenomResult = strprintf(_("Very low number of keys left: %d") + ", " + _("no mixing available."), pwalletMain->nKeysLeftSinceAutoBackup);
        // It's getting really dangerous, stop mixing
        fEnablePrivateSend = false;
        return false;
    } else if(pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING) {
        // Low number of keys left but it's still more or less safe to continue
        LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- Very low number of keys left: %d\n", pwalletMain->nKeysLeftSinceAutoBackup);
        strAutoDenomResult = strprintf(_("Very low number of keys left: %d"), pwalletMain->nKeysLeftSinceAutoBackup);

        if(fCreateAutoBackups) {
            LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- Trying to create new backup.\n");
            std::string warningString;
            std::string errorString;

            if(!AutoBackupWallet(pwalletMain, "", warningString, errorString)) {
                if(!warningString.empty()) {
                    // There were some issues saving backup but yet more or less safe to continue
                    LogPrintf("CDarksendPool::DoAutomaticDenominating -- WARNING! Something went wrong on automatic backup: %s\n", warningString);
                }
                if(!errorString.empty()) {
                    // Things are really broken
                    LogPrintf("CDarksendPool::DoAutomaticDenominating -- ERROR! Failed to create automatic backup: %s\n", errorString);
                    strAutoDenomResult = strprintf(_("ERROR! Failed to create automatic backup") + ": %s", errorString);
                    return false;
                }
            }
        } else {
            // Wait for someone else (e.g. GUI action) to create automatic backup for us
            return false;
        }
    }

    LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- Keys left since latest backup: %d\n", pwalletMain->nKeysLeftSinceAutoBackup);

    if(GetEntriesCount() > 0) {
        strAutoDenomResult = _("Mixing in progress...");
        return false;
    }

    TRY_LOCK(cs_darksend, lockDS);
    if(!lockDS) {
        strAutoDenomResult = _("Lock is already in place.");
        return false;
    }

    if(!masternodeSync.IsBlockchainSynced()) {
        strAutoDenomResult = _("Can't mix while sync in progress.");
        return false;
    }

    if(!fDryRun && pwalletMain->IsLocked()) {
        strAutoDenomResult = _("Wallet is locked.");
        return false;
    }

    if(!fPrivateSendMultiSession && pCurrentBlockIndex->nHeight - nCachedLastSuccessBlock < nMinBlockSpacing) {
        LogPrintf("CDarksendPool::DoAutomaticDenominating -- Last successful PrivateSend action was too recent\n");
        strAutoDenomResult = _("Last successful PrivateSend action was too recent.");
        return false;
    }

    if(mnodeman.size() == 0) {
        LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- No Masternodes detected\n");
        strAutoDenomResult = _("No Masternodes detected.");
        return false;
    }

    // ** find the coins we'll use
    std::vector<CTxIn> vecTxIn;
    CAmount nValueMin = CENT;
    CAmount nValueIn = 0;

    CAmount nOnlyDenominatedBalance;
    CAmount nBalanceNeedsDenominated;

    CAmount nLowestDenom = vecPrivateSendDenominations.back();
    // if there are no confirmed DS collateral inputs yet
    if(!pwalletMain->HasCollateralInputs())
        // should have some additional amount for them
        nLowestDenom += DARKSEND_COLLATERAL*4;

    CAmount nBalanceNeedsAnonymized = pwalletMain->GetNeedsToBeAnonymizedBalance(nLowestDenom);

    // anonymizable balance is way too small
    if(nBalanceNeedsAnonymized < nLowestDenom) {
        LogPrintf("CDarksendPool::DoAutomaticDenominating -- Not enough funds to anonymize\n");
        strAutoDenomResult = _("Not enough funds to anonymize.");
        return false;
    }

    LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- nLowestDenom: %f, nBalanceNeedsAnonymized: %f\n", (float)nLowestDenom/COIN, (float)nBalanceNeedsAnonymized/COIN);

    // select coins that should be given to the pool
    if(!pwalletMain->SelectCoinsDark(nValueMin, nBalanceNeedsAnonymized, vecTxIn, nValueIn, 0, nPrivateSendRounds))
    {
        if(pwalletMain->SelectCoinsDark(nValueMin, 9999999*COIN, vecTxIn, nValueIn, -2, 0))
        {
            nOnlyDenominatedBalance = pwalletMain->GetDenominatedBalance(true) + pwalletMain->GetDenominatedBalance() - pwalletMain->GetAnonymizedBalance();
            nBalanceNeedsDenominated = nBalanceNeedsAnonymized - nOnlyDenominatedBalance;

            if(nBalanceNeedsDenominated > nValueIn) nBalanceNeedsDenominated = nValueIn;

            LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- `SelectCoinsDark` (%f - (%f + %f - %f = %f) ) = %f\n",
                            (float)nBalanceNeedsAnonymized/COIN,
                            (float)pwalletMain->GetDenominatedBalance(true)/COIN,
                            (float)pwalletMain->GetDenominatedBalance()/COIN,
                            (float)pwalletMain->GetAnonymizedBalance()/COIN,
                            (float)nOnlyDenominatedBalance/COIN,
                            (float)nBalanceNeedsDenominated/COIN);

            if(nBalanceNeedsDenominated < nLowestDenom) { // most likely we are just waiting for denoms to confirm
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- No funds detected in need of denominating\n");
                strAutoDenomResult = _("No funds detected in need of denominating.");
                return false;
            }
            if(!fDryRun) return CreateDenominated();

            return true;
        } else {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- Can't denominate (no compatible inputs left)\n");
            strAutoDenomResult = _("Can't denominate: no compatible inputs left.");
            return false;
        }
    }

    if(fDryRun) return true;

    nOnlyDenominatedBalance = pwalletMain->GetDenominatedBalance(true) + pwalletMain->GetDenominatedBalance() - pwalletMain->GetAnonymizedBalance();
    nBalanceNeedsDenominated = nBalanceNeedsAnonymized - nOnlyDenominatedBalance;
    LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating -- 'nBalanceNeedsDenominated > 0' (%f - (%f + %f - %f = %f) ) = %f\n",
                    (float)nBalanceNeedsAnonymized/COIN,
                    (float)pwalletMain->GetDenominatedBalance(true)/COIN,
                    (float)pwalletMain->GetDenominatedBalance()/COIN,
                    (float)pwalletMain->GetAnonymizedBalance()/COIN,
                    (float)nOnlyDenominatedBalance/COIN,
                    (float)nBalanceNeedsDenominated/COIN);

    //check if we have should create more denominated inputs
    if(nBalanceNeedsDenominated > 0) return CreateDenominated();

    //check if we have the collateral sized inputs
    if(!pwalletMain->HasCollateralInputs())
        return !pwalletMain->HasCollateralInputs(false) && MakeCollateralAmounts();

    if(fSessionFoundMasternode) {
        strAutoDenomResult = _("Mixing in progress...");
        return false;
    }

    // Initial phase, find a Masternode
    // Clean if there is anything left from previous session
    UnlockCoins();
    SetNull();

    SetState(POOL_STATE_ACCEPTING_ENTRIES);

    if(!fPrivateSendMultiSession && pwalletMain->GetDenominatedBalance(true) > 0) { //get denominated unconfirmed inputs
        LogPrintf("CDarksendPool::DoAutomaticDenominating -- Found unconfirmed denominated outputs, will wait till they confirm to continue.\n");
        strAutoDenomResult = _("Found unconfirmed denominated outputs, will wait till they confirm to continue.");
        return false;
    }

    //check our collateral and create new if needed
    std::string strReason;
    if(txMyCollateral == CMutableTransaction()) {
        if(!pwalletMain->CreateCollateralTransaction(txMyCollateral, strReason)) {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- create collateral error:%s\n", strReason);
            return false;
        }
    } else {
        if(!IsCollateralValid(txMyCollateral)) {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- invalid collateral, recreating...\n");
            if(!pwalletMain->CreateCollateralTransaction(txMyCollateral, strReason)) {
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- create collateral error: %s\n", strReason);
                return false;
            }
        }
    }

    //if we've used 90% of the Masternode list then drop all the oldest first
    int nThreshold = (int)(mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION) * 0.9);
    LogPrint("privatesend", "Checking vecMasternodesUsed: size: %d, threshold: %d\n", (int)vecMasternodesUsed.size(), nThreshold);
    while((int)vecMasternodesUsed.size() > nThreshold) {
        vecMasternodesUsed.erase(vecMasternodesUsed.begin());
        LogPrint("privatesend", "  vecMasternodesUsed: size: %d, threshold: %d\n", (int)vecMasternodesUsed.size(), nThreshold);
    }

    bool fUseQueue = insecure_rand()%100 > 33;
    // don't use the queues all of the time for mixing unless we are a liquidity provider
    if(nLiquidityProvider || fUseQueue) {

        // Look through the queues and see if anything matches
        BOOST_FOREACH(CDarksendQueue& dsq, vecDarksendQueue) {
            if(dsq.nTime == 0) continue;
            if(dsq.IsExpired()) continue;

            CService addr;
            if(!dsq.GetAddress(addr)) continue;

            int nProtocolVersion;
            if(!dsq.GetProtocolVersion(nProtocolVersion)) continue;
            if(nProtocolVersion < MIN_PRIVATESEND_PEER_PROTO_VERSION) continue;

            // incompatible denom
            if(dsq.nDenom >= (1 << vecPrivateSendDenominations.size())) continue;

            bool fUsed = false;
            //don't reuse Masternodes
            BOOST_FOREACH(CTxIn txinUsed, vecMasternodesUsed) {
                if(dsq.vin == txinUsed) {
                    fUsed = true;
                    break;
                }
            }
            if(fUsed) continue;

            std::vector<CTxIn> vecTxInTmp;
            std::vector<COutput> vCoinsTmp;
            // Try to match their denominations if possible
            if(!pwalletMain->SelectCoinsByDenominations(dsq.nDenom, nValueMin, nBalanceNeedsAnonymized, vecTxInTmp, vCoinsTmp, nValueIn, 0, nPrivateSendRounds)) {
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- Couldn't match denominations %d\n", dsq.nDenom);
                continue;
            }

            CMasternode* pmn = mnodeman.Find(dsq.vin);
            if(pmn == NULL) {
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- dsq masternode is not in masternode list! vin=%s\n", dsq.vin.ToString());
                continue;
            }

            LogPrintf("CDarksendPool::DoAutomaticDenominating -- attempt to connect to masternode from queue, addr=%s\n", pmn->addr.ToString());
            nLastTimeChanged = GetTimeMillis();
            // connect to Masternode and submit the queue request
            CNode* pnode = ConnectNode((CAddress)addr, NULL, true);
            if(pnode != NULL) {
                pSubmittedToMasternode = pmn;
                vecMasternodesUsed.push_back(dsq.vin);
                nSessionDenom = dsq.nDenom;

                pnode->PushMessage(NetMsgType::DSACCEPT, nSessionDenom, txMyCollateral);
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- connected (from queue), sending dsa: nSessionDenom: %d, addr=%s\n", nSessionDenom, pnode->addr.ToString());
                strAutoDenomResult = _("Mixing in progress...");
                dsq.nTime = 0; //remove node
                return true;
            } else {
                LogPrintf("CDarksendPool::DoAutomaticDenominating -- error connecting\n");
                strAutoDenomResult = _("Error connecting to Masternode.");
                dsq.nTime = 0; //remove node
                continue;
            }
        }
    }

    // do not initiate queue if we are a liquidity provider to avoid useless inter-mixing
    if(nLiquidityProvider) return false;

    int nTries = 0;

    // otherwise, try one randomly
    while(nTries < 10) {
        CMasternode* pmn = mnodeman.FindRandomNotInVec(vecMasternodesUsed, MIN_PRIVATESEND_PEER_PROTO_VERSION);
        if(pmn == NULL) {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- Can't find random masternode!\n");
            strAutoDenomResult = _("Can't find random Masternode.");
            return false;
        }

        if(pmn->nLastDsq != 0 && pmn->nLastDsq + mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount) {
            nTries++;
            continue;
        }

        nLastTimeChanged = GetTimeMillis();
        LogPrintf("CDarksendPool::DoAutomaticDenominating -- attempt %d connection to Masternode %s\n", nTries, pmn->addr.ToString());
        CNode* pnode = ConnectNode((CAddress)pmn->addr, NULL, true);
        if(pnode != NULL) {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- connected %s\n", pmn->vin.ToString());
            pSubmittedToMasternode = pmn;
            vecMasternodesUsed.push_back(pmn->vin);

            std::vector<CAmount> vecAmounts;
            pwalletMain->ConvertList(vecTxIn, vecAmounts);
            // try to get a single random denom out of vecAmounts
            while(nSessionDenom == 0)
                nSessionDenom = GetDenominationsByAmounts(vecAmounts);

            pnode->PushMessage(NetMsgType::DSACCEPT, nSessionDenom, txMyCollateral);
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- connected, sending DSACCEPT, nSessionDenom: %d\n", nSessionDenom);
            strAutoDenomResult = _("Mixing in progress...");
            return true;
        } else {
            LogPrintf("CDarksendPool::DoAutomaticDenominating -- can't connect %s\n", pmn->vin.ToString());
            vecMasternodesUsed.push_back(pmn->vin); // postpone MN we wasn't able to connect to
            nTries++;
            continue;
        }
    }

    strAutoDenomResult = _("No compatible Masternode found.");
    return false;
}

bool CDarksendPool::PrepareDenominate()
{
    std::string strError;

    // Submit transaction to the pool if we get here
    // Try to use only inputs with the same number of rounds starting from lowest number of rounds possible
    for(int i = 0; i < nPrivateSendRounds; i++) {
        strError = pwalletMain->PrepareDarksendDenominate(i, i+1);
        if(strError == "") {
            LogPrintf("CDarksendPool::PrepareDenominate -- Running PrivateSend denominate for %d rounds, success\n", i);
            return true;
        }
        LogPrintf("CDarksendPool::PrepareDenominate -- Running PrivateSend denominate for %d rounds. Return '%s'\n", i, strError);
    }

    // We failed? That's strange but let's just make final attempt and try to mix everything
    strError = pwalletMain->PrepareDarksendDenominate(0, nPrivateSendRounds);
    if(strError == "") {
        LogPrintf("CDarksendPool::PrepareDenominate -- Running PrivateSend denominate for all rounds, success\n");
        return true;
    }
    LogPrintf("CDarksendPool::PrepareDenominate -- Running PrivateSend denominate for all rounds. Return '%s'\n", strError);

    // Should never actually get here but just in case
    strAutoDenomResult = strError;
    LogPrintf("CDarksendPool::PrepareDenominate -- Error running denominate, %s\n", strError);
    return false;
}

// Create collaterals by looping through inputs grouped by addresses
bool CDarksendPool::MakeCollateralAmounts()
{
    std::vector<CompactTallyItem> vecTally;
    if(!pwalletMain->SelectCoinsGrouppedByAddresses(vecTally, false)) {
        LogPrint("privatesend", "CDarksendPool::MakeCollateralAmounts -- SelectCoinsGrouppedByAddresses can't find any inputs!\n");
        return false;
    }

    BOOST_FOREACH(CompactTallyItem& item, vecTally) {
        if(!MakeCollateralAmounts(item)) continue;
        return true;
    }

    LogPrintf("CDarksendPool::MakeCollateralAmounts -- failed!\n");
    return false;
}

// Split up large inputs or create fee sized inputs
bool CDarksendPool::MakeCollateralAmounts(const CompactTallyItem& tallyItem)
{
    CWalletTx wtx;
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    std::string strFail = "";
    std::vector<CRecipient> vecSend;

    // make our collateral address
    CReserveKey reservekeyCollateral(pwalletMain);
    // make our change address
    CReserveKey reservekeyChange(pwalletMain);

    CScript scriptCollateral;
    CPubKey vchPubKey;
    assert(reservekeyCollateral.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked
    scriptCollateral = GetScriptForDestination(vchPubKey.GetID());

    vecSend.push_back((CRecipient){scriptCollateral, DARKSEND_COLLATERAL*4, false});

    // try to use non-denominated and not mn-like funds first, select them explicitly
    CCoinControl coinControl;
    coinControl.fAllowOtherInputs = false;
    coinControl.fAllowWatchOnly = false;
    // send change to the same address so that we were able create more denoms out of it later
    coinControl.destChange = tallyItem.address.Get();
    BOOST_FOREACH(const CTxIn& txin, tallyItem.vecTxIn)
        coinControl.Select(txin.prevout);

    bool fSuccess = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
            nFeeRet, nChangePosRet, strFail, &coinControl, true, ONLY_NONDENOMINATED_NOT1000IFMN);
    if(!fSuccess) {
        // if we failed (most likeky not enough funds), try to use all coins instead -
        // MN-like funds should not be touched in any case and we can't mix denominated without collaterals anyway
        LogPrintf("CDarksendPool::MakeCollateralAmounts -- ONLY_NONDENOMINATED_NOT1000IFMN Error: %s\n", strFail);
        CCoinControl *coinControlNull = NULL;
        fSuccess = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
                nFeeRet, nChangePosRet, strFail, coinControlNull, true, ONLY_NOT1000IFMN);
        if(!fSuccess) {
            LogPrintf("CDarksendPool::MakeCollateralAmounts -- ONLY_NOT1000IFMN Error: %s\n", strFail);
            reservekeyCollateral.ReturnKey();
            return false;
        }
    }

    reservekeyCollateral.KeepKey();

    LogPrintf("CDarksendPool::MakeCollateralAmounts -- txid=%s\n", wtx.GetHash().GetHex());

    // use the same nCachedLastSuccessBlock as for DS mixinx to prevent race
    if(!pwalletMain->CommitTransaction(wtx, reservekeyChange)) {
        LogPrintf("CDarksendPool::MakeCollateralAmounts -- CommitTransaction failed!\n");
        return false;
    }

    nCachedLastSuccessBlock = pCurrentBlockIndex->nHeight;

    return true;
}

// Create denominations by looping through inputs grouped by addresses
bool CDarksendPool::CreateDenominated()
{
    std::vector<CompactTallyItem> vecTally;
    if(!pwalletMain->SelectCoinsGrouppedByAddresses(vecTally)) {
        LogPrint("privatesend", "CDarksendPool::CreateDenominated -- SelectCoinsGrouppedByAddresses can't find any inputs!\n");
        return false;
    }

    BOOST_FOREACH(CompactTallyItem& item, vecTally) {
        if(!CreateDenominated(item)) continue;
        return true;
    }

    LogPrintf("CDarksendPool::CreateDenominated -- failed!\n");
    return false;
}

// Create denominations
bool CDarksendPool::CreateDenominated(const CompactTallyItem& tallyItem)
{
    std::vector<CRecipient> vecSend;
    CAmount nValueLeft = tallyItem.nAmount;
    nValueLeft -= DARKSEND_COLLATERAL; // leave some room for fees

    LogPrintf("CreateDenominated0 nValueLeft: %f\n", (float)nValueLeft/COIN);
    // make our collateral address
    CReserveKey reservekeyCollateral(pwalletMain);

    CScript scriptCollateral;
    CPubKey vchPubKey;
    assert(reservekeyCollateral.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked
    scriptCollateral = GetScriptForDestination(vchPubKey.GetID());

    // ****** Add collateral outputs ************ /

    if(!pwalletMain->HasCollateralInputs()) {
        vecSend.push_back((CRecipient){scriptCollateral, DARKSEND_COLLATERAL*4, false});
        nValueLeft -= DARKSEND_COLLATERAL*4;
    }

    // ****** Add denoms ************ /

    // make our denom addresses
    CReserveKey reservekeyDenom(pwalletMain);

    // try few times - skipping smallest denoms first if there are too much already, if failed - use them
    int nOutputsTotal = 0;
    bool fSkip = true;
    do {

        BOOST_REVERSE_FOREACH(CAmount nDenomValue, vecPrivateSendDenominations) {

            if(fSkip) {
                // Note: denoms are skipped if there are already DENOMS_COUNT_MAX of them
                // and there are still larger denoms which can be used for mixing

                // check skipped denoms
                if(IsDenomSkipped(nDenomValue)) continue;

                // find new denoms to skip if any (ignore the largest one)
                if(nDenomValue != vecPrivateSendDenominations[0] && pwalletMain->CountInputsWithAmount(nDenomValue) > DENOMS_COUNT_MAX) {
                    strAutoDenomResult = strprintf(_("Too many %f denominations, removing."), (float)nDenomValue/COIN);
                    LogPrintf("CDarksendPool::CreateDenominated -- %s\n", strAutoDenomResult);
                    vecDenominationsSkipped.push_back(nDenomValue);
                    continue;
                }
            }

            int nOutputs = 0;

            // add each output up to 10 times until it can't be added again
            while(nValueLeft - nDenomValue >= 0 && nOutputs <= 10) {
                CScript scriptDenom;
                CPubKey vchPubKey;
                //use a unique change address
                assert(reservekeyDenom.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked
                scriptDenom = GetScriptForDestination(vchPubKey.GetID());
                // TODO: do not keep reservekeyDenom here
                reservekeyDenom.KeepKey();

                vecSend.push_back((CRecipient){ scriptDenom, nDenomValue, false });

                //increment outputs and subtract denomination amount
                nOutputs++;
                nValueLeft -= nDenomValue;
                LogPrintf("CreateDenominated1: nOutputsTotal: %d, nOutputs: %d, nValueLeft: %f\n", nOutputsTotal, nOutputs, (float)nValueLeft/COIN);
            }

            nOutputsTotal += nOutputs;
            if(nValueLeft == 0) break;
        }
        LogPrintf("CreateDenominated2: nOutputsTotal: %d, nValueLeft: %f\n", nOutputsTotal, (float)nValueLeft/COIN);
        // if there were no outputs added, start over without skipping
        fSkip = !fSkip;
    } while (nOutputsTotal == 0 && !fSkip);
    LogPrintf("CreateDenominated3: nOutputsTotal: %d, nValueLeft: %f\n", nOutputsTotal, (float)nValueLeft/COIN);

    // if we have anything left over, it will be automatically send back as change - there is no need to send it manually

    CCoinControl coinControl;
    coinControl.fAllowOtherInputs = false;
    coinControl.fAllowWatchOnly = false;
    // send change to the same address so that we were able create more denoms out of it later
    coinControl.destChange = tallyItem.address.Get();
    BOOST_FOREACH(const CTxIn& txin, tallyItem.vecTxIn)
        coinControl.Select(txin.prevout);

    CWalletTx wtx;
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    std::string strFail = "";
    // make our change address
    CReserveKey reservekeyChange(pwalletMain);

    bool fSuccess = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
            nFeeRet, nChangePosRet, strFail, &coinControl, true, ONLY_NONDENOMINATED_NOT1000IFMN);
    if(!fSuccess) {
        LogPrintf("CDarksendPool::CreateDenominated -- Error: %s\n", strFail);
        // TODO: return reservekeyDenom here
        reservekeyCollateral.ReturnKey();
        return false;
    }

    // TODO: keep reservekeyDenom here
    reservekeyCollateral.KeepKey();

    if(!pwalletMain->CommitTransaction(wtx, reservekeyChange)) {
        LogPrintf("CDarksendPool::CreateDenominated -- CommitTransaction failed!\n");
        return false;
    }

    // use the same nCachedLastSuccessBlock as for DS mixing to prevent race
    nCachedLastSuccessBlock = pCurrentBlockIndex->nHeight;
    LogPrintf("CDarksendPool::CreateDenominated -- txid=%s\n", wtx.GetHash().GetHex());

    return true;
}

bool CDarksendPool::IsOutputsCompatibleWithSessionDenom(const std::vector<CTxOut>& vecTxOut)
{
    if(GetDenominations(vecTxOut) == 0) return false;

    BOOST_FOREACH(const CDarkSendEntry entry, vecEntries) {
        LogPrintf("CDarksendPool::IsOutputsCompatibleWithSessionDenom -- vecTxOut denom %d, entry.vecTxDSOut denom %d\n", GetDenominations(vecTxOut), GetDenominations(entry.vecTxDSOut));
        if(GetDenominations(vecTxOut) != GetDenominations(entry.vecTxDSOut)) return false;
    }

    return true;
}

bool CDarksendPool::IsDenomCompatibleWithSession(int nDenom, CTransaction txCollateral, int& nErrorID)
{
    if(nDenom == 0) return false;

    LogPrintf("CDarksendPool::IsDenomCompatibleWithSession -- nSessionDenom: %d nSessionUsers: %d\n", nSessionDenom, nSessionUsers);

    if(!fUnitTest && !IsCollateralValid(txCollateral)) {
        LogPrint("privatesend", "CDarksendPool::IsDenomCompatibleWithSession -- collateral not valid!\n");
        nErrorID = ERR_INVALID_COLLATERAL;
        return false;
    }

    if(nSessionUsers < 0) nSessionUsers = 0;

    if(nSessionUsers == 0) {
        nSessionID = 1 + GetRand(999999);
        nSessionDenom = nDenom;
        nSessionUsers++;
        nLastTimeChanged = GetTimeMillis();

        if(!fUnitTest) {
            //broadcast that I'm accepting entries, only if it's the first entry through
            CDarksendQueue dsq(nDenom, activeMasternode.vin, GetTime(), false);
            dsq.Sign();
            dsq.Relay();
        }

        SetState(POOL_STATE_QUEUE);
        vecSessionCollateral.push_back(txCollateral);
        return true;
    }

    if((nState != POOL_STATE_ACCEPTING_ENTRIES && nState != POOL_STATE_QUEUE) || nSessionUsers >= GetMaxPoolTransactions()) {
        if((nState != POOL_STATE_ACCEPTING_ENTRIES && nState != POOL_STATE_QUEUE)) nErrorID = ERR_MODE;
        if(nSessionUsers >= GetMaxPoolTransactions()) nErrorID = ERR_QUEUE_FULL;
        LogPrintf("CDarksendPool::IsDenomCompatibleWithSession -- incompatible mode, return false: nState status %d, nSessionUsers status %d\n", nState != POOL_STATE_ACCEPTING_ENTRIES, nSessionUsers >= GetMaxPoolTransactions());
        return false;
    }

    if(nDenom != nSessionDenom) {
        nErrorID = ERR_DENOM;
        return false;
    }

    LogPrintf("CDarksendPool::IsDenomCompatibleWithSession -- compatible\n");

    nSessionUsers++;
    nLastTimeChanged = GetTimeMillis();
    vecSessionCollateral.push_back(txCollateral);

    return true;
}

/*  Create a nice string to show the denominations
    Function returns as follows:
        ( bit on if present )
        bit 0           - 100
        bit 1           - 10
        bit 2           - 1
        bit 3           - .1
        none of above   - non-denom
        bit 4 and so on - non-denom
*/
std::string CDarksendPool::GetDenominationsToString(int nDenom)
{
    std::string strDenom;

    if(nDenom & (1 << 0)) strDenom += "100";

    if(nDenom & (1 << 1)) {
        if(strDenom.size() > 0) strDenom += "+";
        strDenom += "10";
    }

    if(nDenom & (1 << 2)) {
        if(strDenom.size() > 0) strDenom += "+";
        strDenom += "1";
    }

    if(nDenom & (1 << 3)) {
        if(strDenom.size() > 0) strDenom += "+";
        strDenom += "0.1";
    }

    if(strDenom.size() == 0 && nDenom >= (1 << 4)) strDenom += "non-denom";

    return strDenom;
}

int CDarksendPool::GetDenominations(const std::vector<CTxDSOut>& vecTxDSOut)
{
    std::vector<CTxOut> vecTxOut;

    BOOST_FOREACH(CTxDSOut out, vecTxDSOut)
        vecTxOut.push_back(out);

    return GetDenominations(vecTxOut);
}

/*  Return a bitshifted integer representing the denominations in this list
    Function returns as follows:
        ( bit on if present )
        100       - bit 0
        10        - bit 1
        1         - bit 2
        .1        - bit 3
        non-denom - 0, all bits off
*/
int CDarksendPool::GetDenominations(const std::vector<CTxOut>& vecTxOut, bool fSingleRandomDenom)
{
    std::vector<std::pair<CAmount, int> > vecDenomUsed;

    // make a list of denominations, with zero uses
    BOOST_FOREACH(CAmount nDenomValue, vecPrivateSendDenominations)
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
        int bit = (fSingleRandomDenom ? insecure_rand()%2 : 1) & s.second;
        nDenom |= bit << c++;
        if(fSingleRandomDenom && bit) break; // use just one random denomination
    }

    return nDenom;
}

int CDarksendPool::GetDenominationsByAmounts(const std::vector<CAmount>& vecAmount)
{
    CScript scriptTmp = CScript();
    std::vector<CTxOut> vecTxOut;

    BOOST_REVERSE_FOREACH(CAmount nAmount, vecAmount) {
        CTxOut txout(nAmount, scriptTmp);
        vecTxOut.push_back(txout);
    }

    return GetDenominations(vecTxOut, true);
}

std::string CDarksendPool::GetMessageByID(int nMessageID)
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
        case ERR_MAXIMUM:               return _("Value more than PrivateSend pool maximum allows.");
        case ERR_MN_LIST:               return _("Not in the Masternode list.");
        case ERR_MODE:                  return _("Incompatible mode.");
        case ERR_NON_STANDARD_PUBKEY:   return _("Non-standard public key detected.");
        case ERR_NOT_A_MN:              return _("This is not a Masternode.");
        case ERR_QUEUE_FULL:            return _("Masternode queue is full.");
        case ERR_RECENT:                return _("Last PrivateSend was too recent.");
        case ERR_SESSION:               return _("Session not complete!");
        case ERR_MISSING_TX:            return _("Missing input transaction information.");
        case ERR_VERSION:               return _("Incompatible version.");
        case MSG_SUCCESS:               return _("Transaction created successfully.");
        case MSG_ENTRIES_ADDED:         return _("Your entries added successfully.");
        case MSG_NOERR:                 return _("No errors detected.");
        default:                        return _("Unknown response.");
    }
}

bool CDarkSendSigner::IsVinAssociatedWithPubkey(const CTxIn& txin, const CPubKey& pubkey)
{
    CScript payee;
    payee = GetScriptForDestination(pubkey.GetID());

    CTransaction tx;
    uint256 hash;
    if(GetTransaction(txin.prevout.hash, tx, Params().GetConsensus(), hash, true)) {
        BOOST_FOREACH(CTxOut out, tx.vout)
            if(out.nValue == 1000*COIN && out.scriptPubKey == payee) return true;
    }

    return false;
}

bool CDarkSendSigner::GetKeysFromSecret(std::string strSecret, std::string& strErrorRet, CKey& keyRet, CPubKey& pubkeyRet)
{
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if(!fGood) {
        strErrorRet = _("Invalid secret.");
        return false;
    }

    keyRet = vchSecret.GetKey();
    pubkeyRet = keyRet.GetPubKey();

    return true;
}

bool CDarkSendSigner::SignMessage(std::string strMessage, std::string& strErrorRet, std::vector<unsigned char>& vchSigRet, CKey key)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    if(!key.SignCompact(ss.GetHash(), vchSigRet)) {
        strErrorRet = _("Signing failed.");
        return false;
    }

    return true;
}

bool CDarkSendSigner::VerifyMessage(CPubKey pubkey, const std::vector<unsigned char>& vchSig, std::string strMessage, std::string& strErrorRet)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkeyFromSig;
    if(!pubkeyFromSig.RecoverCompact(ss.GetHash(), vchSig)) {
        strErrorRet = _("Error recovering public key.");
        return false;
    }

    if(pubkeyFromSig.GetID() != pubkey.GetID()) {
        strErrorRet = strprintf("keys don't match: pubkey=%s, pubkeyFromSig=%s, strMessage=%s, vchSig=%s",
                    pubkey.GetID().ToString(), pubkeyFromSig.GetID().ToString(), strMessage,
                    EncodeBase64(&vchSig[0], vchSig.size()));
        return false;
    }

    return true;
}

bool CDarkSendEntry::Add(const std::vector<CTxIn> vecTxIn, CAmount nAmount, const CTransaction txCollateral, const std::vector<CTxOut> vecTxOut)
{
    if(isSet) return false;

    BOOST_FOREACH(const CTxIn& txin, vecTxIn)
        vecTxDSIn.push_back(txin);

    BOOST_FOREACH(const CTxOut& txout, vecTxOut)
        vecTxDSOut.push_back(txout);

    this->nAmount = nAmount;
    this->txCollateral = txCollateral;

    isSet = true;
    nTimeAdded = GetTime();

    return true;
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
    std::string strError = "";

    if(!darkSendSigner.SignMessage(strMessage, strError, vchSig, activeMasternode.keyMasternode)) {
        LogPrintf("CDarksendQueue::Sign -- SignMessage() failed\n");
        return false;
    }

    return CheckSignature();
}

bool CDarksendQueue::CheckSignature()
{
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL) return false;

    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nDenom) + boost::lexical_cast<std::string>(nTime) + boost::lexical_cast<std::string>(fReady);
    std::string strError = "";

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, strError)) {
        LogPrintf("CDarksendQueue::CheckSignature -- Got bad Masternode queue signature, vin=%s\n", vin.ToString());
        return false;
    }

    return true;
}

bool CDarksendQueue::GetAddress(CService &addrRet)
{
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL) return false;

    addrRet = pmn->addr;
    return true;
}

bool CDarksendQueue::GetProtocolVersion(int &nProtocolVersionRet)
{
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL) return false;

    nProtocolVersionRet = pmn->protocolVersion;
    return true;
}

bool CDarksendQueue::Relay()
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSQUEUE, (*this));

    return true;
}

bool CDarksendBroadcastTx::Sign()
{
    if(!fMasterNode) return false;

    std::string strMessage = tx.GetHash().ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if(!darkSendSigner.SignMessage(strMessage, strError, vchSig, activeMasternode.keyMasternode)) {
        LogPrintf("CDarksendBroadcastTx::Sign -- SignMessage() failed\n");
        return false;
    }

    return CheckSignature();
}

bool CDarksendBroadcastTx::CheckSignature()
{
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL) return false;

    std::string strMessage = tx.GetHash().ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, strError)) {
        LogPrintf("CDarksendBroadcastTx::CheckSignature -- Got bad dstx signature\n");
        return false;
    }

    return true;
}

void CDarksendPool::RelayFinalTransaction(const CTransaction& txFinal)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSFINALTX, nSessionID, txFinal);
}

void CDarksendPool::RelayIn(const std::vector<CTxDSIn>& vecTxDSIn, const CAmount& nAmount, const CTransaction& txCollateral, const std::vector<CTxDSOut>& vecTxDSOut)
{
    if(!pSubmittedToMasternode) return;

    std::vector<CTxIn> vecTxIn;
    std::vector<CTxOut> vecTxOut;

    BOOST_FOREACH(CTxDSIn in, vecTxDSIn)
        vecTxIn.push_back(in);

    BOOST_FOREACH(CTxDSOut out, vecTxDSOut)
        vecTxOut.push_back(out);

    CNode* pnode = FindNode(pSubmittedToMasternode->addr);
    if(pnode != NULL) {
        LogPrintf("CDarksendPool::RelayIn -- found master, relaying message to %s\n", pnode->addr.ToString());
        pnode->PushMessage(NetMsgType::DSVIN, vecTxIn, nAmount, txCollateral, vecTxOut);
    }
}

void CDarksendPool::RelayStatus(int nErrorID)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSSTATUSUPDATE, nSessionID, nState, nEntriesCount, nAcceptedEntriesCount, nErrorID);
}

void CDarksendPool::RelayCompletedTransaction(bool fError, int nErrorID)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSCOMPLETE, nSessionID, fError, nErrorID);
}

void CDarksendPool::SetState(unsigned int nStateNew)
{
    if(fMasterNode && (nStateNew == POOL_STATE_ERROR || nStateNew == POOL_STATE_SUCCESS)) {
        LogPrint("privatesend", "CDarksendPool::SetState -- Can't set state to ERROR or SUCCESS as a Masternode. \n");
        return;
    }

    LogPrintf("CDarksendPool::SetState -- nState: %d, nStateNew: %d\n", nState, nStateNew);
    if(nState != nStateNew){
        nLastTimeChanged = GetTimeMillis();
        if(fMasterNode) {
            RelayStatus(MASTERNODE_RESET);
        }
    }
    nState = nStateNew;
}

void CDarksendPool::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("privatesend", "CDarksendPool::UpdatedBlockTip -- pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

//TODO: Rename/move to core
void ThreadCheckDarkSendPool()
{
    if(fLiteMode) return; // disable all Dash specific functionality

    static bool fOneThread;
    if(fOneThread) return;
    fOneThread = true;

    // Make this thread recognisable as the PrivateSend thread
    RenameThread("dash-privatesend");

    unsigned int nTick = 0;
    unsigned int nDoAutoNextRun = nTick + DARKSEND_AUTO_TIMEOUT_MIN;

    while (true)
    {
        MilliSleep(1000);

        // try to sync from all available nodes, one step at a time
        masternodeSync.Process();

        if(masternodeSync.IsBlockchainSynced() && !ShutdownRequested()) {

            nTick++;

            // check if we should activate or ping every few minutes,
            // start right after sync is considered to be done
            if(nTick % MASTERNODE_MIN_MNP_SECONDS == 1)
                activeMasternode.ManageState();

            if(nTick % 60 == 0) {
                mnodeman.CheckAndRemove();
                mnodeman.ProcessMasternodeConnections();
                mnpayments.CheckAndRemove();
                CleanTransactionLocksList();
            }

            darkSendPool.CheckTimeout();
            darkSendPool.CheckForCompleteQueue();

            if(nDoAutoNextRun == nTick) {
                if(darkSendPool.GetState() == POOL_STATE_IDLE)
                    darkSendPool.DoAutomaticDenominating();
                nDoAutoNextRun = nTick + DARKSEND_AUTO_TIMEOUT_MIN + insecure_rand()%(DARKSEND_AUTO_TIMEOUT_MAX - DARKSEND_AUTO_TIMEOUT_MIN);
            }
        }
    }
}
