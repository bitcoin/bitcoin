// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include "darksend.h"
#include "main.h"
#include "init.h"
#include "util.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "script/sign.h"
#include "instantx.h"
#include "txmempool.h"
#include "ui_interface.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <openssl/rand.h>

using namespace std;
using namespace boost;

// The main object for accessing Darksend
CDarksendPool darkSendPool;
// A helper object for signing messages from Masternodes
CDarkSendSigner darkSendSigner;
// The current Darksends in progress on the network
std::vector<CDarksendQueue> vecDarksendQueue;
// Keep track of the used Masternodes
std::vector<CTxIn> vecMasternodesUsed;
// Keep track of the scanning errors I've seen
map<uint256, CDarksendBroadcastTx> mapDarksendBroadcastTxes;
// Keep track of the active Masternode
CActiveMasternode activeMasternode;

/* *** BEGIN DARKSEND MAGIC - DASH **********
    Copyright (c) 2014-2015, Dash Developers
        eduffield - evan@dash.org
        udjinm6   - udjinm6@dash.org
*/

void CDarksendPool::ProcessMessageDarksend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all Darksend/Masternode related functionality
    if(!masternodeSync.IsBlockchainSynced()) return;

    if (strCommand == NetMsgType::DSACCEPT) { //Darksend Accept Into Pool

        int errorID;

        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            errorID = ERR_VERSION;
            LogPrintf("dsa -- incompatible version! \n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);

            return;
        }

        if(!fMasterNode){
            errorID = ERR_NOT_A_MN;
            LogPrintf("dsa -- not a Masternode! \n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);

            return;
        }

        int nDenom;
        CTransaction txCollateral;
        vRecv >> nDenom >> txCollateral;

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if(pmn == NULL)
        {
            errorID = ERR_MN_LIST;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
            return;
        }

        if(sessionUsers == 0) {
            if(pmn->nLastDsq != 0 &&
                pmn->nLastDsq + mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount){
                LogPrintf("dsa -- last dsq too recent, must wait. %s \n", pfrom->addr.ToString());
                errorID = ERR_RECENT;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                return;
            }
        }

        if(!IsCompatibleWithSession(nDenom, txCollateral, errorID))
        {
            LogPrintf("dsa -- not compatible with existing transactions! \n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
            return;
        } else {
            LogPrintf("dsa -- is compatible, please submit! \n");
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_ACCEPTED, errorID);
            return;
        }

    } else if (strCommand == NetMsgType::DSQUEUE) { //Darksend Queue
        TRY_LOCK(cs_darksend, lockRecv);
        if(!lockRecv) return;

        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            return;
        }

        CDarksendQueue dsq;
        vRecv >> dsq;

        CService addr;
        if(!dsq.GetAddress(addr)) return;
        if(!dsq.CheckSignature()) return;

        if(dsq.IsExpired()) return;

        CMasternode* pmn = mnodeman.Find(dsq.vin);
        if(pmn == NULL) return;

        // if the queue is ready, submit if we can
        if(dsq.ready) {
            if(!pSubmittedToMasternode) return;
            if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)addr){
                LogPrintf("dsq - message doesn't match current Masternode - %s != %s\n", pSubmittedToMasternode->addr.ToString(), addr.ToString());
                return;
            }

            if(state == POOL_STATUS_QUEUE){
                LogPrint("privatesend", "PrivateSend queue is ready - %s\n", addr.ToString());
                PrepareDarksendDenominate();
            }
        } else {
            BOOST_FOREACH(CDarksendQueue q, vecDarksendQueue){
                if(q.vin == dsq.vin) return;
            }

            LogPrint("privatesend", "dsq last %d last2 %d count %d\n", pmn->nLastDsq, pmn->nLastDsq + mnodeman.size()/5, mnodeman.nDsqCount);
            //don't allow a few nodes to dominate the queuing process
            if(pmn->nLastDsq != 0 &&
                pmn->nLastDsq + mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount){
                LogPrint("privatesend", "dsq -- Masternode sending too many dsq messages. %s \n", pmn->addr.ToString());
                return;
            }
            mnodeman.nDsqCount++;
            pmn->nLastDsq = mnodeman.nDsqCount;
            pmn->allowFreeTx = true;

            LogPrint("privatesend", "dsq - new PrivateSend queue object - %s\n", addr.ToString());
            vecDarksendQueue.push_back(dsq);
            dsq.Relay();
            dsq.time = GetTime();
        }

    } else if (strCommand == NetMsgType::DSVIN) { //DarkSend vIn
        int errorID;

        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            LogPrintf("dsi -- incompatible version! \n");
            errorID = ERR_VERSION;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);

            return;
        }

        if(!fMasterNode){
            LogPrintf("dsi -- not a Masternode! \n");
            errorID = ERR_NOT_A_MN;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);

            return;
        }

        std::vector<CTxIn> in;
        CAmount nAmount;
        CTransaction txCollateral;
        std::vector<CTxOut> out;
        vRecv >> in >> nAmount >> txCollateral >> out;

        //do we have enough users in the current session?
        if(!IsSessionReady()){
            LogPrintf("dsi -- session not complete! \n");
            errorID = ERR_SESSION;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
            return;
        }

        //do we have the same denominations as the current session?
        if(!IsCompatibleWithEntries(out))
        {
            LogPrintf("dsi -- not compatible with existing transactions! \n");
            errorID = ERR_EXISTING_TX;
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
            return;
        }

        //check it like a transaction
        {
            CAmount nValueIn = 0;
            CAmount nValueOut = 0;
            bool missingTx = false;

            CMutableTransaction tx;

            BOOST_FOREACH(const CTxOut o, out){
                nValueOut += o.nValue;
                tx.vout.push_back(o);

                if(o.scriptPubKey.size() != 25){
                    LogPrintf("dsi - non-standard pubkey detected! %s\n", ScriptToAsmStr(o.scriptPubKey));
                    errorID = ERR_NON_STANDARD_PUBKEY;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                    return;
                }
                if(!o.scriptPubKey.IsNormalPaymentScript()){
                    LogPrintf("dsi - invalid script! %s\n", ScriptToAsmStr(o.scriptPubKey));
                    errorID = ERR_INVALID_SCRIPT;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                    return;
                }
            }

            BOOST_FOREACH(const CTxIn i, in){
                tx.vin.push_back(i);

                LogPrint("privatesend", "dsi -- tx in %s\n", i.ToString());

                CTransaction tx2;
                uint256 hash;
                if(GetTransaction(i.prevout.hash, tx2, Params().GetConsensus(), hash, true)){
                    if(tx2.vout.size() > i.prevout.n) {
                        nValueIn += tx2.vout[i.prevout.n].nValue;
                    }
                } else{
                    missingTx = true;
                }
            }

            if (nValueIn > DARKSEND_POOL_MAX) {
                LogPrintf("dsi -- more than PrivateSend pool max! %s\n", tx.ToString());
                errorID = ERR_MAXIMUM;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                return;
            }

            if(!missingTx){
                if (nValueIn-nValueOut > nValueIn*.01) {
                    LogPrintf("dsi -- fees are too high! %s\n", tx.ToString());
                    errorID = ERR_FEES;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                    return;
                }
            } else {
                LogPrintf("dsi -- missing input tx! %s\n", tx.ToString());
                errorID = ERR_MISSING_TX;
                pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                return;
            }

            {
                LOCK(cs_main);
                CValidationState validationState;
                mempool.PrioritiseTransaction(tx.GetHash(), tx.GetHash().ToString(), 1000, 0.1*COIN);
                if(!AcceptToMemoryPool(mempool, validationState, CTransaction(tx), false, NULL, false, true, true)) {
                    LogPrintf("dsi -- transaction not valid! \n");
                    errorID = ERR_INVALID_TX;
                    pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
                    return;
                }
            }
        }

        if(AddEntry(in, nAmount, txCollateral, out, errorID)){
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_ACCEPTED, errorID);
            Check();

            RelayStatus(sessionID, GetState(), GetEntriesCount(), MASTERNODE_RESET);
        } else {
            pfrom->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, GetState(), GetEntriesCount(), MASTERNODE_REJECTED, errorID);
        }

    } else if (strCommand == NetMsgType::DSSTATUSUPDATE) { //Darksend status update
        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr){
            //LogPrintf("dssu - message doesn't match current Masternode - %s != %s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int sessionIDMessage;
        int state;
        int entriesCount;
        int accepted;
        int errorID;
        vRecv >> sessionIDMessage >> state >> entriesCount >> accepted >> errorID;

        LogPrint("privatesend", "dssu - state: %i entriesCount: %i accepted: %i error: %s \n", state, entriesCount, accepted, GetMessageByID(errorID));

        if((accepted != 1 && accepted != 0) && sessionID != sessionIDMessage){
            LogPrintf("dssu - message doesn't match current PrivateSend session %d %d\n", sessionID, sessionIDMessage);
            return;
        }

        StatusUpdate(state, entriesCount, accepted, errorID, sessionIDMessage);

    } else if (strCommand == NetMsgType::DSSIGNFINALTX) { //Darksend Sign Final Tx

        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            return;
        }

        vector<CTxIn> sigs;
        vRecv >> sigs;

        bool success = true;
        int nSigIndex = 0;
        int nSigsCount = (int)sigs.size();

        BOOST_FOREACH(const CTxIn item, sigs)
        {
            nSigIndex++;
            if(!AddScriptSig(item)) {
                success = false;
                break;
            }
            LogPrint("privatesend", "DSSIGNFINALTX - AddScriptSig %d/%d - success\n", nSigIndex, nSigsCount);
        }

        if(success){
            Check();
            RelayStatus(sessionID, GetState(), GetEntriesCount(), MASTERNODE_RESET);
        } else {
            LogPrint("privatesend", "DSSIGNFINALTX - AddScriptSig failed at %d/%d, session %d\n", nSigIndex, nSigsCount, sessionID);
        }
    } else if (strCommand == NetMsgType::DSFINALTX) { //Darksend Final tx
        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr){
            //LogPrintf("dsc - message doesn't match current Masternode - %s != %s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int sessionIDMessage;
        CTransaction txNew;
        vRecv >> sessionIDMessage >> txNew;

        if(sessionID != sessionIDMessage){
            LogPrint("privatesend", "dsf - message doesn't match current PrivateSend session %d %d\n", sessionID, sessionIDMessage);
            return;
        }

        //check to see if input is spent already? (and probably not confirmed)
        SignFinalTransaction(txNew, pfrom);

    } else if (strCommand == NetMsgType::DSCOMPLETE) { //Darksend Complete

        if (pfrom->nVersion < MIN_POOL_PEER_PROTO_VERSION) {
            return;
        }

        if(!pSubmittedToMasternode) return;
        if((CNetAddr)pSubmittedToMasternode->addr != (CNetAddr)pfrom->addr){
            //LogPrintf("dsc - message doesn't match current Masternode - %s != %s\n", pSubmittedToMasternode->addr.ToString(), pfrom->addr.ToString());
            return;
        }

        int sessionIDMessage;
        bool error;
        int errorID;
        vRecv >> sessionIDMessage >> error >> errorID;

        if(sessionID != sessionIDMessage){
            LogPrint("privatesend", "dsc - message doesn't match current PrivateSend session %d %d\n", darkSendPool.sessionID, sessionIDMessage);
            return;
        }

        darkSendPool.CompletedTransaction(error, errorID);
    }

}

