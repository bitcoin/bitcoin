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
// keep track of Masternode votes I've seen
map<uint256, CMasternodePaymentWinner> mapSeenMasternodeVotes;

void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
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

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        uint256 hash = winner.GetHash();
        if(mapSeenMasternodeVotes.count(hash)) {
            if(fDebug) LogPrintf("mnw - seen vote %s Addr %s Height %d bestHeight %d\n", hash.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.nBlockHeight < chainActive.Tip()->nHeight - 10 || winner.nBlockHeight > chainActive.Tip()->nHeight+20){
            LogPrintf("mnw - winner out of range %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.vin.nSequence != std::numeric_limits<unsigned int>::max()){
            LogPrintf("mnw - invalid nSequence\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrintf("mnw - winning vote - Vin %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);

        if(!masternodePayments.CheckSignature(winner)){
            LogPrintf("mnw - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSeenMasternodeVotes.insert(make_pair(hash, winner));

        if(masternodePayments.AddWinningMasternode(winner)){
            masternodePayments.Relay(winner);
        }
    }
}


bool CMasternodePayments::CheckSignature(CMasternodePaymentWinner& winner)
{
    //note: need to investigate why this is failing
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();
    CPubKey pubkey(ParseHex(Params().MasternodePaymentPubKey()));

    std::string errorMessage = "";
    if(!darkSendSigner.VerifyMessage(pubkey, winner.vchSig, strMessage, errorMessage)){
        return false;
    }

    return true;
}

bool CMasternodePayments::Sign(CMasternodePaymentWinner& winner)
{
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid Masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, winner.vchSig, key2)) {
        LogPrintf("CMasternodePayments::Sign - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, winner.vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

uint64_t CMasternodePayments::CalculateScore(uint256 blockHash, CTxIn& vin)
{
    uint256 n1 = blockHash;
    uint256 n2 = HashX11(BEGIN(n1), END(n1));
    uint256 n3 = HashX11(BEGIN(vin.prevout.hash), END(vin.prevout.hash));
    uint256 n4 = n3 > n2 ? (n3 - n2) : (n2 - n3);

    //printf(" -- CMasternodePayments CalculateScore() n2 = %d \n", n2.Get64());
    //printf(" -- CMasternodePayments CalculateScore() n3 = %d \n", n3.Get64());
    //printf(" -- CMasternodePayments CalculateScore() n4 = %d \n", n4.Get64());

    return n4.Get64();
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payee, CTxIn& vin)
{
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            payee = winner.payee;
            vin = winner.vin;
            return true;
        }
    }

    return false;
}

bool CMasternodePayments::GetWinningMasternode(int nBlockHeight, CTxIn& vinOut)
{
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            vinOut = winner.vin;
            return true;
        }
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-576)) {
        return false;
    }

    winnerIn.score = CalculateScore(blockHash, winnerIn.vin);

    bool foundBlock = false;
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == winnerIn.nBlockHeight) {
            foundBlock = true;
            if(winner.score < winnerIn.score){
                winner.score = winnerIn.score;
                winner.vin = winnerIn.vin;
                winner.payee = winnerIn.payee;
                winner.vchSig = winnerIn.vchSig;

                mapSeenMasternodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

                return true;
            }
        }
    }

    // if it's not in the vector
    if(!foundBlock){
        vWinning.push_back(winnerIn);
        mapSeenMasternodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

        return true;
    }

    return false;
}

void CMasternodePayments::CleanPaymentList()
{
    LOCK(cs_masternodepayments);

    if(chainActive.Tip() == NULL) return;

    int nLimit = std::max(((int)mnodeman.size())*2, 1000);

    vector<CMasternodePaymentWinner>::iterator it;
    for(it=vWinning.begin();it<vWinning.end();it++){
        if(chainActive.Tip()->nHeight - (*it).nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", (*it).nBlockHeight);
            vWinning.erase(it);
            break;
        }
    }
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    LOCK(cs_masternodepayments);

    if(nBlockHeight <= nLastBlockHeight) return false;
    if(!enabled) return false;
    CMasternodePaymentWinner newWinner;
    int nMinimumAge = mnodeman.CountEnabled();
    CScript payeeSource;

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

        newWinner.score = 0;
        newWinner.nBlockHeight = nBlockHeight;
        newWinner.vin = pmn->vin;

        if(pmn->donationPercentage > 0 && (nHash % 100) <= (unsigned int)pmn->donationPercentage) {
            newWinner.payee = pmn->donationAddress;
        } else {
            newWinner.payee = GetScriptForDestination(pmn->pubkey.GetID());
        }

        payeeSource = GetScriptForDestination(pmn->pubkey.GetID());
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

                newWinner.score = 0;
                newWinner.nBlockHeight = nBlockHeight;
                newWinner.vin = pmn->vin;

                if(pmn->donationPercentage > 0 && (nHash % 100) <= (unsigned int)pmn->donationPercentage) {
                    newWinner.payee = pmn->donationAddress;
                } else {
                    newWinner.payee = GetScriptForDestination(pmn->pubkey.GetID());
                }

                payeeSource = GetScriptForDestination(pmn->pubkey.GetID());

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

    if(Sign(newWinner))
    {
        if(AddWinningMasternode(newWinner))
        {
            Relay(newWinner);
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}


void CMasternodePayments::Relay(CMasternodePaymentWinner& winner)
{
    CInv inv(MSG_MASTERNODE_WINNER, winner.GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

void CMasternodePayments::Sync(CNode* node)
{
    LOCK(cs_masternodepayments);

    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning)
        if(winner.nBlockHeight >= chainActive.Tip()->nHeight-10 && winner.nBlockHeight <= chainActive.Tip()->nHeight + 20)
            node->PushMessage("mnw", winner);
}


bool CMasternodePayments::SetPrivKey(std::string strPrivKey)
{
    CMasternodePaymentWinner winner;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(winner);

    if(CheckSignature(winner)){
        LogPrintf("CMasternodePayments::SetPrivKey - Successfully initialized as Masternode payments master\n");
        enabled = true;
        return true;
    } else {
        return false;
    }
}
