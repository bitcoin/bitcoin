// Copyright (c) 2014-2016 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

// todo 12.1 - remove the unused
#include "governance.h"
#include "masternode.h"
#include "darksend.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
//#include "addrman.h"
//#include <boost/filesystem.hpp>
//#include <boost/lexical_cast.hpp>

CBudgetManager budget;
CCriticalSection cs_budget;

std::vector<CFinalizedBudget> vecImmatureFinalizedBudgets;

int nSubmittedFinalBudget;

struct sortFinalizedBudgetsByVotes {
    bool operator()(const std::pair<CFinalizedBudget*, int> &left, const std::pair<CFinalizedBudget*, int> &right) {
        return left.second > right.second;
    }
};

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CGovernanceObject*, int> &left, const std::pair<CGovernanceObject*, int> &right) {
      if( left.second != right.second)
        return (left.second > right.second);
      return (UintToArith256(left.first->nFeeTXHash) > UintToArith256(right.first->nFeeTXHash));
    }
};


// void CBudgetManager::MarkSynced()
// {
//     LOCK(cs);

//     /*
//         Mark that we've sent all valid items
//     */

//     std::map<uint256, CFinalizedBudget>::iterator it3 = mapSeenFinalizedBudgets.begin();
//     while(it3 != mapSeenFinalizedBudgets.end()){
//         CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
//         if(pfinalizedBudget && pfinalizedBudget->fValid){

//             //mark votes
//             std::map<uint256, CBudgetVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
//             while(it4 != pfinalizedBudget->mapVotes.end()){
//                 if((*it4).second.fValid)
//                     (*it4).second.fSynced = true;
//                 ++it4;
//             }
//         }
//         ++it3;
//     }

// }

// //mark that a full sync is needed
// void CBudgetManager::ResetSync()
// {
//     LOCK(cs);

//     std::map<uint256, CFinalizedBudget>::iterator it3 = mapSeenFinalizedBudgets.begin();
//     while(it3 != mapSeenFinalizedBudgets.end()){
//         CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
//         if(pfinalizedBudget && pfinalizedBudget->fValid){

//             //send votes
//             std::map<uint256, CBudgetVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
//             while(it4 != pfinalizedBudget->mapVotes.end()){
//                 (*it4).second.fSynced = false;
//                 ++it4;
//             }
//         }
//         ++it3;
//     }
// }