void CDarksendPool::Reset(){
    cachedLastSuccess = 0;
    lastNewBlock = 0;
    txCollateral = CMutableTransaction();
    vecMasternodesUsed.clear();
    UnlockCoins();
    SetNull();
}

void CDarksendPool::SetNull(){

    // MN side
    sessionUsers = 0;
    vecSessionCollateral.clear();

    // Client side
    entriesCount = 0;
    lastEntryAccepted = 0;
    countEntriesAccepted = 0;
    sessionFoundMasternode = false;

    // Both sides
    state = POOL_STATUS_IDLE;
    sessionID = 0;
    sessionDenom = 0;
    entries.clear();
    finalMutableTransaction.vin.clear();
    finalMutableTransaction.vout.clear();
    lastTimeChanged = GetTimeMillis();

    // -- seed random number generator (used for ordering output lists)
    unsigned int seed = 0;
    RAND_bytes((unsigned char*)&seed, sizeof(seed));
    std::srand(seed);
}

bool CDarksendPool::SetCollateralAddress(std::string strAddress){
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
    {
        LogPrintf("CDarksendPool::SetCollateralAddress - Invalid PrivateSend collateral address\n");
        return false;
    }
    collateralPubKey = GetScriptForDestination(address.Get());
    return true;
}

//
// Unlock coins after Darksend fails or succeeds
//
void CDarksendPool::UnlockCoins(){
    while(true) {
        TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
        if(!lockWallet) {MilliSleep(50); continue;}
        BOOST_FOREACH(CTxIn v, lockedCoins)
            pwalletMain->UnlockCoin(v.prevout);
        break;
    }

    lockedCoins.clear();
}

std::string CDarksendPool::GetStatus()
{
    static int showingDarkSendMessage = 0;
    showingDarkSendMessage += 10;
    std::string suffix = "";

    if((pCurrentBlockIndex && pCurrentBlockIndex->nHeight - cachedLastSuccess < minBlockSpacing) || !masternodeSync.IsBlockchainSynced()) {
        return strAutoDenomResult;
    }
    switch(state) {
        case POOL_STATUS_IDLE:
            return _("PrivateSend is idle.");
        case POOL_STATUS_ACCEPTING_ENTRIES:
            if(entriesCount == 0) {
                showingDarkSendMessage = 0;
                return strAutoDenomResult;
            } else if (lastEntryAccepted == 1) {
                if(showingDarkSendMessage % 10 > 8) {
                    lastEntryAccepted = 0;
                    showingDarkSendMessage = 0;
                }
                return _("PrivateSend request complete:") + " " + _("Your transaction was accepted into the pool!");
            } else {
                std::string suffix = "";
                if(     showingDarkSendMessage % 70 <= 40) return strprintf(_("Submitted following entries to masternode: %u / %d"), entriesCount, GetMaxPoolTransactions());
                else if(showingDarkSendMessage % 70 <= 50) suffix = ".";
                else if(showingDarkSendMessage % 70 <= 60) suffix = "..";
                else if(showingDarkSendMessage % 70 <= 70) suffix = "...";
                return strprintf(_("Submitted to masternode, waiting for more entries ( %u / %d ) %s"), entriesCount, GetMaxPoolTransactions(), suffix);
            }
        case POOL_STATUS_SIGNING:
            if(     showingDarkSendMessage % 70 <= 40) return _("Found enough users, signing ...");
            else if(showingDarkSendMessage % 70 <= 50) suffix = ".";
            else if(showingDarkSendMessage % 70 <= 60) suffix = "..";
            else if(showingDarkSendMessage % 70 <= 70) suffix = "...";
            return strprintf(_("Found enough users, signing ( waiting %s )"), suffix);
        case POOL_STATUS_TRANSMISSION:
            return _("Transmitting final transaction.");
        case POOL_STATUS_FINALIZE_TRANSACTION:
            return _("Finalizing transaction.");
        case POOL_STATUS_ERROR:
            return _("PrivateSend request incomplete:") + " " + lastMessage + " " + _("Will retry...");
        case POOL_STATUS_SUCCESS:
            return _("PrivateSend request complete:") + " " + lastMessage;
        case POOL_STATUS_QUEUE:
            if(     showingDarkSendMessage % 70 <= 30) suffix = ".";
            else if(showingDarkSendMessage % 70 <= 50) suffix = "..";
            else if(showingDarkSendMessage % 70 <= 70) suffix = "...";
            return strprintf(_("Submitted to masternode, waiting in queue %s"), suffix);;
       default:
            return strprintf(_("Unknown state: id = %u"), state);
    }
}

//
// Check the Darksend progress and send client updates if a Masternode
//
void CDarksendPool::Check()
{
    if(fMasterNode) LogPrint("privatesend", "CDarksendPool::Check() - entries count %lu\n", entries.size());
    //printf("CDarksendPool::Check() %d - %d - %d\n", state, anonTx.CountEntries(), GetTimeMillis()-lastTimeChanged);

    if(fMasterNode) {
        LogPrint("privatesend", "CDarksendPool::Check() - entries count %lu\n", entries.size());

        // If entries is full, then move on to the next phase
        if(state == POOL_STATUS_ACCEPTING_ENTRIES && (int)entries.size() >= GetMaxPoolTransactions())
        {
            LogPrint("privatesend", "CDarksendPool::Check() -- TRYING TRANSACTION \n");
            UpdateState(POOL_STATUS_FINALIZE_TRANSACTION);
        }
    }

    // create the finalized transaction for distribution to the clients
    if(state == POOL_STATUS_FINALIZE_TRANSACTION) {
        LogPrint("privatesend", "CDarksendPool::Check() -- FINALIZE TRANSACTIONS\n");
        UpdateState(POOL_STATUS_SIGNING);

        if (fMasterNode) {
            CMutableTransaction txNew;

            // make our new transaction
            for(unsigned int i = 0; i < entries.size(); i++){
                BOOST_FOREACH(const CTxOut& v, entries[i].vout)
                    txNew.vout.push_back(v);

                BOOST_FOREACH(const CTxDSIn& s, entries[i].sev)
                    txNew.vin.push_back(s);
            }

            // BIP69 https://github.com/kristovatlas/bips/blob/master/bip-0069.mediawiki
            sort(txNew.vin.begin(), txNew.vin.end());
            sort(txNew.vout.begin(), txNew.vout.end());


            LogPrint("privatesend", "Transaction 1: %s\n", txNew.ToString());
            finalMutableTransaction = txNew;

            // request signatures from clients
            RelayFinalTransaction(sessionID, finalMutableTransaction);
        }
    }

    // If we have all of the signatures, try to compile the transaction
    if(fMasterNode && state == POOL_STATUS_SIGNING && SignaturesComplete()) {
        LogPrint("privatesend", "CDarksendPool::Check() -- SIGNING\n");
        UpdateState(POOL_STATUS_TRANSMISSION);

        CheckFinalTransaction();
    }

    // reset if we're here for 10 seconds
    if((state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) && GetTimeMillis()-lastTimeChanged >= 10000) {
        LogPrint("privatesend", "CDarksendPool::Check() -- timeout, RESETTING\n");
        UnlockCoins();
        SetNull();
        if(fMasterNode) RelayStatus(sessionID, GetState(), GetEntriesCount(), MASTERNODE_RESET);
    }
}

