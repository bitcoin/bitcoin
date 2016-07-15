// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
#include "governance.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Object for who's going to get paid on which blocks */
CMasternodePayments mnpayments;

CCriticalSection cs_vecPayments;
CCriticalSection cs_mapMasternodeBlocks;
CCriticalSection cs_mapMasternodePayeeVotes;

bool IsBlockValueValid(const CBlock& block, CAmount nExpectedValue){
    int nHeight = 0;

    {
        LOCK(cs_main);
        if(!chainActive.Tip()) return true;

        if(chainActive.Tip()->GetBlockHash() == block.hashPrevBlock)
        {
            nHeight = chainActive.Tip()->nHeight+1;
        } else { //out of order
            BlockMap::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
            if (mi != mapBlockIndex.end() && (*mi).second)
                nHeight = (*mi).second->nHeight+1;
        }
    }
    if(nHeight == 0){
        LogPrintf("IsBlockValueValid() : WARNING: Couldn't find previous block");
    }

    if(!masternodeSync.IsSynced()) { //there is no budget data to use to check anything
        //super blocks will always be on these blocks, max Params().GetConsensus().nBudgetPaymentsWindowBlocks per budgeting
        if(nHeight >= Params().GetConsensus().nBudgetPaymentsStartBlock &&
            nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks < Params().GetConsensus().nBudgetPaymentsWindowBlocks){
            return true;
        } else {
            if(block.vtx[0].GetValueOut() > nExpectedValue) return false;
        }
    } else { // we're synced and have data so check the budget schedule

        //are these blocks even enabled
        if(!IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
            return block.vtx[0].GetValueOut() <= nExpectedValue;
        }
        
        // 12.1
        // if(nHeight >= Params().GetConsensus().nBudgetPaymentsStartBlock &&
        //     budget.IsBudgetPaymentBlock(nHeight)){
        //     //the value of the block is evaluated in CheckBlock
        //     return true;
        // } else {
        //     if(block.vtx[0].GetValueOut() > nExpectedValue) return false;
        // }
    }

    return true;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight)
{
    if(!masternodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint("mnpayments", "Client not synced, skipping block payee checks\n");
        return true;
    }

    //check if it's a budget block
    // 12.1 
    // if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)){
    //     if(budget.IsBudgetPaymentBlock(nBlockHeight)){
    //         if(budget.IsTransactionValid(txNew, nBlockHeight)){
    //             return true;
    //         } else {
    //             LogPrintf("Invalid budget payment detected %s\n", txNew.ToString());
    //             if(IsSporkActive(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT)){
    //                 return false;
    //             } else {
    //                 LogPrintf("Budget enforcement is disabled, accepting block\n");
    //                 return true;
    //             }
    //         }
    //     }
    // }

    //check for masternode payee
    if(mnpayments.IsTransactionValid(txNew, nBlockHeight))
    {
        return true;
    } else {
        LogPrintf("Invalid mn payment detected %s\n", txNew.ToString());
        if(IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)){
            return false;
        } else {
            LogPrintf("Masternode payment enforcement is disabled, accepting block\n");
            return true;
        }
    }

    return false;
}