void CBudgetManager::Sync(CNode* pfrom, uint256 nProp, bool fPartial)
{
    LOCK(cs);

    /*
        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;

    LogPrintf("CBudgetManager::Sync - sent %d items\n", nInvCount);

    // finalized budget -- this code has no issues as far as we can tell
    std::map<uint256, CFinalizedBudget>::iterator it3 = mapSeenFinalizedBudgets.begin();
    while(it3 != mapSeenFinalizedBudgets.end()){
        CFinalizedBudget* pfinalizedBudget = FindFinalizedBudget((*it3).first);
        if(pfinalizedBudget && pfinalizedBudget->fValid && (nProp == uint256() || (*it3).first == nProp)){
            pfrom->PushInventory(CInv(MSG_BUDGET_FINALIZED, (*it3).second.GetHash()));
            nInvCount++;

            //send votes
            std::map<uint256, CBudgetVote>::iterator it4 = pfinalizedBudget->mapVotes.begin();
            while(it4 != pfinalizedBudget->mapVotes.end()){
                if((*it4).second.fValid) {
                    if((fPartial && !(*it4).second.fSynced) || !fPartial) {
                        pfrom->PushInventory(CInv(MSG_BUDGET_FINALIZED_VOTE, (*it4).second.GetHash()));
                        nInvCount++;
                    }
                }
                ++it4;
            }
        }
        ++it3;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_BUDGET_FIN, nInvCount);
    LogPrintf("CBudgetManager::Sync - sent %d items\n", nInvCount);

}

void CBudgetManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    // todo : 12.1 - split out into two messages
    // ---  one for finalized budgets and one for gov objs
    if (strCommand == NetMsgType::MNGOVERNANCEVOTESYNC) { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::MNGOVERNANCEVOTESYNC)) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::MNGOVERNANCEVOTESYNC);
            }
        }

        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    if (strCommand == NetMsgType::MNGOVERNANCEFINAL) { //Finalized Budget Suggestion
        CFinalizedBudget finalizedBudgetBroadcast;
        vRecv >> finalizedBudgetBroadcast;

        if(mapSeenFinalizedBudgets.count(finalizedBudgetBroadcast.GetHash())){
            masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid(finalizedBudgetBroadcast.nFeeTXHash, finalizedBudgetBroadcast.GetHash(), strError, finalizedBudgetBroadcast.nTime, nConf, BUDGET_FEE_TX)){
            LogPrintf("Finalized Budget FeeTX is not valid - %s - %s\n", finalizedBudgetBroadcast.nFeeTXHash.ToString(), strError);

            if(nConf >= 1) vecImmatureFinalizedBudgets.push_back(finalizedBudgetBroadcast);
            return;
        }

        mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));

        if(!finalizedBudgetBroadcast.IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("fbs - invalid finalized budget - %s\n", strError);
            return;
        }

        LogPrintf("fbs - new finalized budget - %s\n", finalizedBudgetBroadcast.GetHash().ToString());

        CFinalizedBudget finalizedBudget(finalizedBudgetBroadcast);
        if(AddFinalizedBudget(finalizedBudget)) {finalizedBudgetBroadcast.Relay();}
        masternodeSync.AddedBudgetItem(finalizedBudgetBroadcast.GetHash());

        //we might have active votes for this budget that are now valid
        CheckOrphanVotes();
    }

    if (strCommand == NetMsgType::MNGOVERNANCEFINALVOTE) { //Finalized Budget Vote
        CBudgetVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenFinalizedBudgetVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mngovernance", "fbvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));

        if(!vote.IsValid(true)){
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateFinalizedBudget(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());

            LogPrintf("fbvote - new finalized budget vote - %s\n", vote.GetHash().ToString());
        } else {
            LogPrintf("fbvote - rejected finalized budget vote - %s - %s\n", vote.GetHash().ToString(), strError);
        }
    }
}

void CBudgetManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock) return;

    if(!pCurrentBlockIndex) return;

    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_BUDGET) return;

    if (strBudgetMode == "suggest") { //suggest the budget we see
        SubmitFinalBudget();
    }

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(pCurrentBlockIndex->nHeight % 6 != 0) return;

    // incremental sync with our peers
    std::map<uint256, CFinalizedBudget>::iterator it3 = mapFinalizedBudgets.begin();
    while(it3 != mapFinalizedBudgets.end()){
        (*it3).second.CleanAndRemove(false);
        ++it3;
    }

    std::vector<CFinalizedBudget>::iterator it5 = vecImmatureFinalizedBudgets.begin();
    while(it5 != vecImmatureFinalizedBudgets.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid((*it5).nFeeTXHash, (*it5).GetHash(), strError, (*it5).nTime, nConf, BUDGET_FEE_TX)){
            ++it5;
            continue;
        }

        if(!(*it5).IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("fbs (immature) - invalid finalized budget - %s\n", strError);
            it5 = vecImmatureFinalizedBudgets.erase(it5); 
            continue;
        }

        LogPrintf("fbs (immature) - new finalized budget - %s\n", (*it5).GetHash().ToString());

        CFinalizedBudget finalizedBudget((*it5));
        if(AddFinalizedBudget(finalizedBudget)) {(*it5).Relay();}

        it5 = vecImmatureFinalizedBudgets.erase(it5); 
    }
}

CFinalizedBudget *CBudgetManager::FindFinalizedBudget(uint256 nHash)
{
    if(mapFinalizedBudgets.count(nHash))
        return &mapFinalizedBudgets[nHash];

    return NULL;
}

CAmount CBudgetManager::GetTotalBudget(int nHeight)
{
    if(!pCurrentBlockIndex) return 0;

    //get min block value and calculate from that
    CAmount nSubsidy = 5 * COIN;

    const Consensus::Params consensusParams = Params().GetConsensus();

    // TODO: Remove this to further unify logic among mainnet/testnet/whatevernet,
    //       use single formula instead (the one that is for current mainnet).
    //       Probably a good idea to use a significally lower consensusParams.nSubsidyHalvingInterval
    //       for testnet (like 10 times for example) to see the effect of halving there faster.
    //       Will require testnet restart.
    if(Params().NetworkIDString() == CBaseChainParams::TESTNET){
        for(int i = 46200; i <= nHeight; i += consensusParams.nSubsidyHalvingInterval) nSubsidy -= nSubsidy/14;
    } else {
        // yearly decline of production by 7.1% per year, projected 21.3M coins max by year 2050.
        for(int i = consensusParams.nSubsidyHalvingInterval; i <= nHeight; i += consensusParams.nSubsidyHalvingInterval) nSubsidy -= nSubsidy/14;
    }

    // 10%
    return ((nSubsidy/100)*10)*consensusParams.nBudgetPaymentsCycleBlocks;
}

std::vector<CFinalizedBudget*> CBudgetManager::GetFinalizedBudgets()
{
    LOCK(cs);

    std::vector<CFinalizedBudget*> vFinalizedBudgetsRet;
    std::vector<std::pair<CFinalizedBudget*, int> > vFinalizedBudgetsSort;

    // ------- Grab The Budgets In Order

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);

        vFinalizedBudgetsSort.push_back(make_pair(pfinalizedBudget, pfinalizedBudget->GetVoteCount()));
        ++it;
    }
    std::sort(vFinalizedBudgetsSort.begin(), vFinalizedBudgetsSort.end(), sortFinalizedBudgetsByVotes());

    std::vector<std::pair<CFinalizedBudget*, int> >::iterator it2 = vFinalizedBudgetsSort.begin();
    while(it2 != vFinalizedBudgetsSort.end())
    {
        vFinalizedBudgetsRet.push_back((*it2).first);
        ++it2;
    }

    return vFinalizedBudgetsRet;
}

bool CBudgetManager::IsBudgetPaymentBlock(int nBlockHeight)
{
    int nHighestCount = -1;

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(pfinalizedBudget->GetVoteCount() > nHighestCount && 
            nBlockHeight >= pfinalizedBudget->GetBlockStart() && 
            nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
            nHighestCount = pfinalizedBudget->GetVoteCount();
        }

        ++it;
    }

    /*
        If budget doesn't have 5% of the network votes, then we should pay a masternode instead
    */
    if(nHighestCount > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/20) return true;

    return false;
}