void CDarksendPool::CheckFinalTransaction()
{
    if (!fMasterNode) return; // check and relay final tx only on masternode

    CTransaction finalTransaction = CTransaction(finalMutableTransaction);

    LogPrint("privatesend", "Transaction 2: %s\n", finalTransaction.ToString());

    {
        // See if the transaction is valid
        TRY_LOCK(cs_main, lockMain);
        CValidationState validationState;
        mempool.PrioritiseTransaction(finalTransaction.GetHash(), finalTransaction.GetHash().ToString(), 1000, 0.1*COIN);
        if(!lockMain || !AcceptToMemoryPool(mempool, validationState, finalTransaction, false, NULL, false, true, true))
        {
            LogPrintf("CDarksendPool::Check() - CommitTransaction : Error: Transaction not valid\n");
            SetNull();

            // not much we can do in this case
            UpdateState(POOL_STATUS_ACCEPTING_ENTRIES);
            RelayCompletedTransaction(sessionID, true, ERR_INVALID_TX);
            return;
        }
    }
    LogPrintf("CDarksendPool::Check() -- IS MASTER -- TRANSMITTING DARKSEND\n");

    // sign a message

    int64_t sigTime = GetAdjustedTime();
    std::string strMessage = finalTransaction.GetHash().ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";
    std::vector<unsigned char> vchSig;
    CKey key2;
    CPubKey pubkey2;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, strError, key2, pubkey2))
    {
        LogPrintf("CDarksendPool::Check() - ERROR: Invalid Masternodeprivkey: '%s'\n", strError);
        return;
    }

    if(!darkSendSigner.SignMessage(strMessage, strError, vchSig, key2)) {
        LogPrintf("CDarksendPool::Check() - Sign message failed\n");
        return;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchSig, strMessage, strError)) {
        LogPrintf("CDarksendPool::Check() - Verify message failed\n");
        return;
    }

    if(!mapDarksendBroadcastTxes.count(finalTransaction.GetHash())){
        CDarksendBroadcastTx dstx;
        dstx.tx = finalTransaction;
        dstx.vin = activeMasternode.vin;
        dstx.vchSig = vchSig;
        dstx.sigTime = sigTime;

        mapDarksendBroadcastTxes.insert(make_pair(finalTransaction.GetHash(), dstx));
    }

    CInv inv(MSG_DSTX, finalTransaction.GetHash());
    RelayInv(inv);

    // Tell the clients it was successful
    RelayCompletedTransaction(sessionID, false, MSG_SUCCESS);

    // Randomly charge clients
    ChargeRandomFees();

    // Reset
    LogPrint("privatesend", "CDarksendPool::Check() -- COMPLETED -- RESETTING\n");
    SetNull();
    RelayStatus(sessionID, GetState(), GetEntriesCount(), MASTERNODE_RESET);
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
void CDarksendPool::ChargeFees(){
    if(!fMasterNode) return;

    //we don't need to charge collateral for every offence.
    int offences = 0;
    int r = rand()%100;
    if(r > 33) return;

    if(state == POOL_STATUS_ACCEPTING_ENTRIES){
        BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
            bool found = false;
            BOOST_FOREACH(const CDarkSendEntry& v, entries) {
                if(v.collateral == txCollateral) {
                    found = true;
                }
            }

            // This queue entry didn't send us the promised transaction
            if(!found){
                LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't send transaction). Found offence.\n");
                offences++;
            }
        }
    }

    if(state == POOL_STATUS_SIGNING) {
        // who didn't sign?
        BOOST_FOREACH(const CDarkSendEntry v, entries) {
            BOOST_FOREACH(const CTxDSIn s, v.sev) {
                if(!s.fHasSig){
                    LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't sign). Found offence\n");
                    offences++;
                }
            }
        }
    }

    r = rand()%100;
    int target = 0;

    //mostly offending?
    if(offences >= Params().PoolMaxTransactions()-1 && r > 33) return;

    //everyone is an offender? That's not right
    if(offences >= Params().PoolMaxTransactions()) return;

    //charge one of the offenders randomly
    if(offences > 1) target = 50;

    //pick random client to charge
    r = rand()%100;

    if(state == POOL_STATUS_ACCEPTING_ENTRIES){
        BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
            bool found = false;
            BOOST_FOREACH(const CDarkSendEntry& v, entries) {
                if(v.collateral == txCollateral) {
                    found = true;
                }
            }

            // This queue entry didn't send us the promised transaction
            if(!found && r > target){
                LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't send transaction). charging fees.\n");

                CWalletTx wtxCollateral = CWalletTx(pwalletMain, txCollateral);

                // Broadcast
                if (!wtxCollateral.AcceptToMemoryPool(true))
                {
                    // This must not fail. The transaction has already been signed and recorded.
                    LogPrintf("CDarksendPool::ChargeFees() : Error: Transaction not valid");
                }
                wtxCollateral.RelayWalletTransaction();
                return;
            }
        }
    }

    if(state == POOL_STATUS_SIGNING) {
        // who didn't sign?
        BOOST_FOREACH(const CDarkSendEntry v, entries) {
            BOOST_FOREACH(const CTxDSIn s, v.sev) {
                if(!s.fHasSig && r > target){
                    LogPrintf("CDarksendPool::ChargeFees -- found uncooperative node (didn't sign). charging fees.\n");

                    CWalletTx wtxCollateral = CWalletTx(pwalletMain, v.collateral);

                    // Broadcast
                    if (!wtxCollateral.AcceptToMemoryPool(false))
                    {
                        // This must not fail. The transaction has already been signed and recorded.
                        LogPrintf("CDarksendPool::ChargeFees() : Error: Transaction not valid");
                    }
                    wtxCollateral.RelayWalletTransaction();
                    return;
                }
            }
        }
    }
}

// charge the collateral randomly
//  - Darksend is completely free, to pay miners we randomly pay the collateral of users.
void CDarksendPool::ChargeRandomFees(){
    if(fMasterNode) {
        int i = 0;

        BOOST_FOREACH(const CTransaction& txCollateral, vecSessionCollateral) {
            int r = rand()%100;

            /*
                Collateral Fee Charges:

                Being that Darksend has "no fees" we need to have some kind of cost associated
                with using it to stop abuse. Otherwise it could serve as an attack vector and
                allow endless transaction that would bloat Dash and make it unusable. To
                stop these kinds of attacks 1 in 10 successful transactions are charged. This
                adds up to a cost of 0.001DRK per transaction on average.
            */
            if(r <= 10)
            {
                LogPrintf("CDarksendPool::ChargeRandomFees -- charging random fees. %u\n", i);

                CWalletTx wtxCollateral = CWalletTx(pwalletMain, txCollateral);

                // Broadcast
                if (!wtxCollateral.AcceptToMemoryPool(true))
                {
                    // This must not fail. The transaction has already been signed and recorded.
                    LogPrintf("CDarksendPool::ChargeRandomFees() : Error: Transaction not valid");
                }
                wtxCollateral.RelayWalletTransaction();
            }
        }
    }
}

//
// Check for various timeouts (queue objects, Darksend, etc)
//
void CDarksendPool::CheckTimeout(){

    // check Darksend queue objects for timeouts
    int c = 0;
    vector<CDarksendQueue>::iterator it = vecDarksendQueue.begin();
    while(it != vecDarksendQueue.end()){
        if((*it).IsExpired()){
            LogPrint("privatesend", "CDarksendPool::CheckTimeout() : Removing expired queue entry - %d\n", c);
            it = vecDarksendQueue.erase(it);
        } else ++it;
        c++;
    }

    if(!fEnablePrivateSend && !fMasterNode) return;

    // catching hanging sessions
    if(!fMasterNode) {
        switch(state) {
            case POOL_STATUS_TRANSMISSION:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout() -- Session complete -- Running Check()\n");
                Check();
                break;
            case POOL_STATUS_ERROR:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout() -- Pool error -- Running Check()\n");
                Check();
                break;
            case POOL_STATUS_SUCCESS:
                LogPrint("privatesend", "CDarksendPool::CheckTimeout() -- Pool success -- Running Check()\n");
                Check();
                break;
        }
    }

    int addLagTime = 0;
    if(!fMasterNode) addLagTime = 10000; //if we're the client, give the server a few extra seconds before resetting.

    if(state == POOL_STATUS_ACCEPTING_ENTRIES || state == POOL_STATUS_QUEUE){
        c = 0;

        // check for a timeout and reset if needed
        vector<CDarkSendEntry>::iterator it2 = entries.begin();
        while(it2 != entries.end()){
            if((*it2).IsExpired()){
                LogPrint("privatesend", "CDarksendPool::CheckTimeout() : Removing expired entry - %d\n", c);
                it2 = entries.erase(it2);
                if(entries.size() == 0){
                    UnlockCoins();
                    SetNull();
                }
                if(fMasterNode){
                    RelayStatus(sessionID, GetState(), GetEntriesCount(), MASTERNODE_RESET);
                }
            } else ++it2;
            c++;
        }

        if(GetTimeMillis()-lastTimeChanged >= (DARKSEND_QUEUE_TIMEOUT*1000)+addLagTime){
            UnlockCoins();
            SetNull();
        }
    } else if(GetTimeMillis()-lastTimeChanged >= (DARKSEND_QUEUE_TIMEOUT*1000)+addLagTime){
        LogPrint("privatesend", "CDarksendPool::CheckTimeout() -- Session timed out (%ds) -- resetting\n", DARKSEND_QUEUE_TIMEOUT);
        UnlockCoins();
        SetNull();

        UpdateState(POOL_STATUS_ERROR);
        lastMessage = _("Session timed out.");
    }

    if(state == POOL_STATUS_SIGNING && GetTimeMillis()-lastTimeChanged >= (DARKSEND_SIGNING_TIMEOUT*1000)+addLagTime ) {
            LogPrint("privatesend", "CDarksendPool::CheckTimeout() -- Session timed out (%ds) -- restting\n", DARKSEND_SIGNING_TIMEOUT);
            ChargeFees();
            UnlockCoins();
            SetNull();

            UpdateState(POOL_STATUS_ERROR);
            lastMessage = _("Signing timed out.");
    }
}

