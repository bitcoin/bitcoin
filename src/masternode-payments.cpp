// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
#include "masternode-budget.h"
#include "masternodeman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

CCriticalSection cs_masternodepayments;

/** Object for who's going to get paid on which blocks */
CMasternodePayments masternodePayments;
map<uint256, CMasternodePaymentWinner> mapMasternodePayeeVotes;
map<uint256, CMasternodeBlockPayees> mapMasternodeBlocks;

bool IsBlockPayeeValid(const CTransaction& txNew, int64_t nBlockHeight)
{
    //check if it's a budget block
    if(budget.IsBudgetPaymentBlock(nBlockHeight)){
        if(budget.IsTransactionValid(txNew, nBlockHeight)){
            return true;
        }
    }

    //check for masternode payee
    if(masternodePayments.IsTransactionValid(txNew, nBlockHeight))
    {
        return true;
    }

    return false;
}

std::string GetRequiredPaymentsString(int64_t nBlockHeight)
{
    if(budget.IsBudgetPaymentBlock(nBlockHeight)){
        return budget.GetRequiredPaymentsString(nBlockHeight);
    } else {
        return masternodePayments.GetRequiredPaymentsString(nBlockHeight);
    }
}

void CMasternodePayments::ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnget") { //Masternode Payments Request Sync
        if(fLiteMode) return; //disable all Darksend/Masternode related functionality

        if(pfrom->HasFulfilledRequest("mnget")) {
            LogPrintf("mnget - peer already asked me for the list\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        pfrom->FulfilledRequest("mnget");
        masternodePayments.Sync(pfrom);
        LogPrintf("mnget - Sent Masternode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "mnw") { //Masternode Payments Declare Winner
        LOCK(cs_masternodepayments);

        //this is required in litemode
        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if(chainActive.Tip() == NULL) return;

        if(mapMasternodePayeeVotes.count(winner.GetHash())){
           if(fDebug) LogPrintf("mnw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), chainActive.Tip()->nHeight);
           return; 
        }

        if(winner.nBlockHeight < chainActive.Tip()->nHeight - 10 || winner.nBlockHeight > chainActive.Tip()->nHeight+20){
            LogPrintf("mnw - winner out of range - Height %d bestHeight %d\n", winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(!winner.IsValid()){
            LogPrintf("mnw - invalid message\n");
            return;
        }

        if(!winner.SignatureValid()){
            LogPrintf("mnw - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if(fDebug) LogPrintf("mnw - winning vote - Addr %s Height %d bestHeight %d\n", address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);

        if(masternodePayments.AddWinningMasternode(winner)){
            winner.Relay();
        }
    }
}

bool CMasternodePaymentWinner::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    std::string strMessage =  vinMasternode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) + 
                payee.ToString();
                
    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(!mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-100)) {
        return false;
    }

    if(mapMasternodePayeeVotes.count(winnerIn.GetHash())){
       return false; 
    }

    mapMasternodePayeeVotes[winnerIn.GetHash()] = winnerIn;

    if(!mapMasternodeBlocks.count(winnerIn.nBlockHeight)){
       CMasternodeBlockPayees blockPayees;
       mapMasternodeBlocks[winnerIn.nBlockHeight] = blockPayees;
    }

    int n = 1;
    if(IsReferenceNode(winnerIn.vinMasternode)) n = 100;
    mapMasternodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee.scriptPubKey, winnerIn.payee.nValue, n);

    return true;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    //require at least 6 signatures

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
        if(payee.nVotes >= nMaxSignatures && payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures == 0) return true;

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut out, txNew.vout)
            if(payee.scriptPubKey == out.scriptPubKey && payee.nValue == out.nValue) 
                found = true;

        //if 2 contenders are within +/- 20%, take either 
        // If nMaxSignatures == 6, the min signatures = 2
        // If nMaxSignatures == 10, the min signatures = 6
        // If nMaxSignatures == 15, the min signatures = 11
        if(payee.nVotes >= nMaxSignatures-(MNPAYMENTS_SIGNATURES_TOTAL/5)){            
            if(found) return true;

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if(strPayeesPossible == ""){
                strPayeesPossible += address2.ToString()  + ":" + boost::lexical_cast<std::string>(payee.nValue);
            } else {
                strPayeesPossible += "," + address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nValue);
            }
        }
    }


    LogPrintf("CMasternodePayments::IsTransactionValid - Missing required payment - %s\n", strPayeesPossible.c_str());
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    std::string ret = "Unknown";

    BOOST_FOREACH(CMasternodePayee& payee, vecPayments)
    {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if(ret != "Unknown"){
            ret += ", " + address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nValue);
        } else {
            ret = address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nValue);
        }
    }

    return ret;
}

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CMasternodePayments::CleanPaymentList()
{
    LOCK(cs_masternodepayments);

    if(chainActive.Tip() == NULL) return;

    int nLimit = std::max(((int)mnodeman.size())*2, 1000);

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;
   
        if(chainActive.Tip()->nHeight - winner.nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", winner.nBlockHeight);
            mapMasternodePayeeVotes.erase(it++);
        } else {
            ++it;
        }
    }
}