bool CBudgetManager::AddFinalizedBudget(CFinalizedBudget& finalizedBudget)
{
    std::string strError = "";
    if(!finalizedBudget.IsValid(pCurrentBlockIndex, strError)) return false;

    if(mapFinalizedBudgets.count(finalizedBudget.GetHash())) {
        return false;
    }

    mapFinalizedBudgets.insert(make_pair(finalizedBudget.GetHash(), finalizedBudget));
    return true;
}

bool CBudgetManager::HasNextFinalizedBudget()
{
    if(!pCurrentBlockIndex) return false;

    if(masternodeSync.IsBudgetFinEmpty()) return true;

    int nBlockStart = pCurrentBlockIndex->nHeight - pCurrentBlockIndex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    if(nBlockStart - pCurrentBlockIndex->nHeight > 576*2) return true; //we wouldn't have the budget yet

    if(IsBudgetPaymentBlock(nBlockStart)) return true;

    LogPrintf("CBudgetManager::HasNextFinalizedBudget() - Client is missing budget - %lli\n", nBlockStart);

    return false;
}

bool CBudgetManager::UpdateFinalizedBudget(CBudgetVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapFinalizedBudgets.count(vote.nBudgetHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CBudgetManager::UpdateFinalizedBudget - Unknown Finalized Proposal %s, asking for source budget\n", vote.nBudgetHash.ToString());
            mapOrphanFinalizedBudgetVotes[vote.nBudgetHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nBudgetHash)){
                pfrom->PushMessage(NetMsgType::MNGOVERNANCEVOTESYNC, vote.nBudgetHash);
                askedForSourceProposalOrBudget[vote.nBudgetHash] = GetTime();
            }

        }

        strError = "Finalized Budget not found!";
        return false;
    }

    return mapFinalizedBudgets[vote.nBudgetHash].AddOrUpdateVote(vote, strError);
}

// todo - 12.1 - terrible name - how about IsBlockTransactionValid
bool CBudgetManager::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs);

    int nHighestCount = 0;
    std::vector<CFinalizedBudget*> ret;

    // ------- Grab The Highest Count

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(pfinalizedBudget->GetVoteCount() > nHighestCount &&
                nBlockHeight >= pfinalizedBudget->GetBlockStart() &&
                nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
                    nHighestCount = pfinalizedBudget->GetVoteCount();
        }

        ++it;
    }

    /*
        If budget doesn't have 5% of the network votes, then we should pay a masternode instead
    */
    if(nHighestCount < mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/20) return false;

    // check the highest finalized budgets (+/- 10% to assist in consensus)

    it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);

        if(pfinalizedBudget->GetVoteCount() > nHighestCount - mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
            if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
                if(pfinalizedBudget->IsTransactionValid(txNew, nBlockHeight)){
                    return true;
                }
            }
        }

        ++it;
    }

    //we looked through all of the known budgets
    return false;
}