//
// Check for complete queue
//
void CDarksendPool::CheckForCompleteQueue(){
    if(!fEnablePrivateSend && !fMasterNode) return;

    /* Check to see if we're ready for submissions from clients */
    //
    // After receiving multiple dsa messages, the queue will switch to "accepting entries"
    // which is the active state right before merging the transaction
    //
    if(state == POOL_STATUS_QUEUE && sessionUsers == GetMaxPoolTransactions()) {
        UpdateState(POOL_STATUS_ACCEPTING_ENTRIES);

        CDarksendQueue dsq;
        dsq.nDenom = sessionDenom;
        dsq.vin = activeMasternode.vin;
        dsq.time = GetTime();
        dsq.ready = true;
        dsq.Sign();
        dsq.Relay();
    }
}

// check to see if the signature is valid
bool CDarksendPool::SignatureValid(const CScript& newSig, const CTxIn& newVin){
    CMutableTransaction txNew;
    txNew.vin.clear();
    txNew.vout.clear();

    int found = -1;
    CScript sigPubKey = CScript();
    unsigned int i = 0;

    BOOST_FOREACH(CDarkSendEntry& e, entries) {
        BOOST_FOREACH(const CTxOut& out, e.vout)
            txNew.vout.push_back(out);

        BOOST_FOREACH(const CTxDSIn& s, e.sev){
            txNew.vin.push_back(s);

            if(s == newVin){
                found = i;
                sigPubKey = s.prevPubKey;
            }
            i++;
        }
    }

    if(found >= 0){ //might have to do this one input at a time?
        int n = found;
        txNew.vin[n].scriptSig = newSig;
        LogPrint("privatesend", "CDarksendPool::SignatureValid() - Sign with sig %s\n", ScriptToAsmStr(newSig).substr(0,24));
        if (!VerifyScript(txNew.vin[n].scriptSig, sigPubKey, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, MutableTransactionSignatureChecker(&txNew, n))){
            LogPrint("privatesend", "CDarksendPool::SignatureValid() - Signing - Error signing input %u\n", n);
            return false;
        }
    }

    LogPrint("privatesend", "CDarksendPool::SignatureValid() - Signing - Successfully validated input\n");
    return true;
}

// check to make sure the collateral provided by the client is valid
bool CDarksendPool::IsCollateralValid(const CTransaction& txCollateral){
    if(txCollateral.vout.size() < 1) return false;
    if(txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;
    bool missingTx = false;

    BOOST_FOREACH(const CTxOut o, txCollateral.vout){
        nValueOut += o.nValue;

        if(!o.scriptPubKey.IsNormalPaymentScript()){
            LogPrintf ("CDarksendPool::IsCollateralValid - Invalid Script %s", txCollateral.ToString());
            return false;
        }
    }

    BOOST_FOREACH(const CTxIn i, txCollateral.vin){
        CTransaction tx2;
        uint256 hash;
        if(GetTransaction(i.prevout.hash, tx2, Params().GetConsensus(), hash, true)){
            if(tx2.vout.size() > i.prevout.n) {
                nValueIn += tx2.vout[i.prevout.n].nValue;
            }
        } else{
            missingTx = true;
        }
    }

    if(missingTx){
        LogPrint("privatesend", "CDarksendPool::IsCollateralValid - Unknown inputs in collateral transaction - %s", txCollateral.ToString());
        return false;
    }

    //collateral transactions are required to pay out DARKSEND_COLLATERAL as a fee to the miners
    if(nValueIn - nValueOut < DARKSEND_COLLATERAL) {
        LogPrint("privatesend", "CDarksendPool::IsCollateralValid - did not include enough fees in transaction %d\n%s", nValueOut-nValueIn, txCollateral.ToString());
        return false;
    }

    LogPrint("privatesend", "CDarksendPool::IsCollateralValid %s", txCollateral.ToString());

    {
        LOCK(cs_main);
        CValidationState validationState;
        if(!AcceptToMemoryPool(mempool, validationState, txCollateral, false, NULL, false, true, true)){
            if(fDebug) LogPrintf ("CDarksendPool::IsCollateralValid - didn't pass IsAcceptable\n");
            return false;
        }
    }

    return true;
}


//
// Add a clients transaction to the pool
//
bool CDarksendPool::AddEntry(const std::vector<CTxIn>& newInput, const CAmount& nAmount, const CTransaction& txCollateral, const std::vector<CTxOut>& newOutput, int& errorID){
    if (!fMasterNode) return false;

    BOOST_FOREACH(CTxIn in, newInput) {
        if (in.prevout.IsNull() || nAmount < 0) {
            LogPrint("privatesend", "CDarksendPool::AddEntry - input not valid!\n");
            errorID = ERR_INVALID_INPUT;
            sessionUsers--;
            return false;
        }
    }

    if (!IsCollateralValid(txCollateral)){
        LogPrint("privatesend", "CDarksendPool::AddEntry - collateral not valid!\n");
        errorID = ERR_INVALID_COLLATERAL;
        sessionUsers--;
        return false;
    }

    if((int)entries.size() >= GetMaxPoolTransactions()){
        LogPrint("privatesend", "CDarksendPool::AddEntry - entries is full!\n");
        errorID = ERR_ENTRIES_FULL;
        sessionUsers--;
        return false;
    }

    BOOST_FOREACH(CTxIn in, newInput) {
        LogPrint("privatesend", "looking for vin -- %s\n", in.ToString());
        BOOST_FOREACH(const CDarkSendEntry& v, entries) {
            BOOST_FOREACH(const CTxDSIn& s, v.sev){
                if((CTxIn)s == in) {
                    LogPrint("privatesend", "CDarksendPool::AddEntry - found in vin\n");
                    errorID = ERR_ALREADY_HAVE;
                    sessionUsers--;
                    return false;
                }
            }
        }
    }

    CDarkSendEntry v;
    v.Add(newInput, nAmount, txCollateral, newOutput);
    entries.push_back(v);

    LogPrint("privatesend", "CDarksendPool::AddEntry -- adding %s\n", newInput[0].ToString());
    errorID = MSG_ENTRIES_ADDED;

    return true;
}

bool CDarksendPool::AddScriptSig(const CTxIn& newVin){
    LogPrint("privatesend", "CDarksendPool::AddScriptSig -- new sig  %s\n", ScriptToAsmStr(newVin.scriptSig).substr(0,24));


    BOOST_FOREACH(const CDarkSendEntry& v, entries) {
        BOOST_FOREACH(const CTxDSIn& s, v.sev){
            if(s.scriptSig == newVin.scriptSig) {
                LogPrint("privatesend", "CDarksendPool::AddScriptSig - already exists\n");
                return false;
            }
        }
    }

    if(!SignatureValid(newVin.scriptSig, newVin)){
        LogPrint("privatesend", "CDarksendPool::AddScriptSig - Invalid Sig\n");
        return false;
    }

    LogPrint("privatesend", "CDarksendPool::AddScriptSig -- sig %s\n", ScriptToAsmStr(newVin.scriptSig));

    BOOST_FOREACH(CTxIn& vin, finalMutableTransaction.vin){
        if(newVin.prevout == vin.prevout && vin.nSequence == newVin.nSequence){
            vin.scriptSig = newVin.scriptSig;
            vin.prevPubKey = newVin.prevPubKey;
            LogPrint("privatesend", "CDarksendPool::AddScriptSig -- adding to finalMutableTransaction  %s\n", ScriptToAsmStr(newVin.scriptSig).substr(0,24));
        }
    }
    for(unsigned int i = 0; i < entries.size(); i++){
        if(entries[i].AddSig(newVin)){
            LogPrint("privatesend", "CDarksendPool::AddScriptSig -- adding  %s\n", ScriptToAsmStr(newVin.scriptSig).substr(0,24));
            return true;
        }
    }

    LogPrintf("CDarksendPool::AddScriptSig -- Couldn't set sig!\n" );
    return false;
}

// Check to make sure everything is signed
bool CDarksendPool::SignaturesComplete(){
    BOOST_FOREACH(const CDarkSendEntry& v, entries) {
        BOOST_FOREACH(const CTxDSIn& s, v.sev){
            if(!s.fHasSig) return false;
        }
    }
    return true;
}

//
// Execute a Darksend denomination via a Masternode.
// This is only ran from clients
//
void CDarksendPool::SendDarksendDenominate(std::vector<CTxIn>& vin, std::vector<CTxOut>& vout, CAmount amount){

    if(fMasterNode) {
        LogPrintf("CDarksendPool::SendDarksendDenominate() - PrivateSend from a Masternode is not supported currently.\n");
        return;
    }

    if(txCollateral == CMutableTransaction()){
        LogPrintf ("CDarksendPool:SendDarksendDenominate() - PrivateSend collateral not set");
        return;
    }

    // lock the funds we're going to use
    BOOST_FOREACH(CTxIn in, txCollateral.vin)
        lockedCoins.push_back(in);

    BOOST_FOREACH(CTxIn in, vin)
        lockedCoins.push_back(in);

    //BOOST_FOREACH(CTxOut o, vout)
    //    LogPrintf(" vout - %s\n", o.ToString());


    // we should already be connected to a Masternode
    if(!sessionFoundMasternode){
        LogPrintf("CDarksendPool::SendDarksendDenominate() - No Masternode has been selected yet.\n");
        UnlockCoins();
        SetNull();
        return;
    }

    if (!CheckDiskSpace()) {
        UnlockCoins();
        SetNull();
        fEnablePrivateSend = false;
        LogPrintf("CDarksendPool::SendDarksendDenominate() - Not enough disk space, disabling PrivateSend.\n");
        return;
    }

    UpdateState(POOL_STATUS_ACCEPTING_ENTRIES);

    LogPrintf("CDarksendPool::SendDarksendDenominate() - Added transaction to pool.\n");

    ClearLastMessage();

    //check it against the memory pool to make sure it's valid
    {
        CAmount nValueOut = 0;

        CValidationState validationState;
        CMutableTransaction tx;

        BOOST_FOREACH(const CTxOut& o, vout){
            nValueOut += o.nValue;
            tx.vout.push_back(o);
        }

        BOOST_FOREACH(const CTxIn& i, vin){
            tx.vin.push_back(i);

            LogPrint("privatesend", "CDarksendPool::SendDarksendDenominate() -- tx in %s\n", i.ToString());
        }

        LogPrintf("Submitting tx %s\n", tx.ToString());

        mempool.PrioritiseTransaction(tx.GetHash(), tx.GetHash().ToString(), 1000, 0.1*COIN);
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain || !AcceptToMemoryPool(mempool, validationState, CTransaction(tx), false, NULL, false, true, true)){
            LogPrintf("CDarksendPool::SendDarksendDenominate() -- transaction failed! %s \n", tx.ToString());
            UnlockCoins();
            SetNull();
            return;
        }
    }

    // store our entry for later use
    CDarkSendEntry e;
    e.Add(vin, amount, txCollateral, vout);
    entries.push_back(e);

    RelayIn(entries[0].sev, entries[0].amount, txCollateral, entries[0].vout);
    Check();
}