void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees)
{
    AssertLockHeld(cs_main);
    if(!chainActive.Tip()) return;

    // 12.1
    // if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(chainActive.Tip()->nHeight+1)){
    //     budget.FillBlockPayee(txNew, nFees);
    // } else {
    //     mnpayments.FillBlockPayee(txNew, nFees);
    // }
    mnpayments.FillBlockPayee(txNew, nFees);
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    // 12.1 -- added triggered payments

    // if(IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS) && budget.IsBudgetPaymentBlock(nBlockHeight)){
    //     return budget.GetRequiredPaymentsString(nBlockHeight);
    // } else {
    //     return mnpayments.GetRequiredPaymentsString(nBlockHeight);
    // }

    return mnpayments.GetRequiredPaymentsString(nBlockHeight);
}

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, CAmount nFees)
{
    AssertLockHeld(cs_main);
    if(!chainActive.Tip()) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if(!mnpayments.GetBlockPayee(chainActive.Tip()->nHeight+1, payee)){
        //no masternode detected
        CMasternode* winningNode = mnodeman.GetCurrentMasterNode();
        if(winningNode){
            payee = GetScriptForDestination(winningNode->pubkey.GetID());
        } else {
            LogPrintf("CreateNewBlock: Failed to detect masternode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = nFees + GetBlockSubsidy(chainActive.Tip()->nBits, chainActive.Tip()->nHeight, Params().GetConsensus());
    CAmount masternodePayment = GetMasternodePayment(chainActive.Tip()->nHeight+1, blockValue);

    txNew.vout[0].nValue = blockValue;

    if(hasPayment){
        txNew.vout.resize(2);

        txNew.vout[1].scriptPubKey = payee;
        txNew.vout[1].nValue = masternodePayment;

        txNew.vout[0].nValue -= masternodePayment;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Masternode payment %d to %s\n", masternodePayment, address2.ToString());
    }
}

int CMasternodePayments::GetMinMasternodePaymentsProto() {
    return IsSporkActive(SPORK_10_MASTERNODE_PAY_UPDATED_NODES)
            ? MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2
            : MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
}

void CMasternodePayments::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!masternodeSync.IsBlockchainSynced()) return;

    if(fLiteMode) return; //disable all Darksend/Masternode related functionality


    if (strCommand == NetMsgType::MNWINNERSSYNC) { //Masternode Payments Request Sync
        if(fLiteMode) return; //disable all Darksend/Masternode related functionality

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(pfrom->HasFulfilledRequest(NetMsgType::MNWINNERSSYNC)) {
                LogPrintf("mnget - peer already asked me for the list\n");
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest(NetMsgType::MNWINNERSSYNC);
        Sync(pfrom, nCountNeeded);
        LogPrintf("mnget - Sent Masternode winners to %s\n", pfrom->addr.ToString());
    }
    else if (strCommand == NetMsgType::MNWINNER) { //Masternode Payments Declare Winner
        //this is required in litemodef
        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if(pfrom->nVersion < MIN_MNW_PEER_PROTO_VERSION) return;

        if(!pCurrentBlockIndex) return;

        if(mapMasternodePayeeVotes.count(winner.GetHash())){
            LogPrint("mnpayments", "mnw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString(), pCurrentBlockIndex->nHeight);
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = pCurrentBlockIndex->nHeight - (mnodeman.CountEnabled()*1.25);
        if(winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > pCurrentBlockIndex->nHeight+20){
            LogPrint("mnpayments", "mnw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, pCurrentBlockIndex->nHeight);
            return;
        }

        std::string strError = "";
        if(!winner.IsValid(pfrom, strError)){
            if(strError != "") LogPrint("mnpayments", "mnw - invalid message - %s\n", strError);
            return;
        }

        if(!CanVote(winner.vinMasternode.prevout, winner.nBlockHeight)){
            LogPrintf("mnw - masternode already voted - %s\n", winner.vinMasternode.prevout.ToStringShort());
            return;
        }

        if(!winner.SignatureValid()){
            LogPrintf("mnw - invalid signature\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, winner.vinMasternode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint("mnpayments", "mnw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString(), winner.nBlockHeight, pCurrentBlockIndex->nHeight, winner.vinMasternode.prevout.ToStringShort());

        if(AddWinningMasternode(winner)){
            winner.Relay();
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
        }
    }
}

bool CMasternodePaymentWinner::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    std::string strMessage =  vinMasternode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                ScriptToAsmStr(payee);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

// Is this masternode scheduled to get paid soon? 
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CMasternodePayments::IsScheduled(CMasternode& mn, int nNotBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(!pCurrentBlockIndex) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mn.pubkey.GetID());

    CScript payee;
    for(int64_t h = pCurrentBlockIndex->nHeight; h <= pCurrentBlockIndex->nHeight + 8; h++){
        if(h == nNotBlockHeight) continue;
        if(mapMasternodeBlocks.count(h)){
            if(mapMasternodeBlocks[h].GetPayee(payee)){
                if(mnpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-100)) {
        return false;
    }

    {
        LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);
    
        if(mapMasternodePayeeVotes.count(winnerIn.GetHash())){
           return false;
        }

        mapMasternodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if(!mapMasternodeBlocks.count(winnerIn.nBlockHeight)){
           CMasternodeBlockPayees blockPayees(winnerIn.nBlockHeight);
           mapMasternodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    mapMasternodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, 1);

    return true;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount masternodePayment = GetMasternodePayment(nBlockHeight, txNew.GetValueOut());

    //require at least MNPAYMENTS_SIGNATURES_REQUIRED signatures

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least MNPAYMENTS_SIGNATURES_REQUIRED signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut out, txNew.vout){
            if(payee.scriptPubKey == out.scriptPubKey && masternodePayment == out.nValue){
                found = true;
            }
        }

        if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED){
            if(found) return true;

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


    LogPrintf("CMasternodePayments::IsTransactionValid - Missing required payment - %s %d\n", strPayeesPossible, masternodePayment);
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
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

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CMasternodePayments::CheckAndRemove()
{
    if(!pCurrentBlockIndex) return;

    LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

    // keep a bit more for historical sake but at least minBlocksToStore
    int nLimit = std::max(int(mnodeman.size() * nStorageCoeff), nMinBlocksToStore);

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;

        if(pCurrentBlockIndex->nHeight - winner.nBlockHeight > nLimit){
            LogPrint("mnpayments", "CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", winner.nBlockHeight);
            masternodeSync.mapSeenSyncMNW.erase((*it).first);
            mapMasternodePayeeVotes.erase(it++);
            mapMasternodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
    LogPrintf("CMasternodePayments::CleanPaymentList() - %s mapSeenSyncMNW %lld\n", ToString(), masternodeSync.mapSeenSyncMNW.size());
}

bool CMasternodePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(!pmn)
    {
        strError = strprintf("Unknown Masternode %s", vinMasternode.prevout.ToStringShort());
        LogPrint("mnpayments", "CMasternodePaymentWinner::IsValid - %s\n", strError);
        mnodeman.AskForMN(pnode, vinMasternode);
        return false;
    }

    if(pmn->protocolVersion < MIN_MNW_PEER_PROTO_VERSION)
    {
        strError = strprintf("Masternode protocol too old %d - req %d", pmn->protocolVersion, MIN_MNW_PEER_PROTO_VERSION);
        LogPrint("mnpayments", "CMasternodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)
    {    
        //It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if(n > MNPAYMENTS_SIGNATURES_TOTAL*2)
        {
            strError = strprintf("Masternode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, n);
            LogPrint("mnpayments", "CMasternodePaymentWinner::IsValid - %s\n", strError);
            if(masternodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    // DETERMINE IF WE SHOULD BE VOTING FOR THE NEXT PAYEE

    if(!fMasterNode) return false;
    
    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight-100, MIN_MNW_PEER_PROTO_VERSION);        
       
    if(n == -1)       
    {     
        LogPrint("mnpayments", "CMasternodePayments::ProcessBlock - Unknown Masternode\n");       
        return false;     
    }     

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)       
    {     
        LogPrint("mnpayments", "CMasternodePayments::ProcessBlock - Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);        
        return false;     
    }      


    // LOCATE THE NEXT MASTERNODE WHICH SHOULD BE PAID

    CMasternodePaymentWinner newWinner(activeMasternode.vin);
    {
        LogPrintf("CMasternodePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeMasternode.vin.ToString());

        // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
        int nCount = 0;
        CMasternode *pmn = mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount);
        
        if(pmn != NULL)
        {
            LogPrintf("CMasternodePayments::ProcessBlock() Found by FindOldestNotInVec \n");

            newWinner.nBlockHeight = nBlockHeight;

            CScript payee = GetScriptForDestination(pmn->pubkey.GetID());
            newWinner.AddPayee(payee);

            CTxDestination address1;
            ExtractDestination(payee, address1);
            CBitcoinAddress address2(address1);

            LogPrintf("CMasternodePayments::ProcessBlock() Winner payee %s nHeight %d. \n", address2.ToString(), newWinner.nBlockHeight);
        } else {
            LogPrintf("CMasternodePayments::ProcessBlock() Failed to find masternode to pay\n");
        }
    }

    // SIGN MESSAGE TO NETWORK WITH OUR MASTERNODE KEYS

    std::string errorMessage;

    LogPrintf("CMasternodePayments::ProcessBlock() - Signing Winner\n");
    if(newWinner.Sign(activeMasternode.keyMasternode, activeMasternode.pubKeyMasternode))
    {
        LogPrintf("CMasternodePayments::ProcessBlock() - AddWinningMasternode\n");

        if(AddWinningMasternode(newWinner))
        {
            newWinner.Relay();
            return true;
        }
    }

    return false;
}

void CMasternodePaymentWinner::Relay()
{
    CInv inv(MSG_MASTERNODE_WINNER, GetHash());
    RelayInv(inv);
}

bool CMasternodePaymentWinner::SignatureValid()
{

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(pmn != NULL)
    {
        std::string strMessage =  vinMasternode.prevout.ToStringShort() +
                    boost::lexical_cast<std::string>(nBlockHeight) +
                    ScriptToAsmStr(payee);

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CMasternodePaymentWinner::SignatureValid() - Got bad Masternode payment signature %s", vinMasternode.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CMasternodePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapMasternodePayeeVotes);

    if(!pCurrentBlockIndex) return;

    int nCount = (mnodeman.CountEnabled()*1.25);
    if(nCountNeeded > nCount) nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= pCurrentBlockIndex->nHeight - nCountNeeded && winner.nBlockHeight <= pCurrentBlockIndex->nHeight + 20) {
            node->PushInventory(CInv(MSG_MASTERNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }
    node->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_MNW, nInvCount);
}

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapMasternodePayeeVotes.size() <<
            ", Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}



int CMasternodePayments::GetOldestBlock()
{
    LOCK(cs_mapMasternodeBlocks);

    int nOldestBlock = std::numeric_limits<int>::max();

    std::map<int, CMasternodeBlockPayees>::iterator it = mapMasternodeBlocks.begin();
    while(it != mapMasternodeBlocks.end()) {
        if((*it).first < nOldestBlock) {
            nOldestBlock = (*it).first;
        }
        it++;
    }

    return nOldestBlock;
}



int CMasternodePayments::GetNewestBlock()
{
    LOCK(cs_mapMasternodeBlocks);

    int nNewestBlock = 0;

    std::map<int, CMasternodeBlockPayees>::iterator it = mapMasternodeBlocks.begin();
    while(it != mapMasternodeBlocks.end()) {
        if((*it).first > nNewestBlock) {
            nNewestBlock = (*it).first;
        }
        it++;
    }

    return nNewestBlock;
}

bool CMasternodePayments::IsEnoughData(int nMnCount) {
    if(GetBlockCount() > nMnCount * nStorageCoeff && GetBlockCount() > nMinBlocksToStore)
    {
        float nAverageVotes = (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED) / 2;
        if(GetVoteCount() > nMnCount * nStorageCoeff * nAverageVotes && GetVoteCount() > nMinBlocksToStore * nAverageVotes)
        {
            return true;
        }
    }
    return false;
}

void CMasternodePayments::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mnpayments", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        ProcessBlock(pindex->nHeight+10);
}