std::string CBudgetManager::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs);

    std::string ret = "unknown-budget";

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(nBlockHeight >= pfinalizedBudget->GetBlockStart() && nBlockHeight <= pfinalizedBudget->GetBlockEnd()){
            CTxBudgetPayment payment;
            if(pfinalizedBudget->GetBudgetPaymentByBlock(nBlockHeight, payment)){
                if(ret == "unknown-budget"){
                    ret = payment.nProposalHash.ToString();
                } else {
                    ret += ",";
                    ret += payment.nProposalHash.ToString();
                }
            } else {
                LogPrintf("CBudgetManager::GetRequiredPaymentsString - Couldn't find budget payment for block %d\n", nBlockHeight);
            }
        }

        ++it;
    }

    return ret;
}

void CBudgetManager::FillBlockPayee(CMutableTransaction& txNew, CAmount nFees)
{
    LOCK(cs);

    AssertLockHeld(cs_main);
    if(!chainActive.Tip()) return;

    int nHighestCount = 0;
    CScript payee;
    CAmount nAmount = 0;

    // ------- Grab The Highest Count

    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);
        if(pfinalizedBudget->GetVoteCount() > nHighestCount &&
                chainActive.Tip()->nHeight + 1 >= pfinalizedBudget->GetBlockStart() &&
                chainActive.Tip()->nHeight + 1 <= pfinalizedBudget->GetBlockEnd() &&
                pfinalizedBudget->GetPayeeAndAmount(chainActive.Tip()->nHeight + 1, payee, nAmount)){
                    nHighestCount = pfinalizedBudget->GetVoteCount();
        }

        ++it;
    }

    //miners get the full amount on these blocks
    txNew.vout[0].nValue = nFees + GetBlockSubsidy(chainActive.Tip()->nBits, chainActive.Tip()->nHeight, Params().GetConsensus());

    if(nHighestCount > 0){
        txNew.vout.resize(2);

        //these are super blocks, so their value can be much larger than normal
        txNew.vout[1].scriptPubKey = payee;
        txNew.vout[1].nValue = nAmount;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CBudgetManager::FillBlockPayee - Budget payment to %s for %lld\n", address2.ToString(), nAmount);
    }

}


void CBudgetManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";

    std::map<uint256, CBudgetVote>::iterator it2 = mapOrphanFinalizedBudgetVotes.begin();
    while(it2 != mapOrphanFinalizedBudgetVotes.end()){
        if(UpdateFinalizedBudget(((*it2).second),NULL, strError)){
            LogPrintf("CBudgetManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanFinalizedBudgetVotes.erase(it2++);
        } else {
            ++it2;
        }
    }
}

void CBudgetManager::CheckAndRemove()
{
    LogPrintf("CBudgetManager::CheckAndRemove \n");

    if(!pCurrentBlockIndex) return;

    std::string strError = "";
    std::map<uint256, CFinalizedBudget>::iterator it = mapFinalizedBudgets.begin();
    while(it != mapFinalizedBudgets.end())
    {
        CFinalizedBudget* pfinalizedBudget = &((*it).second);

        pfinalizedBudget->fValid = pfinalizedBudget->IsValid(pCurrentBlockIndex, strError);
        if(pfinalizedBudget->fValid) {
            pfinalizedBudget->AutoCheck();
            ++it;
            continue;
        } else if(pfinalizedBudget->nBlockStart != 0 && pfinalizedBudget->nBlockStart < pCurrentBlockIndex->nHeight - Params().GetConsensus().nBudgetPaymentsCycleBlocks) {
            // it's too old, remove it
            mapFinalizedBudgets.erase(it++);
            LogPrintf("CBudgetManager::CheckAndRemove - removing budget %s\n", pfinalizedBudget->GetHash().ToString());
            continue;
        }
        // it's not valid already but it's not too old yet, keep it and move to the next one
        ++it;
    }
}