// Incoming message from Masternode updating the progress of Darksend
//    newAccepted:  -1 mean's it'n not a "transaction accepted/not accepted" message, just a standard update
//                  0 means transaction was not accepted
//                  1 means transaction was accepted

bool CDarksendPool::StatusUpdate(int newState, int newEntriesCount, int newAccepted, int& errorID, int newSessionID){
    if(fMasterNode) return false;
    if(state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) return false;

    UpdateState(newState);
    entriesCount = newEntriesCount;

    if(errorID != MSG_NOERR) strAutoDenomResult = _("Masternode:") + " " + GetMessageByID(errorID);

    if(newAccepted != -1) {
        lastEntryAccepted = newAccepted;
        countEntriesAccepted += newAccepted;
        if(newAccepted == 0){
            UpdateState(POOL_STATUS_ERROR);
            lastMessage = GetMessageByID(errorID);
        }

        if(newAccepted == 1 && newSessionID != 0) {
            sessionID = newSessionID;
            LogPrintf("CDarksendPool::StatusUpdate - set sessionID to %d\n", sessionID);
            sessionFoundMasternode = true;
        }
    }

    if(newState == POOL_STATUS_ACCEPTING_ENTRIES){
        if(newAccepted == 1){
            LogPrintf("CDarksendPool::StatusUpdate - entry accepted! \n");
            sessionFoundMasternode = true;
            //wait for other users. Masternode will report when ready
            UpdateState(POOL_STATUS_QUEUE);
        } else if (newAccepted == 0 && sessionID == 0 && !sessionFoundMasternode) {
            LogPrintf("CDarksendPool::StatusUpdate - entry not accepted by Masternode \n");
            UnlockCoins();
            UpdateState(POOL_STATUS_ACCEPTING_ENTRIES);
            DoAutomaticDenominating(); //try another Masternode
        }
        if(sessionFoundMasternode) return true;
    }

    return true;
}

//
// After we receive the finalized transaction from the Masternode, we must
// check it to make sure it's what we want, then sign it if we agree.
// If we refuse to sign, it's possible we'll be charged collateral
//
bool CDarksendPool::SignFinalTransaction(CTransaction& finalTransactionNew, CNode* node){
    if(fMasterNode) return false;

    finalMutableTransaction = finalTransactionNew;
    LogPrintf("CDarksendPool::SignFinalTransaction %s", finalMutableTransaction.ToString());

    vector<CTxIn> sigs;

    //make sure my inputs/outputs are present, otherwise refuse to sign
    BOOST_FOREACH(const CDarkSendEntry e, entries) {
        BOOST_FOREACH(const CTxDSIn s, e.sev) {
            /* Sign my transaction and all outputs */
            int mine = -1;
            CScript prevPubKey = CScript();
            CTxIn vin = CTxIn();

            for(unsigned int i = 0; i < finalMutableTransaction.vin.size(); i++){
                if(finalMutableTransaction.vin[i] == s){
                    mine = i;
                    prevPubKey = s.prevPubKey;
                    vin = s;
                }
            }

            if(mine >= 0){ //might have to do this one input at a time?
                int foundOutputs = 0;
                CAmount nValue1 = 0;
                CAmount nValue2 = 0;

                for(unsigned int i = 0; i < finalMutableTransaction.vout.size(); i++){
                    BOOST_FOREACH(const CTxOut& o, e.vout) {
                        if(finalMutableTransaction.vout[i] == o){
                            foundOutputs++;
                            nValue1 += finalMutableTransaction.vout[i].nValue;
                        }
                    }
                }

                BOOST_FOREACH(const CTxOut o, e.vout)
                    nValue2 += o.nValue;

                int targetOuputs = e.vout.size();
                if(foundOutputs < targetOuputs || nValue1 != nValue2) {
                    // in this case, something went wrong and we'll refuse to sign. It's possible we'll be charged collateral. But that's
                    // better then signing if the transaction doesn't look like what we wanted.
                    LogPrintf("CDarksendPool::Sign - My entries are not correct! Refusing to sign. %d entries %d target. \n", foundOutputs, targetOuputs);
                    UnlockCoins();
                    SetNull();

                    return false;
                }

                const CKeyStore& keystore = *pwalletMain;

                LogPrint("privatesend", "CDarksendPool::Sign - Signing my input %i\n", mine);
                if(!SignSignature(keystore, prevPubKey, finalMutableTransaction, mine, int(SIGHASH_ALL|SIGHASH_ANYONECANPAY))) { // changes scriptSig
                    LogPrint("privatesend", "CDarksendPool::Sign - Unable to sign my own transaction! \n");
                    // not sure what to do here, it will timeout...?
                }

                sigs.push_back(finalMutableTransaction.vin[mine]);
                LogPrint("privatesend", " -- dss %d %d %s\n", mine, (int)sigs.size(), ScriptToAsmStr(finalMutableTransaction.vin[mine].scriptSig));
            }

        }

        LogPrint("privatesend", "CDarksendPool::Sign - txNew:\n%s", finalMutableTransaction.ToString());
    }

	// push all of our signatures to the Masternode
	if(sigs.size() > 0 && node != NULL)
	    node->PushMessage(NetMsgType::DSSIGNFINALTX, sigs);


    return true;
}

void CDarksendPool::NewBlock()
{
    LogPrint("privatesend", "CDarksendPool::NewBlock \n");

    //we we're processing lots of blocks, we'll just leave
    if(GetTime() - lastNewBlock < 10) return;
    lastNewBlock = GetTime();

    CheckTimeout();
}

// Darksend transaction was completed (failed or successful)
void CDarksendPool::CompletedTransaction(bool error, int errorID)
{
    if(fMasterNode) return;

    if(error){
        LogPrintf("CompletedTransaction -- error \n");
        UpdateState(POOL_STATUS_ERROR);

        Check();
        UnlockCoins();
        SetNull();
    } else {
        LogPrintf("CompletedTransaction -- success \n");
        UpdateState(POOL_STATUS_SUCCESS);

        UnlockCoins();
        SetNull();

        // To avoid race conditions, we'll only let DS run once per block
        cachedLastSuccess = pCurrentBlockIndex->nHeight;
    }
    lastMessage = GetMessageByID(errorID);
}

void CDarksendPool::ClearLastMessage()
{
    lastMessage = "";
}

