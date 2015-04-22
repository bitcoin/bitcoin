// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
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
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-576)) {
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
  /*  BOOST_FOREACH(CMasternodePayee& payee, vecPayees)
    {
        bool found = false;
        BOOST_FOREACH(CTxOut& out, txNew.vout)
        {
            if(payee.scriptPubKey == out.scriptPubKey && payee.nValue == out.nValue) 
                found = true;
        }

        if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED && !found){

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            LogPrintf("CMasternodePayments::IsTransactionValid - Missing required payment - %s:%d\n", address2.ToString().c_str, payee.nValue);
            return false;
        }
    }
*/
    return true;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    std::string ret = "Unknown";

/*    BOOST_FOREACH(CMasternodePayee& payee, vecPayees)
    {
        if(payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED){
            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if(ret != "Unknown"){
                ret += sprintf(", %s:%d", address2.ToString(), payee.nValue);
            } else {
                ret = sprintf("%s:%d", address2.ToString(), payee.nValue);
            }
        }
    }
*/
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

   /* vector<CMasternodePaymentWinner>::iterator it;
    for(it=vWinning.begin();it<vWinning.end();it++){
        if(chainActive.Tip()->nHeight - (*it).nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", (*it).nBlockHeight);
            vWinning.erase(it);
            break;
        }
    }*/
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

    int n = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight, MIN_MNPAYMENTS_PROTO_VERSION);

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
        int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_MNPAYMENTS_PROTO_VERSION);

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
    
  /*  if(budget.IsBudgetPaymentBlock(nBlockHeight)){
        //is budget payment block
        std::vector<CBudgetProposal*> budget = budget.GetBudget();
        BOOST_FOREACH(CBudgetProposal* prop, budget){
            if(prop.GetAllocated() > 0){
                newWinner.AddPayee(prop.GetPayee(), prop.GetAllocated());
            }
        }
    } else {

        CMasternodePaymentWinner newWinner(activeMasternode.vin);
        int nMinimumAge = mnodeman.CountEnabled();
        CScript payeeSource;

        CBlockIndex* pindexPrev = chainActive.Tip();
        if(pindexPrev == NULL) return false;
        CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, 0);
        CAmount masternodePayment = GetMasternodePayment(pindexPrev->nHeight+1, blockValue);

        uint256 hash;
        if(!GetBlockHash(hash, nBlockHeight-10)) return false;
        unsigned int nHash;
        memcpy(&nHash, &hash, 2);

        LogPrintf(" ProcessBlock Start nHeight %d. \n", nBlockHeight);

        std::vector<CTxIn> vecLastPayments;
        BOOST_REVERSE_FOREACH(CMasternodePaymentWinner& winner, vWinning)
        {
            //if we already have the same vin - we have one full payment cycle, break
            if(vecLastPayments.size() > (unsigned int)nMinimumAge) break;
            vecLastPayments.push_back(winner.vin);
        }

        // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
        CMasternode *pmn = mnodeman.FindOldestNotInVec(vecLastPayments, nMinimumAge);
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
            newWinner.AddPayee(payee, masternodePayment);
        }

        //if we can't find new MN to get paid, pick first active MN counting back from the end of vecLastPayments list
        if(newWinner.nBlockHeight == 0 && nMinimumAge > 0)
        {
            LogPrintf(" Find by reverse \n");

            BOOST_REVERSE_FOREACH(CTxIn& vinLP, vecLastPayments)
            {
                CMasternode* pmn = mnodeman.Find(vinLP);
                if(pmn != NULL)
                {
                    pmn->Check();
                    if(!pmn->IsEnabled()) continue; 
                    if(pmn->SecondsSincePayment() < 60*60*24) continue;

                    newWinner.nBlockHeight = nBlockHeight;

                    CScript payee;
                    if(pmn->donationPercentage > 0 && (nHash % 100) <= (unsigned int)pmn->donationPercentage) {
                        payee = pmn->donationAddress;
                    } else {
                        payee = GetScriptForDestination(pmn->pubkey.GetID());
                    }
                    newWinner.AddPayee(payee, masternodePayment);

                    break; // we found active MN
                }
            }
        }

        if(newWinner.nBlockHeight == 0) return false;

        CTxDestination address1;
        ExtractDestination(newWinner.payee, address1);
        CBitcoinAddress address2(address1);

        CTxDestination address3;
        ExtractDestination(payeeSource, address3);
        CBitcoinAddress address4(address3);

        LogPrintf("Winner payee %s nHeight %d vin source %s. \n", address2.ToString().c_str(), newWinner.nBlockHeight, address4.ToString().c_str());
    }

    std::string errorMessage;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    {
        LogPrintf("CMasternodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    if(Sign(keyMasternode, pubKeyMasternode))
    {
        if(AddWinningMasternode(newWinner))
        {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }
*/
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
/*
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning)
        if(winner.nBlockHeight >= chainActive.Tip()->nHeight-10 && winner.nBlockHeight <= chainActive.Tip()->nHeight + 20)
            node->PushMessage("mnw", winner);*/
}