std::string CBudgetManager::ToString() const
{
    std::ostringstream info;

    info << "Budgets: " << (int)mapFinalizedBudgets.size() <<
            ", Seen Final Budgets: " << (int)mapSeenFinalizedBudgets.size() <<
            ", Seen Final Budget Votes: " << (int)mapSeenFinalizedBudgetVotes.size();

    return info.str();
}

// todo - 12.1 - rename : UpdateBlockTip
void CBudgetManager::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mngovernance", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

/*
 * Class : CFinalizedBudget
 * --------------------
 *  Holds a proposed finalized budget
 *
*/


CFinalizedBudget::CFinalizedBudget()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecBudgetPayments.clear();
    mapVotes.clear();
    nFeeTXHash = uint256();
    nTime = 0;
    fValid = true;
    fAutoChecked = false;
}

CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    vecBudgetPayments = other.vecBudgetPayments;
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
    nTime = other.nTime;
    fValid = true;
    fAutoChecked = false;
}

bool CFinalizedBudget::AddOrUpdateVote(CBudgetVote& vote, std::string& strError)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();
    if(mapVotes.count(hash)){
        if(mapVotes[hash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
            LogPrint("mngovernance", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash].nTime < BUDGET_VOTE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime);
            LogPrint("mngovernance", "CFinalizedBudget::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    mapVotes[hash] = vote;
    return true;
}

//evaluate if we should vote for this. Masternode only
void CFinalizedBudget::AutoCheck()
{
    LOCK(cs);

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    LogPrintf("CFinalizedBudget::AutoCheck - %lli - %d\n", pindexPrev->nHeight, fAutoChecked);

    if(!fMasterNode || fAutoChecked) return;

    //do this 1 in 4 blocks -- spread out the voting activity on mainnet
    // -- this function is only called every sixth block, so this is really 1 in 24 blocks
    if(Params().NetworkIDString() == CBaseChainParams::MAIN && rand() % 4 != 0) {
        LogPrintf("CFinalizedBudget::AutoCheck - waiting\n");
        return;
    }

    fAutoChecked = true; //we only need to check this once


    if(strBudgetMode == "auto") //only vote for exact matches
    {
        std::vector<CGovernanceObject*> vBudgetProposals = budget.GetBudget();


        for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
            LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vecBudgetPayments[i].nProposalHash.ToString());
            LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, ScriptToAsmStr(vecBudgetPayments[i].payee));
            LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vecBudgetPayments[i].nAmount);
        }

        for(unsigned int i = 0; i < vBudgetProposals.size(); i++){
            LogPrintf("CFinalizedBudget::AutoCheck - nProp %d %s\n", i, vBudgetProposals[i]->GetHash().ToString());
            LogPrintf("CFinalizedBudget::AutoCheck - Payee %d %s\n", i, ScriptToAsmStr(vBudgetProposals[i]->GetPayee()));
            LogPrintf("CFinalizedBudget::AutoCheck - nAmount %d %lli\n", i, vBudgetProposals[i]->GetAmount());
        }

        if(vBudgetProposals.size() == 0) {
            LogPrintf("CFinalizedBudget::AutoCheck - Can't get Budget, aborting\n");
            return;
        }

        if(vBudgetProposals.size() != vecBudgetPayments.size()) {
            LogPrintf("CFinalizedBudget::AutoCheck - Budget length doesn't match\n");
            return;
        }


        for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
            if(i > vBudgetProposals.size() - 1) {
                LogPrintf("CFinalizedBudget::AutoCheck - Vector size mismatch, aborting\n");
                return;
            }

            if(vecBudgetPayments[i].nProposalHash != vBudgetProposals[i]->GetHash()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d doesn't match %s %s\n", i, vecBudgetPayments[i].nProposalHash.ToString(), vBudgetProposals[i]->GetHash().ToString());
                return;
            }

            // if(vecBudgetPayments[i].payee != vBudgetProposals[i]->GetPayee()){ -- triggered with false positive 
            if(vecBudgetPayments[i].payee != vBudgetProposals[i]->GetPayee()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %s %s\n", i, ScriptToAsmStr(vecBudgetPayments[i].payee), ScriptToAsmStr(vBudgetProposals[i]->GetPayee()));
                return;
            }

            if(vecBudgetPayments[i].nAmount != vBudgetProposals[i]->GetAmount()){
                LogPrintf("CFinalizedBudget::AutoCheck - item #%d payee doesn't match %lli %lli\n", i, vecBudgetPayments[i].nAmount, vBudgetProposals[i]->GetAmount());
                return;
            }
        }

        LogPrintf("CFinalizedBudget::AutoCheck - Finalized Budget Matches! Submitting Vote.\n");
        SubmitVote();

    }
}
// If masternode voted for a proposal, but is now invalid -- remove the vote
void CFinalizedBudget::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        (*it).second.fValid = (*it).second.IsValid(fSignatureCheck);
        ++it;
    }
}