//
// Passively run Darksend in the background to anonymize funds based on the given configuration.
//
// This does NOT run by default for daemons, only for QT.
//
bool CDarksendPool::DoAutomaticDenominating(bool fDryRun)
{
    if(!fEnablePrivateSend) return false;
    if(fMasterNode) return false;

    if(!pCurrentBlockIndex) return false;

    if(state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) return false;
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

    if (!fDryRun && pwalletMain->IsLocked()){
        strAutoDenomResult = _("Wallet is locked.");
        return false;
    }

    if(!fPrivateSendMultiSession && pCurrentBlockIndex->nHeight - cachedLastSuccess < minBlockSpacing) {
        LogPrintf("CDarksendPool::DoAutomaticDenominating - Last successful PrivateSend action was too recent\n");
        strAutoDenomResult = _("Last successful PrivateSend action was too recent.");
        return false;
    }

    if(mnodeman.size() == 0){
        LogPrint("privatesend", "CDarksendPool::DoAutomaticDenominating - No Masternodes detected\n");
        strAutoDenomResult = _("No Masternodes detected.");
        return false;
    }

    // ** find the coins we'll use
    std::vector<CTxIn> vCoins;
    CAmount nValueMin = CENT;
    CAmount nValueIn = 0;

    CAmount nOnlyDenominatedBalance;
    CAmount nBalanceNeedsDenominated;

    CAmount nLowestDenom = darkSendDenominations[darkSendDenominations.size() - 1];

    // if there are no confirmed DS collateral inputs yet
    if(!pwalletMain->HasCollateralInputs())
        // should have some additional amount for them
        nLowestDenom += DARKSEND_COLLATERAL*4;

    CAmount nBalanceNeedsAnonymized = nAnonymizeDashAmount*COIN - pwalletMain->GetAnonymizedBalance();

    // if balanceNeedsAnonymized is more than pool max, take the pool max
    if(nBalanceNeedsAnonymized > DARKSEND_POOL_MAX) nBalanceNeedsAnonymized = DARKSEND_POOL_MAX;

    // try to overshoot target DS balance up to nLowestDenom
    nBalanceNeedsAnonymized += nLowestDenom;

    CAmount nAnonymizableBalance = pwalletMain->GetAnonymizableBalance();

    // anonymizable balance is way too small
    if(nAnonymizableBalance < nLowestDenom)
    {
        LogPrintf("DoAutomaticDenominating : No funds detected in need of denominating \n");
        strAutoDenomResult = _("No funds detected in need of denominating.");
        return false;
    }

    // not enough funds to anonymze amount we want, try the max we can
    if(nBalanceNeedsAnonymized > nAnonymizableBalance) nBalanceNeedsAnonymized = nAnonymizableBalance;

    LogPrint("privatesend", "DoAutomaticDenominating : nLowestDenom=%d, nBalanceNeedsAnonymized=%d\n", nLowestDenom, nBalanceNeedsAnonymized);

    // select coins that should be given to the pool
    if (!pwalletMain->SelectCoinsDark(nValueMin, nBalanceNeedsAnonymized, vCoins, nValueIn, 0, nPrivateSendRounds))
    {
        nValueIn = 0;
        vCoins.clear();

        if (pwalletMain->SelectCoinsDark(nValueMin, 9999999*COIN, vCoins, nValueIn, -2, 0))
        {
            nOnlyDenominatedBalance = pwalletMain->GetDenominatedBalance(true) + pwalletMain->GetDenominatedBalance() - pwalletMain->GetAnonymizedBalance();
            nBalanceNeedsDenominated = nBalanceNeedsAnonymized - nOnlyDenominatedBalance;

            if(nBalanceNeedsDenominated > nValueIn) nBalanceNeedsDenominated = nValueIn;

            if(nBalanceNeedsDenominated < nLowestDenom) return false; // most likely we just waiting for denoms to confirm
            if(!fDryRun) return CreateDenominated(nBalanceNeedsDenominated);

            return true;
        } else {
            LogPrintf("DoAutomaticDenominating : Can't denominate - no compatible inputs left\n");
            strAutoDenomResult = _("Can't denominate: no compatible inputs left.");
            return false;
        }

    }

    if(fDryRun) return true;

    nOnlyDenominatedBalance = pwalletMain->GetDenominatedBalance(true) + pwalletMain->GetDenominatedBalance() - pwalletMain->GetAnonymizedBalance();
    nBalanceNeedsDenominated = nBalanceNeedsAnonymized - nOnlyDenominatedBalance;

    //check if we have should create more denominated inputs
    if(nBalanceNeedsDenominated > nOnlyDenominatedBalance) return CreateDenominated(nBalanceNeedsDenominated);

    //check if we have the collateral sized inputs
    if(!pwalletMain->HasCollateralInputs()) return !pwalletMain->HasCollateralInputs(false) && MakeCollateralAmounts();

    std::vector<CTxOut> vOut;

    // initial phase, find a Masternode
    if(!sessionFoundMasternode){
        // Clean if there is anything left from previous session
        UnlockCoins();
        SetNull();

        int nUseQueue = rand()%100;
        UpdateState(POOL_STATUS_ACCEPTING_ENTRIES);

        if(!fPrivateSendMultiSession && pwalletMain->GetDenominatedBalance(true) > 0) { //get denominated unconfirmed inputs
            LogPrintf("DoAutomaticDenominating -- Found unconfirmed denominated outputs, will wait till they confirm to continue.\n");
            strAutoDenomResult = _("Found unconfirmed denominated outputs, will wait till they confirm to continue.");
            return false;
        }

        //check our collateral and create new if needed
        std::string strReason;
        if(txCollateral == CMutableTransaction()){
            if(!pwalletMain->CreateCollateralTransaction(txCollateral, strReason)){
                LogPrintf("% -- create collateral error:%s\n", __func__, strReason);
                return false;
            }
        } else {
            if(!IsCollateralValid(txCollateral)) {
                LogPrintf("%s -- invalid collateral, recreating...\n", __func__);
                if(!pwalletMain->CreateCollateralTransaction(txCollateral, strReason)){
                    LogPrintf("%s -- create collateral error: %s\n", __func__, strReason);
                    return false;
                }
            }
        }

        //if we've used 90% of the Masternode list then drop all the oldest first
        int nThreshold = (int)(mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION) * 0.9);
        LogPrint("privatesend", "Checking vecMasternodesUsed size %d threshold %d\n", (int)vecMasternodesUsed.size(), nThreshold);
        while((int)vecMasternodesUsed.size() > nThreshold){
            vecMasternodesUsed.erase(vecMasternodesUsed.begin());
            LogPrint("privatesend", "  vecMasternodesUsed size %d threshold %d\n", (int)vecMasternodesUsed.size(), nThreshold);
        }

        // don't use the queues all of the time for mixing unless we are a liquidity provider
        if(nLiquidityProvider || nUseQueue > 33){

            // Look through the queues and see if anything matches
            BOOST_FOREACH(CDarksendQueue& dsq, vecDarksendQueue){
                CService addr;
                if(dsq.time == 0) continue;

                if(!dsq.GetAddress(addr)) continue;
                if(dsq.IsExpired()) continue;

                int protocolVersion;
                if(!dsq.GetProtocolVersion(protocolVersion)) continue;
                if(protocolVersion < MIN_POOL_PEER_PROTO_VERSION) continue;

                //non-denom's are incompatible
                if((dsq.nDenom & (1 << 4))) continue;

                bool fUsed = false;
                //don't reuse Masternodes
                BOOST_FOREACH(CTxIn usedVin, vecMasternodesUsed){
                    if(dsq.vin == usedVin) {
                        fUsed = true;
                        break;
                    }
                }
                if(fUsed) continue;

                std::vector<CTxIn> vTempCoins;
                std::vector<COutput> vTempCoins2;
                // Try to match their denominations if possible
                if (!pwalletMain->SelectCoinsByDenominations(dsq.nDenom, nValueMin, nBalanceNeedsAnonymized, vTempCoins, vTempCoins2, nValueIn, 0, nPrivateSendRounds)){
                    LogPrintf("DoAutomaticDenominating --- Couldn't match denominations %d\n", dsq.nDenom);
                    continue;
                }

                CMasternode* pmn = mnodeman.Find(dsq.vin);
                if(pmn == NULL)
                {
                    LogPrintf("DoAutomaticDenominating --- dsq vin %s is not in masternode list!", dsq.vin.ToString());
                    continue;
                }

                LogPrintf("DoAutomaticDenominating --- attempt to connect to masternode from queue %s\n", pmn->addr.ToString());
                lastTimeChanged = GetTimeMillis();
                // connect to Masternode and submit the queue request
                CNode* pnode = ConnectNode((CAddress)addr, NULL, true);
                if(pnode != NULL)
                {
                    pSubmittedToMasternode = pmn;
                    vecMasternodesUsed.push_back(dsq.vin);
                    sessionDenom = dsq.nDenom;

                    pnode->PushMessage(NetMsgType::DSACCEPT, sessionDenom, txCollateral);
                    LogPrintf("DoAutomaticDenominating --- connected (from queue), sending dsa for %d - %s\n", sessionDenom, pnode->addr.ToString());
                    strAutoDenomResult = _("Mixing in progress...");
                    dsq.time = 0; //remove node
                    return true;
                } else {
                    LogPrintf("DoAutomaticDenominating --- error connecting \n");
                    strAutoDenomResult = _("Error connecting to Masternode.");
                    dsq.time = 0; //remove node
                    continue;
                }
            }
        }

        // do not initiate queue if we are a liquidity provider to avoid useless inter-mixing
        if(nLiquidityProvider) return false;

        int i = 0;

        // otherwise, try one randomly
        while(i < 10)
        {
            CMasternode* pmn = mnodeman.FindRandomNotInVec(vecMasternodesUsed, MIN_POOL_PEER_PROTO_VERSION);
            if(pmn == NULL)
            {
                LogPrintf("DoAutomaticDenominating --- Can't find random masternode!\n");
                strAutoDenomResult = _("Can't find random Masternode.");
                return false;
            }

            if(pmn->nLastDsq != 0 &&
                pmn->nLastDsq + mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION)/5 > mnodeman.nDsqCount){
                i++;
                continue;
            }

            lastTimeChanged = GetTimeMillis();
            LogPrintf("DoAutomaticDenominating --- attempt %d connection to Masternode %s\n", i, pmn->addr.ToString());
            CNode* pnode = ConnectNode((CAddress)pmn->addr, NULL, true);
            if(pnode != NULL){
                pSubmittedToMasternode = pmn;
                vecMasternodesUsed.push_back(pmn->vin);

                std::vector<CAmount> vecAmounts;
                pwalletMain->ConvertList(vCoins, vecAmounts);
                // try to get a single random denom out of vecAmounts
                while(sessionDenom == 0)
                    sessionDenom = GetDenominationsByAmounts(vecAmounts);

                pnode->PushMessage(NetMsgType::DSACCEPT, sessionDenom, txCollateral);
                LogPrintf("DoAutomaticDenominating --- connected, sending dsa for %d\n", sessionDenom);
                strAutoDenomResult = _("Mixing in progress...");
                return true;
            } else {
                vecMasternodesUsed.push_back(pmn->vin); // postpone MN we wasn't able to connect to
                i++;
                continue;
            }
        }

        strAutoDenomResult = _("No compatible Masternode found.");
        return false;
    }

    strAutoDenomResult = _("Mixing in progress...");
    return false;
}


bool CDarksendPool::PrepareDarksendDenominate()
{
    std::string strError = "";
    // Submit transaction to the pool if we get here
    // Try to use only inputs with the same number of rounds starting from lowest number of rounds possible
    for(int i = 0; i < nPrivateSendRounds; i++) {
        strError = pwalletMain->PrepareDarksendDenominate(i, i+1);
        LogPrintf("DoAutomaticDenominating : Running PrivateSend denominate for %d rounds. Return '%s'\n", i, strError);
        if(strError == "") return true;
    }

    // We failed? That's strange but let's just make final attempt and try to mix everything
    strError = pwalletMain->PrepareDarksendDenominate(0, nPrivateSendRounds);
    LogPrintf("DoAutomaticDenominating : Running PrivateSend denominate for all rounds. Return '%s'\n", strError);
    if(strError == "") return true;

    // Should never actually get here but just in case
    strAutoDenomResult = strError;
    LogPrintf("DoAutomaticDenominating : Error running denominate, %s\n", strError);
    return false;
}

