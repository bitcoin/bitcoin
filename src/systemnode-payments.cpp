// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "systemnode-payments.h"
#include "systemnode-sync.h"
#include "systemnodeman.h"
#include "legacysigner.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "masternode-budget.h"

/** Object for who's going to get paid on which blocks */
CSystemnodePayments systemnodePayments;

CCriticalSection cs_vecSNPayments;
CCriticalSection cs_mapSystemnodeBlocks;
CCriticalSection cs_mapSystemnodePayeeVotes;

bool SNIsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight)
{
    if(!systemnodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint("snpayments", "Client not synced, skipping block payee checks\n");
        return true;
    }

    //check if it's a budget block
    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
        if(budget.IsBudgetPaymentBlock(nBlockHeight)){
            if(budget.IsTransactionValid(txNew, nBlockHeight)){
                return true;
            } else {
                LogPrintf("Invalid budget payment detected %s\n", txNew.ToString().c_str());
                if(IsSporkActive(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)){
                    return false;
                } else {
                    LogPrintf("Budget enforcement is disabled, accepting block\n");
                    return true;
                }
            }
        }
    }

    //check for systemnode payee
    if(systemnodePayments.IsTransactionValid(txNew, nBlockHeight))
    {
        return true;
    } else {
        LogPrintf("Invalid mn payment detected %s\n", txNew.ToString().c_str());
        if(IsSporkActive(SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)){
            return false;
        } else {
            LogPrintf("Systemnode payment enforcement is disabled, accepting block\n");
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

int CSystemnodePayments::GetMinSystemnodePaymentsProto() const
{
    return IsSporkActive(SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES)
            ? MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_CURR
            : MIN_SYSTEMNODE_PAYMENT_PROTO_VERSION_PREV;
}

void CSystemnodePayments::ProcessMessageSystemnodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!systemnodeSync.IsBlockchainSynced()) return;

    if(fLiteMode) return; //disable all Systemnode related functionality


    if (strCommand == "snget") { //Systemnode Payments Request Sync
        if(fLiteMode) return; //disable all Systemnode related functionality

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if(Params().NetworkID() == CBaseChainParams::MAIN){
            if(pfrom->HasFulfilledRequest("snget")) {
                LogPrintf("snget - peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest("snget");
        systemnodePayments.Sync(pfrom, nCountNeeded);
        LogPrintf("snget - Sent Systemnode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "snw") { //Systemnode Payments Declare Winner
        //this is required in litemodef
        CSystemnodePaymentWinner winner;
        vRecv >> winner;

        if(pfrom->nVersion < MIN_MNW_PEER_PROTO_VERSION) return;

        int nHeight;
        {
            TRY_LOCK(cs_main, locked);
            if(!locked || chainActive.Tip() == NULL) return;
            nHeight = chainActive.Tip()->nHeight;
        }

        if(systemnodePayments.mapSystemnodePayeeVotes.count(winner.GetHash())){
            LogPrint("snpayments", "snw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (snodeman.CountEnabled()*1.25);
        if(winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight+20){
            LogPrint("snpayments", "snw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        std::string strError = "";
        if(!winner.IsValid(pfrom, strError)){
            if(strError != "") LogPrintf("snw - invalid message - %s\n", strError);
            return;
        }

        if(!systemnodePayments.CanVote(winner.vinSystemnode.prevout, winner.nBlockHeight)){
            LogPrintf("snw - systemnode already voted - %s\n", winner.vinSystemnode.prevout.ToStringShort());
            return;
        }

        if(!winner.SignatureValid()){
            LogPrintf("snw - invalid signature\n");
            if(systemnodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced systemnode
            snodeman.AskForSN(pfrom, winner.vinSystemnode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint("snpayments", "snw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString().c_str(), winner.nBlockHeight, nHeight, winner.vinSystemnode.prevout.ToStringShort());

        if(systemnodePayments.AddWinningSystemnode(winner)){
            winner.Relay();
            systemnodeSync.AddedSystemnodeWinner(winner.GetHash());
        }
    }
}

bool CSystemnodePayments::CanVote(COutPoint outSystemnode, int nBlockHeight)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    if (mapSystemnodesLastVote.count(outSystemnode) && mapSystemnodesLastVote[outSystemnode] == nBlockHeight) {
        return false;
    }

    //record this masternode voted
    mapSystemnodesLastVote[outSystemnode] = nBlockHeight;
    return true;
}

void SNFillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(pindexPrev->nHeight+1)){
        // Doesn't pay systemnode, miners get the full amount on these blocks
    } else {
        systemnodePayments.FillBlockPayee(txNew, nFees);
    }
}

std::string SNGetRequiredPaymentsString(int nBlockHeight)
{
    if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(nBlockHeight)){
        return budget.GetRequiredPaymentsString(nBlockHeight);
    } else {
        return systemnodePayments.GetRequiredPaymentsString(nBlockHeight);
    }
}

void CSystemnodePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if(!systemnodePayments.GetBlockPayee(pindexPrev->nHeight+1, payee)){
        //no systemnode detected
        CSystemnode* winningNode = snodeman.GetCurrentSystemNode(1);
        if(winningNode) {
            payee = GetScriptForDestination(winningNode->pubkey.GetID());
        } else {
            LogPrintf("CreateNewBlock: Failed to detect systemnode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = GetBlockValue(pindexPrev->nHeight, nFees);
    CAmount systemnodePayment = GetSystemnodePayment(pindexPrev->nHeight+1, blockValue);

    // This is already done in masternode
    //txNew.vout[0].nValue = blockValue;

    if(hasPayment) {
        txNew.vout.resize(3);

        // [0] is for miner, [1] masternode, [2] systemnode
        txNew.vout[2].scriptPubKey = payee;
        txNew.vout[2].nValue = systemnodePayment;

        txNew.vout[0].nValue -= systemnodePayment;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Systemnode payment to %s\n", address2.ToString().c_str());
    }
}

std::string CSystemnodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    BOOST_FOREACH(CSystemnodePayee& payee, vecPayments)
    {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if(ret != "Unknown"){
            ret += ", " + address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        } else {
            ret = address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        }
    }

    return ret;
}

std::string CSystemnodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CSystemnodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount systemnodePayment = GetSystemnodePayment(nBlockHeight, txNew.GetValueOut());

    //require at least 6 signatures

    BOOST_FOREACH(CSystemnodePayee& payee, vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < SNPAYMENTS_SIGNATURES_REQUIRED) return true;

    BOOST_FOREACH(CSystemnodePayee& payee, vecPayments)
    {
        bool found = false;
        int pos = -1;
        for (unsigned int i = 0; i < txNew.vout.size(); i++){
            if(payee.scriptPubKey == txNew.vout[i].scriptPubKey && systemnodePayment == txNew.vout[i].nValue){
                pos = i;
                found = true;
                break;
            }
        }

        if(payee.nVotes >= SNPAYMENTS_SIGNATURES_REQUIRED){
            if(found) {
                //When proof of stake is active, enforce specific payment positions
                if (nBlockHeight >= Params().PoSStartHeight() && pos != SN_PMT_SLOT)
                    return error("%s: Systemnode payment is not in coinbase.vout[%d]", __func__, SN_PMT_SLOT);
                return true;
            }

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if(strPayeesPossible == ""){
                strPayeesPossible += address2.ToString();
            } else {
                strPayeesPossible += "," + address2.ToString();
            }
        }
    }


    LogPrintf("CSystemnodePayments::IsTransactionValid - Missing required payment - %s\n", strPayeesPossible.c_str());
    return false;
}

bool CSystemnodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

void CSystemnodePayments::CheckAndRemove()
{
    LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(snodeman.size()*1.25), 1000);

    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while(it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;

        if(nHeight - winner.nBlockHeight > nLimit){
            LogPrint("snpayments", "CSystemnodePayments::CleanPaymentList - Removing old Systemnode payment - block %d\n", winner.nBlockHeight);
            systemnodeSync.mapSeenSyncSNW.erase((*it).first);
            mapSystemnodePayeeVotes.erase(it++);
            mapSystemnodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool CSystemnodePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    if(IsReferenceNode(vinSystemnode)) return true;

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if(!psn)
    {
        strError = strprintf("Unknown Systemnode %s", vinSystemnode.prevout.ToStringShort());
        LogPrintf ("CSystemnodePaymentWinner::IsValid - %s\n", strError);
        snodeman.AskForSN(pnode, vinSystemnode);
        return false;
    }

    if(psn->protocolVersion < MIN_MNW_PEER_PROTO_VERSION)
    {
        strError = strprintf("Systemnode protocol too old %d - req %d", psn->protocolVersion, MIN_MNW_PEER_PROTO_VERSION);
        LogPrintf ("CSystemnodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = snodeman.GetSystemnodeRank(vinSystemnode, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)
    {    
        //It's common to have systemnodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if(n > MNPAYMENTS_SIGNATURES_TOTAL*2)
        {
            strError = strprintf("Systemnode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, n);
            LogPrintf("CSystemnodePaymentWinner::IsValid - %s\n", strError);
            if(systemnodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CSystemnodePayments::ProcessBlock(int nBlockHeight)
{
    if(!fSystemNode) return false;

    //reference node - hybrid mode

    if(!IsReferenceNode(activeSystemnode.vin)) {
        int n = snodeman.GetSystemnodeRank(activeSystemnode.vin, nBlockHeight - 100, MIN_MNW_PEER_PROTO_VERSION);

        if(n == -1)
        {
            LogPrint("snpayments", "CSystemnodePayments::ProcessBlock - Unknown Systemnode\n");
            return false;
        }

        if(n > SNPAYMENTS_SIGNATURES_TOTAL)
        {
            LogPrint("snpayments", "CSystemnodePayments::ProcessBlock - Systemnode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
            return false;
        }
    }

    if(nBlockHeight <= nLastBlockHeight) return false;

    CSystemnodePaymentWinner newWinner(activeSystemnode.vin);

    LogPrintf("CSystemnodePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeSystemnode.vin.ToString().c_str());
    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    CSystemnode *psn = snodeman.GetNextSystemnodeInQueueForPayment(nBlockHeight, true, nCount);

    if(psn != NULL)
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() Found by FindOldestNotInVec \n");

        newWinner.nBlockHeight = nBlockHeight;

        CScript payee = GetScriptForDestination(psn->pubkey.GetID());
        newWinner.AddPayee(payee);

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CSystemnodePayments::ProcessBlock() Winner payee %s nHeight %d. \n", address2.ToString().c_str(), newWinner.nBlockHeight);
    } else {
        LogPrintf("CSystemnodePayments::ProcessBlock() Failed to find systemnode to pay\n");
    }

    std::string errorMessage;
    CPubKey pubKeySystemnode;
    CKey keySystemnode;

    if(!legacySigner.SetKey(strSystemNodePrivKey, errorMessage, keySystemnode, pubKeySystemnode))
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    LogPrintf("CSystemnodePayments::ProcessBlock() - Signing Winner\n");
    if(newWinner.Sign(keySystemnode, pubKeySystemnode))
    {
        LogPrintf("CSystemnodePayments::ProcessBlock() - AddWinningSystemnode\n");

        if(AddWinningSystemnode(newWinner))
        {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

bool CSystemnodePayments::AddWinningSystemnode(CSystemnodePaymentWinner& winnerIn)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-100)) {
        return false;
    }

    {
        LOCK2(cs_mapSystemnodePayeeVotes, cs_mapSystemnodeBlocks);
    
        if(mapSystemnodePayeeVotes.count(winnerIn.GetHash())){
           return false;
        }

        mapSystemnodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if(!mapSystemnodeBlocks.count(winnerIn.nBlockHeight)){
           CSystemnodeBlockPayees blockPayees(winnerIn.nBlockHeight);
           mapSystemnodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    int n = 1;
    if(IsReferenceNode(winnerIn.vinSystemnode)) n = 100;
    mapSystemnodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, n);

    return true;
}

void CSystemnodePaymentWinner::Relay()
{
    CInv inv(MSG_SYSTEMNODE_WINNER, GetHash());
    RelayInv(inv);
}

bool CSystemnodePaymentWinner::SignatureValid()
{

    CSystemnode* psn = snodeman.Find(vinSystemnode);

    if(psn != NULL)
    {
        std::string strMessage =  vinSystemnode.prevout.ToStringShort() +
                    boost::lexical_cast<std::string>(nBlockHeight) +
                    payee.ToString();

        std::string errorMessage = "";
        if(!legacySigner.VerifyMessage(psn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CSystemnodePaymentWinner::SignatureValid() - Got bad Systemnode address signature %s \n", vinSystemnode.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CSystemnodePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapSystemnodePayeeVotes);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    int nCount = (snodeman.CountEnabled()*1.25);
    if(nCountNeeded > nCount) nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CSystemnodePaymentWinner>::iterator it = mapSystemnodePayeeVotes.begin();
    while(it != mapSystemnodePayeeVotes.end()) {
        CSystemnodePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= nHeight-nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
            node->PushInventory(CInv(MSG_SYSTEMNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }
    node->PushMessage("snssc", SYSTEMNODE_SYNC_SNW, nInvCount);
}

// Is this systemnode scheduled to get paid soon? 
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CSystemnodePayments::IsScheduled(CSystemnode& sn, int nNotBlockHeight)
{
    LOCK(cs_mapSystemnodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if(!locked || chainActive.Tip() == NULL) return false;
        nHeight = chainActive.Tip()->nHeight;
    }

    CScript snpayee;
    snpayee = GetScriptForDestination(sn.pubkey.GetID());

    CScript payee;
    for(int64_t h = nHeight; h <= nHeight+8; h++){
        if(h == nNotBlockHeight) continue;
        if(mapSystemnodeBlocks.count(h)){
            if(mapSystemnodeBlocks[h].GetPayee(payee)){
                if(snpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::string CSystemnodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapSystemnodePayeeVotes.size() <<
            ", Blocks: " << (int)mapSystemnodeBlocks.size();

    return info.str();
}

bool CSystemnodePaymentWinner::Sign(CKey& keySystemnode, CPubKey& pubKeySystemnode)
{
    std::string errorMessage;
    std::string strSystemNodeSignMessage;

    std::string strMessage =  vinSystemnode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                payee.ToString();

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keySystemnode)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}