CAmount CFinalizedBudget::GetTotalPayout()
{
    CAmount ret = 0;

    for(unsigned int i = 0; i < vecBudgetPayments.size(); i++){
        ret += vecBudgetPayments[i].nAmount;
    }

    return ret;
}

std::string CFinalizedBudget::GetProposals() 
{
    LOCK(cs);
    std::string ret = "";

    BOOST_FOREACH(CTxBudgetPayment& budgetPayment, vecBudgetPayments){
        CGovernanceObject* pbudgetProposal = governance.FindProposal(budgetPayment.nProposalHash);

        std::string token = budgetPayment.nProposalHash.ToString();

        if(pbudgetProposal) token = pbudgetProposal->GetName();
        if(ret == "") {ret = token;}
        else {ret += "," + token;}
    }
    return ret;
}

std::string CFinalizedBudget::GetStatus()
{
    std::string retBadHashes = "";
    std::string retBadPayeeOrAmount = "";

    for(int nBlockHeight = GetBlockStart(); nBlockHeight <= GetBlockEnd(); nBlockHeight++)
    {
        CTxBudgetPayment budgetPayment;
        if(!GetBudgetPaymentByBlock(nBlockHeight, budgetPayment)){
            LogPrintf("CFinalizedBudget::GetStatus - Couldn't find budget payment for block %lld\n", nBlockHeight);
            continue;
        }

        CGovernanceObject* pbudgetProposal =  governance.FindProposal(budgetPayment.nProposalHash);
        if(!pbudgetProposal){
            if(retBadHashes == ""){
                retBadHashes = "Unknown proposal hash! Check this proposal before voting" + budgetPayment.nProposalHash.ToString();
            } else {
                retBadHashes += "," + budgetPayment.nProposalHash.ToString();
            }
        } else {
            if(pbudgetProposal->GetPayee() != budgetPayment.payee || pbudgetProposal->GetAmount() != budgetPayment.nAmount)
            {
                if(retBadPayeeOrAmount == ""){
                    retBadPayeeOrAmount = "Budget payee/nAmount doesn't match our proposal! " + budgetPayment.nProposalHash.ToString();
                } else {
                    retBadPayeeOrAmount += "," + budgetPayment.nProposalHash.ToString();
                }
            }
        }
    }

    if(retBadHashes == "" && retBadPayeeOrAmount == "") return "OK";

    return retBadHashes + retBadPayeeOrAmount;
}

bool CFinalizedBudget::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    //must be the correct block for payment to happen (once a month)
    if(nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks != 0) {strError = "Invalid BlockStart"; return false;}
    if(GetBlockEnd() - nBlockStart > Params().GetConsensus().nBudgetPaymentsWindowBlocks) {strError = "Invalid BlockEnd"; return false;}
    if((int)vecBudgetPayments.size() > Params().GetConsensus().nBudgetPaymentsWindowBlocks) {strError = "Invalid budget payments count (too many)"; return false;}
    if(strBudgetName == "") {strError = "Invalid Budget Name"; return false;}
    if(nBlockStart == 0) {strError = "Invalid BlockStart == 0"; return false;}
    if(nFeeTXHash == uint256()) {strError = "Invalid FeeTx == 0"; return false;}

    //can only pay out 10% of the possible coins (min value of coins)
    if(GetTotalPayout() > budget.GetTotalBudget(nBlockStart)) {strError = "Invalid Payout (more than max)"; return false;}

    std::string strError2 = "";
    if(fCheckCollateral){
        int nConf = 0;
        if(!IsCollateralValid(nFeeTXHash, GetHash(), strError2, nTime, nConf, BUDGET_FEE_TX)){
            strError = "Invalid Collateral : " + strError2;
            return false;
        }
    }

    //TODO: if N cycles old, invalid, invalid

    if(!pindex) return true;

    if(nBlockStart < pindex->nHeight - Params().GetConsensus().nBudgetPaymentsWindowBlocks) {
        strError = "Older than current blockHeight";
        return false;
    }

    return true;
}