// Split up large inputs or create fee sized inputs
bool CDarksendPool::MakeCollateralAmounts()
{
    CWalletTx wtx;
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    std::string strFail = "";
    vector< CRecipient > vecSend;
    CCoinControl *coinControl = NULL;

    // make our collateral address
    CReserveKey reservekeyCollateral(pwalletMain);
    // make our change address
    CReserveKey reservekeyChange(pwalletMain);

    CScript scriptCollateral;
    CPubKey vchPubKey;
    assert(reservekeyCollateral.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked
    scriptCollateral = GetScriptForDestination(vchPubKey.GetID());

    vecSend.push_back((CRecipient){scriptCollateral, DARKSEND_COLLATERAL*4, false});

    // try to use non-denominated and not mn-like funds
    bool success = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
            nFeeRet, nChangePosRet, strFail, coinControl, true, ONLY_NONDENOMINATED_NOT1000IFMN);
    if(!success){
        // if we failed (most likeky not enough funds), try to use all coins instead -
        // MN-like funds should not be touched in any case and we can't mix denominated without collaterals anyway
        LogPrintf("MakeCollateralAmounts: ONLY_NONDENOMINATED_NOT1000IFMN Error - %s\n", strFail);
        success = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
                nFeeRet, nChangePosRet, strFail, coinControl, true, ONLY_NOT1000IFMN);
        if(!success){
            LogPrintf("MakeCollateralAmounts: ONLY_NOT1000IFMN Error - %s\n", strFail);
            reservekeyCollateral.ReturnKey();
            return false;
        }
    }

    reservekeyCollateral.KeepKey();

    LogPrintf("MakeCollateralAmounts: tx %s\n", wtx.GetHash().GetHex());

    // use the same cachedLastSuccess as for DS mixinx to prevent race
    if(!pwalletMain->CommitTransaction(wtx, reservekeyChange)) {
        LogPrintf("MakeCollateralAmounts: CommitTransaction failed!\n");
        return false;
    }

    cachedLastSuccess = pCurrentBlockIndex->nHeight;

    return true;
}

// Create denominations
bool CDarksendPool::CreateDenominated(CAmount nTotalValue)
{
    CWalletTx wtx;
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    std::string strFail = "";
    vector< CRecipient > vecSend;
    CAmount nValueLeft = nTotalValue;

    LogPrintf("CreateDenominated0 %d\n", nValueLeft);
    // make our collateral address
    CReserveKey reservekeyCollateral(pwalletMain);
    // make our change address
    CReserveKey reservekeyChange(pwalletMain);
    // make our denom addresses
    CReserveKey reservekeyDenom(pwalletMain);

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

    // try few times - skipping smallest denoms first if there are too much already, if failed - use them
    int nOutputsTotal = 0;
    bool fSkip = true;
    do {

        BOOST_REVERSE_FOREACH(CAmount v, darkSendDenominations){

            if(fSkip) {
                // Note: denoms are skipped if there are already DENOMS_COUNT_MAX of them
                // and there are still larger denoms which can be used for mixing

                // check skipped denoms
                if(IsDenomSkipped(v)) continue;

                // find new denoms to skip if any (ignore the largest one)
                if (v != darkSendDenominations[0] && pwalletMain->CountInputsWithAmount(v) > DENOMS_COUNT_MAX){
                    strAutoDenomResult = strprintf(_("Too many %f denominations, removing."), (float)v/COIN);
                    LogPrintf("DoAutomaticDenominating : %s\n", strAutoDenomResult);
                    darkSendDenominationsSkipped.push_back(v);
                    continue;
                }
            }

            int nOutputs = 0;

            // add each output up to 10 times until it can't be added again
            while(nValueLeft - v >= 0 && nOutputs <= 10) {
                CScript scriptDenom;
                CPubKey vchPubKey;
                //use a unique change address
                assert(reservekeyDenom.GetReservedKey(vchPubKey)); // should never fail, as we just unlocked
                scriptDenom = GetScriptForDestination(vchPubKey.GetID());
                // TODO: do not keep reservekeyDenom here
                reservekeyDenom.KeepKey();

                vecSend.push_back((CRecipient){scriptDenom, v, false});

                //increment outputs and subtract denomination amount
                nOutputs++;
                nValueLeft -= v;
                LogPrintf("CreateDenominated1 %d %d %d\n", nOutputsTotal, nOutputs, nValueLeft);
            }

            nOutputsTotal += nOutputs;
            if(nValueLeft == 0) break;
        }
        LogPrintf("CreateDenominated2 %d %d\n", nOutputsTotal, nValueLeft);
        // if there were no outputs added, start over without skipping
        fSkip = !fSkip;
    } while (nOutputsTotal == 0 && !fSkip);
    LogPrintf("CreateDenominated3 %d %d\n", nOutputsTotal, nValueLeft);

    // if we have anything left over, it will be automatically send back as change - there is no need to send it manually

    CCoinControl *coinControl=NULL;
    bool success = pwalletMain->CreateTransaction(vecSend, wtx, reservekeyChange,
            nFeeRet, nChangePosRet, strFail, coinControl, true, ONLY_NONDENOMINATED_NOT1000IFMN);
    if(!success){
        LogPrintf("CreateDenominated: Error - %s\n", strFail);
        // TODO: return reservekeyDenom here
        reservekeyCollateral.ReturnKey();
        return false;
    }

    // TODO: keep reservekeyDenom here
    reservekeyCollateral.KeepKey();

    // use the same cachedLastSuccess as for DS mixinx to prevent race
    if(pwalletMain->CommitTransaction(wtx, reservekeyChange))
        cachedLastSuccess = pCurrentBlockIndex->nHeight;
    else
        LogPrintf("CreateDenominated: CommitTransaction failed!\n");

    LogPrintf("CreateDenominated: tx %s\n", wtx.GetHash().GetHex());

    return true;
}

bool CDarksendPool::IsCompatibleWithEntries(std::vector<CTxOut>& vout)
{
    if(GetDenominations(vout) == 0) return false;

    BOOST_FOREACH(const CDarkSendEntry v, entries) {
        LogPrintf(" IsCompatibleWithEntries %d %d\n", GetDenominations(vout), GetDenominations(v.vout));
        if(GetDenominations(vout) != GetDenominations(v.vout)) return false;
    }

    return true;
}

bool CDarksendPool::IsCompatibleWithSession(CAmount nDenom, CTransaction txCollateral, int& errorID)
{
    if(nDenom == 0) return false;

    LogPrintf("CDarksendPool::IsCompatibleWithSession - sessionDenom %d sessionUsers %d\n", sessionDenom, sessionUsers);

    if (!unitTest && !IsCollateralValid(txCollateral)){
        LogPrint("privatesend", "CDarksendPool::IsCompatibleWithSession - collateral not valid!\n");
        errorID = ERR_INVALID_COLLATERAL;
        return false;
    }

    if(sessionUsers < 0) sessionUsers = 0;

    if(sessionUsers == 0) {
        sessionID = 1 + (rand() % 999999);
        sessionDenom = nDenom;
        sessionUsers++;
        lastTimeChanged = GetTimeMillis();

        if(!unitTest){
            //broadcast that I'm accepting entries, only if it's the first entry through
            CDarksendQueue dsq;
            dsq.nDenom = nDenom;
            dsq.vin = activeMasternode.vin;
            dsq.time = GetTime();
            dsq.Sign();
            dsq.Relay();
        }

        UpdateState(POOL_STATUS_QUEUE);
        vecSessionCollateral.push_back(txCollateral);
        return true;
    }

    if((state != POOL_STATUS_ACCEPTING_ENTRIES && state != POOL_STATUS_QUEUE) || sessionUsers >= GetMaxPoolTransactions()){
        if((state != POOL_STATUS_ACCEPTING_ENTRIES && state != POOL_STATUS_QUEUE)) errorID = ERR_MODE;
        if(sessionUsers >= GetMaxPoolTransactions()) errorID = ERR_QUEUE_FULL;
        LogPrintf("CDarksendPool::IsCompatibleWithSession - incompatible mode, return false %d %d\n", state != POOL_STATUS_ACCEPTING_ENTRIES, sessionUsers >= GetMaxPoolTransactions());
        return false;
    }

    if(nDenom != sessionDenom) {
        errorID = ERR_DENOM;
        return false;
    }

    LogPrintf("CDarksendPool::IsCompatibleWithSession - compatible\n");

    sessionUsers++;
    lastTimeChanged = GetTimeMillis();
    vecSessionCollateral.push_back(txCollateral);

    return true;
}

//create a nice string to show the denominations
void CDarksendPool::GetDenominationsToString(int nDenom, std::string& strDenom){
    // Function returns as follows:
    //
    // bit 0 - 100DRK+1 ( bit on if present )
    // bit 1 - 10DRK+1
    // bit 2 - 1DRK+1
    // bit 3 - .1DRK+1
    // bit 3 - non-denom


    strDenom = "";

    if(nDenom & (1 << 0)) {
        if(strDenom.size() > 0) strDenom += "+";
        strDenom += "100";
    }

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
}

int CDarksendPool::GetDenominations(const std::vector<CTxDSOut>& vout){
    std::vector<CTxOut> vout2;

    BOOST_FOREACH(CTxDSOut out, vout)
        vout2.push_back(out);

    return GetDenominations(vout2);
}