bool IsReferenceNode(CTxIn& vin)
{
    //reference node - hybrid mode
    if(vin.prevout.ToStringShort() == "099c01bea63abd1692f60806bb646fa1d288e2d049281225f17e499024084e28-0") return true; // mainnet
    if(vin.prevout.ToStringShort() == "testnet-0") return true; // testnet
    if(vin.prevout.ToStringShort() == "regtest-0") return true; // regtest

    return false;
}

bool CMasternodePaymentWinner::IsValid()
{
    if(IsReferenceNode(vinMasternode)) return true;

    int n = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight-100, MIN_MNPAYMENTS_PROTO_VERSION);

    if(n == -1)
    {
        if(fDebug) LogPrintf("CMasternodePaymentWinner::IsValid - Unknown Masternode\n");
        return false;
    }

    if(n > MNPAYMENTS_SIGNATURES_TOTAL)
    {
        if(fDebug) LogPrintf("CMasternodePaymentWinner::IsValid - Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
        return false;
    }

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    LOCK(cs_masternodepayments);

    if(!fMasterNode) return false;

    //reference node - hybrid mode

    if(!IsReferenceNode(activeMasternode.vin)){
        int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight-100, MIN_MNPAYMENTS_PROTO_VERSION);

        if(n == -1)
        {
            if(fDebug) LogPrintf("CMasternodePayments::ProcessBlock - Unknown Masternode\n");
            return false;
        }

        if(n > MNPAYMENTS_SIGNATURES_TOTAL)
        {
            if(fDebug) LogPrintf("CMasternodePayments::ProcessBlock - Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
            return false;
        }
    }

    if(nBlockHeight <= nLastBlockHeight) return false;
    
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return false;
    CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, 0);
    CAmount masternodePayment = GetMasternodePayment(pindexPrev->nHeight+1, blockValue);

    CMasternodePaymentWinner newWinner(activeMasternode.vin);

    if(budget.IsBudgetPaymentBlock(nBlockHeight)){
        //is budget payment block
        CScript payee;
        GetMasternodeBudgetEscrow(payee);
        newWinner.AddPayee(payee, masternodePayment);

    } else {
        CScript payeeSource;
        uint256 hash;
        if(!GetBlockHash(hash, nBlockHeight-100)) return false;
        unsigned int nHash;
        memcpy(&nHash, &hash, 2);

        LogPrintf(" ProcessBlock Start nHeight %d. \n", nBlockHeight);

        // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
        CMasternode *pmn = mnodeman.GetNextMasternodeInQueueForPayment();
        if(pmn != NULL)
        {
            LogPrintf(" Found by FindOldestNotInVec \n");

            newWinner.nBlockHeight = nBlockHeight;

            CScript payee;
            if(pmn->donationPercentage > 0 && (nHash % 100) <= (unsigned int)pmn->donationPercentage) {
                payee = pmn->donationAddress;
            } else {
                payee = GetScriptForDestination(pmn->pubkey.GetID());
            }
            
            payeeSource = GetScriptForDestination(pmn->pubkey.GetID());
            newWinner.AddPayee(payee, masternodePayment);
            
            CTxDestination address1;
            ExtractDestination(payee, address1);
            CBitcoinAddress address2(address1);

            CTxDestination address3;
            ExtractDestination(payeeSource, address3);
            CBitcoinAddress address4(address3);

            LogPrintf("Winner payee %s nHeight %d vin source %s. \n", address2.ToString().c_str(), newWinner.nBlockHeight, address4.ToString().c_str());
        } else {
            LogPrintf(" Failed to find masternode to pay\n");
        }

    }

    std::string errorMessage;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    {
        LogPrintf("CMasternodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    if(newWinner.Sign(keyMasternode, pubKeyMasternode))
    {
        if(AddWinningMasternode(newWinner))
        {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

void CMasternodePaymentWinner::Relay()
{
    CInv inv(MSG_MASTERNODE_WINNER, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

bool CMasternodePaymentWinner::SignatureValid()
{

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(pmn != NULL)
    {
        std::string strMessage =  vinMasternode.prevout.ToStringShort() +
                    boost::lexical_cast<std::string>(nBlockHeight) + 
                    payee.ToString();


        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)){
            return error("CMasternodePaymentWinner::SignatureValid() - Got bad Masternode address signature %s \n", vinMasternode.ToString().c_str());
        }

        return true;
    }

    return false;
}

void CMasternodePayments::Sync(CNode* node)
{
    LOCK(cs_masternodepayments);

    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= chainActive.Tip()->nHeight-10 && winner.nBlockHeight <= chainActive.Tip()->nHeight + 20)
            node->PushMessage("mnw", winner);
        ++it;
    }
}