bool CFinalizedBudget::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    int nCurrentBudgetPayment = nBlockHeight - GetBlockStart();
    if(nCurrentBudgetPayment < 0) {
        LogPrintf("CFinalizedBudget::IsTransactionValid - Invalid block - height: %d start: %d\n", nBlockHeight, GetBlockStart());
        return false;
    }

    if(nCurrentBudgetPayment > (int)vecBudgetPayments.size() - 1) {
        LogPrintf("CFinalizedBudget::IsTransactionValid - Invalid block - current budget payment: %d of %d\n", nCurrentBudgetPayment + 1, (int)vecBudgetPayments.size());
        return false;
    }

    bool found = false;
    BOOST_FOREACH(CTxOut out, txNew.vout)
    {
        if(vecBudgetPayments[nCurrentBudgetPayment].payee == out.scriptPubKey && vecBudgetPayments[nCurrentBudgetPayment].nAmount == out.nValue)
            found = true;
    }
    if(!found) {
        CTxDestination address1;
        ExtractDestination(vecBudgetPayments[nCurrentBudgetPayment].payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("CFinalizedBudget::IsTransactionValid - Missing required payment - %s: %d\n", address2.ToString(), vecBudgetPayments[nCurrentBudgetPayment].nAmount);
    }
    
    return found;
}

void CFinalizedBudget::SubmitVote()
{
    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    std::string errorMessage;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Error upon calling SetKey\n");
        return;
    }

    CBudgetVote vote(activeMasternode.vin, GetHash());
    if(!vote.Sign(keyMasternode, pubKeyMasternode)){
        LogPrintf("CFinalizedBudget::SubmitVote - Failure to sign.");
        return;
    }

    std::string strError = "";
    if(budget.UpdateFinalizedBudget(vote, NULL, strError)){
        LogPrintf("CFinalizedBudget::SubmitVote  - new finalized budget vote - %s\n", vote.GetHash().ToString());

        budget.mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
    } else {
        LogPrintf("CFinalizedBudget::SubmitVote : Error submitting vote - %s\n", strError);
    }
}

CFinalizedBudget::CFinalizedBudget()
{
    strBudgetName = "";
    nBlockStart = 0;
    vecBudgetPayments.clear();
    mapVotes.clear();
    vchSig.clear();
    nFeeTXHash = uint256();
}

CFinalizedBudget::CFinalizedBudget(const CFinalizedBudget& other)
{
    strBudgetName = other.strBudgetName;
    nBlockStart = other.nBlockStart;
    BOOST_FOREACH(CTxBudgetPayment out, other.vecBudgetPayments) vecBudgetPayments.push_back(out);
    mapVotes = other.mapVotes;
    nFeeTXHash = other.nFeeTXHash;
}

CFinalizedBudget::CFinalizedBudget(std::string strBudgetNameIn, int nBlockStartIn, std::vector<CTxBudgetPayment> vecBudgetPaymentsIn, uint256 nFeeTXHashIn)
{
    strBudgetName = strBudgetNameIn;
    nBlockStart = nBlockStartIn;
    BOOST_FOREACH(CTxBudgetPayment out, vecBudgetPaymentsIn) vecBudgetPayments.push_back(out);
    mapVotes.clear();
    nFeeTXHash = nFeeTXHashIn;
}

void CFinalizedBudget::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

CBudgetVote::CBudgetVote()
{
    vin = CTxIn();
    nBudgetHash = uint256();
    nTime = 0;
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

CBudgetVote::CBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn)
{
    vin = vinIn;
    nBudgetHash = nBudgetHashIn;
    nTime = GetAdjustedTime();
    vchSig.clear();
    fValid = true;
    fSynced = false;
}

void CBudgetVote::Relay()
{
    CInv inv(MSG_BUDGET_FINALIZED_VOTE, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

bool CBudgetVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CBudgetVote::IsValid(bool fSignatureCheck)
{
    if(nTime > GetTime() + (60*60)){
        LogPrint("mngovernance", "CBudgetVote::IsValid() - vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn == NULL)
    {
        LogPrint("mngovernance", "CBudgetVote::IsValid() - Unknown Masternode\n");
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nBudgetHash.ToString() + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CBudgetVote::IsValid() - Verify message failed\n");
        return false;
    }

    return true;
}