// return a bitshifted integer representing the denominations in this list
int CDarksendPool::GetDenominations(const std::vector<CTxOut>& vout, bool fSingleRandomDenom){
    std::vector<pair<CAmount, int> > denomUsed;

    // make a list of denominations, with zero uses
    BOOST_FOREACH(CAmount d, darkSendDenominations)
        denomUsed.push_back(make_pair(d, 0));

    // look for denominations and update uses to 1
    BOOST_FOREACH(CTxOut out, vout){
        bool found = false;
        BOOST_FOREACH (PAIRTYPE(CAmount, int)& s, denomUsed){
            if (out.nValue == s.first){
                s.second = 1;
                found = true;
            }
        }
        if(!found) return 0;
    }

    int denom = 0;
    int c = 0;
    // if the denomination is used, shift the bit on.
    // then move to the next
    BOOST_FOREACH (PAIRTYPE(CAmount, int)& s, denomUsed) {
        int bit = (fSingleRandomDenom ? rand()%2 : 1) * s.second;
        denom |= bit << c++;
        if(fSingleRandomDenom && bit) break; // use just one random denomination
    }

    // Function returns as follows:
    //
    // bit 0 - 100DRK+1 ( bit on if present )
    // bit 1 - 10DRK+1
    // bit 2 - 1DRK+1
    // bit 3 - .1DRK+1

    return denom;
}


int CDarksendPool::GetDenominationsByAmounts(std::vector<CAmount>& vecAmount){
    CScript e = CScript();
    std::vector<CTxOut> vout1;

    // Make outputs by looping through denominations, from small to large
    BOOST_REVERSE_FOREACH(CAmount v, vecAmount){
        CTxOut o(v, e);
        vout1.push_back(o);
    }

    return GetDenominations(vout1, true);
}

int CDarksendPool::GetDenominationsByAmount(CAmount nAmount, int nDenomTarget){
    CScript e = CScript();
    CAmount nValueLeft = nAmount;

    std::vector<CTxOut> vout1;

    // Make outputs by looping through denominations, from small to large
    BOOST_REVERSE_FOREACH(CAmount v, darkSendDenominations){
        if(nDenomTarget != 0){
            bool fAccepted = false;
            if((nDenomTarget & (1 << 0)) &&      v == ((100*COIN)+100000)) {fAccepted = true;}
            else if((nDenomTarget & (1 << 1)) && v == ((10*COIN) +10000)) {fAccepted = true;}
            else if((nDenomTarget & (1 << 2)) && v == ((1*COIN)  +1000)) {fAccepted = true;}
            else if((nDenomTarget & (1 << 3)) && v == ((.1*COIN) +100)) {fAccepted = true;}
            if(!fAccepted) continue;
        }

        int nOutputs = 0;

        // add each output up to 10 times until it can't be added again
        while(nValueLeft - v >= 0 && nOutputs <= 10) {
            CTxOut o(v, e);
            vout1.push_back(o);
            nValueLeft -= v;
            nOutputs++;
        }
        LogPrintf("GetDenominationsByAmount --- %d nOutputs %d\n", v, nOutputs);
    }

    return GetDenominations(vout1);
}

std::string CDarksendPool::GetMessageByID(int messageID) {
    switch (messageID) {
    case ERR_ALREADY_HAVE: return _("Already have that input.");
    case ERR_DENOM: return _("No matching denominations found for mixing.");
    case ERR_ENTRIES_FULL: return _("Entries are full.");
    case ERR_EXISTING_TX: return _("Not compatible with existing transactions.");
    case ERR_FEES: return _("Transaction fees are too high.");
    case ERR_INVALID_COLLATERAL: return _("Collateral not valid.");
    case ERR_INVALID_INPUT: return _("Input is not valid.");
    case ERR_INVALID_SCRIPT: return _("Invalid script detected.");
    case ERR_INVALID_TX: return _("Transaction not valid.");
    case ERR_MAXIMUM: return _("Value more than PrivateSend pool maximum allows.");
    case ERR_MN_LIST: return _("Not in the Masternode list.");
    case ERR_MODE: return _("Incompatible mode.");
    case ERR_NON_STANDARD_PUBKEY: return _("Non-standard public key detected.");
    case ERR_NOT_A_MN: return _("This is not a Masternode.");
    case ERR_QUEUE_FULL: return _("Masternode queue is full.");
    case ERR_RECENT: return _("Last PrivateSend was too recent.");
    case ERR_SESSION: return _("Session not complete!");
    case ERR_MISSING_TX: return _("Missing input transaction information.");
    case ERR_VERSION: return _("Incompatible version.");
    case MSG_SUCCESS: return _("Transaction created successfully.");
    case MSG_ENTRIES_ADDED: return _("Your entries added successfully.");
    case MSG_NOERR:
    default:
        return "";
    }
}

bool CDarkSendSigner::IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey){
    CScript payee2;
    payee2 = GetScriptForDestination(pubkey.GetID());

    CTransaction txVin;
    uint256 hash;
    if(GetTransaction(vin.prevout.hash, txVin, Params().GetConsensus(), hash, true)){
        BOOST_FOREACH(CTxOut out, txVin.vout){
            if(out.nValue == 1000*COIN){
                if(out.scriptPubKey == payee2) return true;
            }
        }
    }

    return false;
}

bool CDarkSendSigner::SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey){
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) {
        errorMessage = _("Invalid private key.");
        return false;
    }

    key = vchSecret.GetKey();
    pubkey = key.GetPubKey();

    return true;
}

bool CDarkSendSigner::SignMessage(std::string strMessage, std::string& errorMessage, vector<unsigned char>& vchSig, CKey key)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    if (!key.SignCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Signing failed.");
        return false;
    }

    return true;
}

bool CDarkSendSigner::VerifyMessage(CPubKey pubkey, vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey2;
    if (!pubkey2.RecoverCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Error recovering public key.");
        return false;
    }

    if (fDebug && pubkey2.GetID() != pubkey.GetID())
        LogPrintf("CDarkSendSigner::VerifyMessage -- keys don't match: %s %s\n", pubkey2.GetID().ToString(), pubkey.GetID().ToString());

    return (pubkey2.GetID() == pubkey.GetID());
}

bool CDarksendQueue::Sign()
{
    if(!fMasterNode) return false;

    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nDenom) + boost::lexical_cast<std::string>(time) + boost::lexical_cast<std::string>(ready);

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CDarksendQueue():Relay - ERROR: Invalid Masternodeprivkey: '%s'\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, key2)) {
        LogPrintf("CDarksendQueue():Relay - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CDarksendQueue():Relay - Verify message failed");
        return false;
    }

    return true;
}

bool CDarksendQueue::Relay()
{

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        // always relay to everyone
        pnode->PushMessage(NetMsgType::DSQUEUE, (*this));
    }

    return true;
}

bool CDarksendQueue::CheckSignature()
{
    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn != NULL)
    {
        std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nDenom) + boost::lexical_cast<std::string>(time) + boost::lexical_cast<std::string>(ready);

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CDarksendQueue::CheckSignature() - Got bad Masternode queue signature %s", vin.ToString().c_str());
        }

        return true;
    }

    return false;
}


void CDarksendPool::RelayFinalTransaction(const int sessionID, const CTransaction& txNew)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        pnode->PushMessage(NetMsgType::DSFINALTX, sessionID, txNew);
    }
}

void CDarksendPool::RelayIn(const std::vector<CTxDSIn>& vin, const CAmount& nAmount, const CTransaction& txCollateral, const std::vector<CTxDSOut>& vout)
{
    if(!pSubmittedToMasternode) return;

    std::vector<CTxIn> vin2;
    std::vector<CTxOut> vout2;

    BOOST_FOREACH(CTxDSIn in, vin)
        vin2.push_back(in);

    BOOST_FOREACH(CTxDSOut out, vout)
        vout2.push_back(out);

    CNode* pnode = FindNode(pSubmittedToMasternode->addr);
    if(pnode != NULL) {
        LogPrintf("RelayIn - found master, relaying message - %s \n", pnode->addr.ToString());
        pnode->PushMessage(NetMsgType::DSVIN, vin2, nAmount, txCollateral, vout2);
    }
}

void CDarksendPool::RelayStatus(const int sessionID, const int newState, const int newEntriesCount, const int newAccepted, const int errorID)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSSTATUSUPDATE, sessionID, newState, newEntriesCount, newAccepted, errorID);
}

void CDarksendPool::RelayCompletedTransaction(const int sessionID, const bool error, const int errorID)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage(NetMsgType::DSCOMPLETE, sessionID, error, errorID);
}

void CDarksendPool::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("privatesend", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

//TODO: Rename/move to core
void ThreadCheckDarkSendPool()
{
    if(fLiteMode) return; //disable all Darksend/Masternode related functionality

    static bool fOneThread;
    if (fOneThread)
        return;
    fOneThread = true;

    // Make this thread recognisable as the Darksend/Masternode thread
    RenameThread("dash-darksend");

    unsigned int c = 0;

    while (true)
    {
        MilliSleep(1000);
        //LogPrintf("ThreadCheckDarkSendPool::check timeout\n");

        // try to sync from all available nodes, one step at a time
        masternodeSync.Process();

        if(masternodeSync.IsBlockchainSynced()) {

            c++;

            // check if we should activate or ping every few minutes,
            // start right after sync is considered to be done
            if(c % MASTERNODE_PING_SECONDS == 1) activeMasternode.ManageStatus();

            if(c % 60 == 0)
            {
                mnodeman.CheckAndRemove();
                mnodeman.ProcessMasternodeConnections();
                mnpayments.CheckAndRemove();
                CleanTransactionLocksList();
            }

            darkSendPool.CheckTimeout();
            darkSendPool.CheckForCompleteQueue();

            if(darkSendPool.GetState() == POOL_STATUS_IDLE && c % 15 == 0){
                darkSendPool.DoAutomaticDenominating();
            }
        }
    }
}
