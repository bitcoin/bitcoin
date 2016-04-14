// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "governance-vote.h"
#include "masternode.h"
#include "masternode-budget.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

class CNode;

CGovernanceManager governance;
CCriticalSection cs_budget;

std::map<uint256, int64_t> askedForSourceProposalOrBudget;
std::vector<CBudgetProposal> vecImmatureBudgetProposals;

int nSubmittedFinalBudget;

bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf, CAmount minFee)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if(!GetTransaction(nTxCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf ("CBudgetProposal::IsCollateralValid - %s\n", strError);
        return false;
    }

    if(txCollateral.vout.size() < 1) return false;
    if(txCollateral.nLockTime != 0) return false;

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    bool foundOpReturn = false;
    BOOST_FOREACH(const CTxOut o, txCollateral.vout){
        if(!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()){
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrintf ("CBudgetProposal::IsCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= minFee) foundOpReturn = true;

    }
    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CBudgetProposal::IsCollateralValid - %s\n", strError);
        return false;
    }

    LOCK(cs_main);
    int conf = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != uint256()) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                conf += chainActive.Height() - pindex->nHeight + 1;
                nTime = pindex->nTime;
            }
        }
    }

    nConf = conf;

    //if we're syncing we won't have instantX information, so accept 1 confirmation 
    if(conf >= BUDGET_FEE_CONFIRMATIONS){
        return true;
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", BUDGET_FEE_CONFIRMATIONS, conf);
        LogPrintf ("CBudgetProposal::IsCollateralValid - %s - %d confirmations\n", strError, conf);
        return false;
    }
}

void CGovernanceManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";
    std::map<uint256, CBudgetVote>::iterator it1 = mapOrphanMasternodeBudgetVotes.begin();
    while(it1 != mapOrphanMasternodeBudgetVotes.end()){
        if(UpdateProposal(((*it1).second), NULL, strError)){
            LogPrintf("CGovernanceManager::CheckOrphanVotes - Proposal/Budget is known, activating and removing orphan vote\n");
            mapOrphanMasternodeBudgetVotes.erase(it1++);
        } else {
            ++it1;
        }
    }
}

bool CGovernanceManager::AddProposal(CBudgetProposal& budgetProposal)
{
    LOCK(cs);
    std::string strError = "";
    if(!budgetProposal.IsValid(pCurrentBlockIndex, strError)) {
        LogPrintf("CGovernanceManager::AddProposal - invalid budget proposal - %s\n", strError);
        return false;
    }

    if(mapProposals.count(budgetProposal.GetHash())) {
        return false;
    }

    mapProposals.insert(make_pair(budgetProposal.GetHash(), budgetProposal));
    return true;
}

void CGovernanceManager::CheckAndRemove()
{
    LogPrintf("CGovernanceManager::CheckAndRemove \n");

    if(!pCurrentBlockIndex) return;

    std::string strError = "";

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CBudgetProposal* pbudgetProposal = &((*it2).second);
        pbudgetProposal->fValid = pbudgetProposal->IsValid(pCurrentBlockIndex, strError);
        ++it2;
    }
}

CBudgetProposal *CGovernanceManager::FindProposal(const std::string &strProposalName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CBudgetProposal* pbudgetProposal = NULL;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        if((*it).second.strProposalName == strProposalName && (*it).second.GetYesCount() > nYesCount){
            pbudgetProposal = &((*it).second);
            nYesCount = pbudgetProposal->GetYesCount();
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pbudgetProposal;
}

CBudgetProposal *CGovernanceManager::FindProposal(uint256 nHash)
{
    LOCK(cs);

    if(mapProposals.count(nHash))
        return &mapProposals[nHash];

    return NULL;
}

std::vector<CBudgetProposal*> CGovernanceManager::GetAllProposals()
{
    LOCK(cs);

    std::vector<CBudgetProposal*> vBudgetProposalRet;

    std::map<uint256, CBudgetProposal>::iterator it = mapProposals.begin();
    while(it != mapProposals.end())
    {
        (*it).second.CleanAndRemove(false);

        CBudgetProposal* pbudgetProposal = &((*it).second);
        vBudgetProposalRet.push_back(pbudgetProposal);

        ++it;
    }

    return vBudgetProposalRet;
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortProposalsByVotes {
    bool operator()(const std::pair<CBudgetProposal*, int> &left, const std::pair<CBudgetProposal*, int> &right) {
      if( left.second != right.second)
        return (left.second > right.second);
      return (UintToArith256(left.first->nFeeTXHash) > UintToArith256(right.first->nFeeTXHash));
    }
};

void CGovernanceManager::NewBlock()
{
    TRY_LOCK(cs, fBudgetNewBlock);
    if(!fBudgetNewBlock) return;

    if(!pCurrentBlockIndex) return;

    // todo - 12.1 - add govobj sync
    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_BUDGET) return;

    //this function should be called 1/6 blocks, allowing up to 100 votes per day on all proposals
    if(pCurrentBlockIndex->nHeight % 6 != 0) return;

    // description: incremental sync with our peers
    // note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
    // note: 12.1 - remove
    // if(masternodeSync.IsSynced()){
    //     /*
    //         Needs comment below:

    //         - Why are we doing a partial sync with our peers on new blocks? 
    //         - Why only partial? Why not partial when we recieve the request directly?
    //         - Can we simplify this somehow?
    //         - Why are we marking everything synced? Was this to correct a bug? 
    //     */

    //     LogPrintf("CGovernanceManager::NewBlock - incremental sync started\n");
    //     if(pCurrentBlockIndex->nHeight % 600 == rand() % 600) {
    //         ClearSeen();
    //         ResetSync();
    //     }

    //     LOCK(cs_vNodes);
    //     BOOST_FOREACH(CNode* pnode, vNodes)
    //         if(pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) 
    //             Sync(pnode, uint256());

    //     MarkSynced();
    // }

    CheckAndRemove();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)

    std::map<uint256, int64_t>::iterator it = askedForSourceProposalOrBudget.begin();
    while(it != askedForSourceProposalOrBudget.end()){
        if((*it).second > GetTime() - (60*60*24)){
            ++it;
        } else {
            askedForSourceProposalOrBudget.erase(it++);
        }
    }

    std::map<uint256, CBudgetProposal>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end()){
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    std::vector<CBudgetProposal>::iterator it4 = vecImmatureBudgetProposals.begin();
    while(it4 != vecImmatureBudgetProposals.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf, GOVERNANCE_FEE_TX)){
            ++it4;
            continue;
        }

        if(!(*it4).IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("mprop (immature) - invalid budget proposal - %s\n", strError);
            it4 = vecImmatureBudgetProposals.erase(it4); 
            continue;
        }

        CBudgetProposal budgetProposal((*it4));
        if(AddProposal(budgetProposal)) {(*it4).Relay();}

        LogPrintf("mprop (immature) - new budget - %s\n", (*it4).GetHash().ToString());
        it4 = vecImmatureBudgetProposals.erase(it4); 
    }

}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    // todo - 12.1 - change to MNGOVERNANCEVOTESYNC
    if (strCommand == NetMsgType::MNBUDGETVOTESYNC) { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::MNBUDGETVOTESYNC)) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::MNBUDGETVOTESYNC);
            }
        }

        // ask for a specific proposal and it's votes
        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    // todo - 12.1 - change to MNGOVERNANCEPROPOSAL
    if (strCommand == NetMsgType::MNBUDGETPROPOSAL) { //Masternode Proposal
        CBudgetProposal budgetProposalBroadcast;
        vRecv >> budgetProposalBroadcast;

        if(mapSeenMasternodeBudgetProposals.count(budgetProposalBroadcast.GetHash())){
            masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid(budgetProposalBroadcast.nFeeTXHash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf, GOVERNANCE_FEE_TX)){
            LogPrintf("Proposal FeeTX is not valid - %s - %s\n", budgetProposalBroadcast.nFeeTXHash.ToString(), strError);
            if(nConf >= 1) vecImmatureBudgetProposals.push_back(budgetProposalBroadcast);
            return;
        }

        mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));

        if(!budgetProposalBroadcast.IsValid(pCurrentBlockIndex, strError)) {
            LogPrintf("mprop - invalid budget proposal - %s\n", strError);
            return;
        }

        CBudgetProposal budgetProposal(budgetProposalBroadcast);
        if(AddProposal(budgetProposal)) {budgetProposalBroadcast.Relay();}
        masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());

        LogPrintf("mprop - new budget - %s\n", budgetProposalBroadcast.GetHash().ToString());

        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    // todo - 12.1 - change to MNGOVERNANCEVOTE
    if (strCommand == NetMsgType::MNBUDGETVOTE) { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenMasternodeBudgetVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if(pmn == NULL) {
            LogPrint("mnbudget", "mvote - unknown masternode - vin: %s\n", vote.vin.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }


        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(!vote.IsValid(true)){
            LogPrintf("mvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if(UpdateProposal(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedBudgetItem(vote.GetHash());
        }

        LogPrintf("mvote - new budget vote - %s\n", vote.GetHash().ToString());
    }

}

//todo - 12.1 - terrible name - maybe DoesObjectExist?
bool CGovernanceManager::PropExists(uint256 nHash)
{
    if(mapProposals.count(nHash)) return true;
    return false;
}

// description: incremental sync with our peers
// note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
// note: 12.1 - remove
// void CGovernanceManager::ResetSync()
// {
//     LOCK(cs);

//     std::map<uint256, std::map<uint256, CBudgetVote> >::iterator it1 = mapVotes.begin();
//     while(it1 != mapVotes.end()){
//         (*it1).second.second.fSynced = false;
//         ++it1;
//     }
// }

// description: incremental sync with our peers
// note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
// note: 12.1 - remove
// void CGovernanceManager::MarkSynced()
// {
//     LOCK(cs);

//     /*
//         Mark that we've sent all valid items
//     */

//     // this could screw up syncing, so let's log it
//     LogPrintf("CGovernanceManager::MarkSynced\n");

//     std::map<uint256, std::map<uint256, CBudgetVote> >::iterator it1 = mapVotes.begin();
//     while(it1 != mapVotes.end()){
//         if((*it1).second.second.fValid)
//             (*it1).second.second.fSynced = true;
//         ++it1;
//     }
// }


void CGovernanceManager::Sync(CNode* pfrom, uint256 nProp)
{
    LOCK(cs);

    /*
        note 12.1 - fix comments below

        Sync with a client on the network

        --

        This code checks each of the hash maps for all known budget proposals and finalized budget proposals, then checks them against the
        budget object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;

    // sync gov objects
    std::map<uint256, CBudgetProposal>::iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end()){
        CBudgetProposal* pbudgetProposal = FindProposal((*it1).first);
        if(pbudgetProposal && pbudgetProposal->fValid && ((nProp == uint256() || ((*it1).first == nProp)))){
            // Push the inventory budget proposal message over to the other client
            pfrom->PushInventory(CInv(MSG_BUDGET_PROPOSAL, (*it1).second.GetHash()));
            nInvCount++;
        }
        ++it1;
    }

    // sync votes
    std::map<uint256, std::map<uint256, CBudgetVote> >::iterator it2 = mapVotes.begin();
    while(it2 != mapVotes.end()){
        std::map<uint256, CBudgetVote>::iterator it3 = mapVotes[(*it2).first].begin();
        if((*it3).second.fValid && ((nProp == uint256() || ((*it2).first == nProp))))
                pfrom->PushInventory(CInv(MSG_BUDGET_VOTE, (*it3).second.GetHash()));
                nInvCount++;
            }
        ++it2;
    }

    pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_BUDGET_PROP, nInvCount);

    LogPrintf("CGovernanceManager::Sync - sent %d items\n", nInvCount);
}

bool CGovernanceManager::UpdateProposal(CBudgetVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if(!mapProposals.count(vote.nProposalHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CGovernanceManager::UpdateProposal - Unknown proposal %d, asking for source proposal\n", vote.nProposalHash.ToString());
            mapOrphanMasternodeBudgetVotes[vote.nProposalHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nProposalHash)){
                pfrom->PushMessage(NetMsgType::MNBUDGETVOTESYNC, vote.nProposalHash);
                askedForSourceProposalOrBudget[vote.nProposalHash] = GetTime();
            }
        }

        strError = "Proposal not found!";
        return false;
    }


    return AddOrUpdateVote(vote, strError);
}

bool CGovernanceManager::AddOrUpdateVote(CBudgetVote& vote, std::string& strError)
{
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();

    // todo - need to rewrite 
    //        check for first vote, create structure
    //        clear out votes if the budgets are invalid

    if(mapVotes.count(hash)){
        if(mapVotes[hash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
            LogPrint("mnbudget", "CBudgetProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash].nTime < BUDGET_VOTE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime);
            LogPrint("mnbudget", "CBudgetProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    mapVotes[hash] = vote;
    return true;
}

CBudgetProposal::CBudgetProposal()
{
    strProposalName = "unknown";
    nBlockStart = 0;
    nBlockEnd = 0;
    nAmount = 0;
    nTime = 0;
    fValid = true;
}

CBudgetProposal::CBudgetProposal(const CBudgetProposal& other)
{
    strProposalName = other.strProposalName;
    strURL = other.strURL;
    nBlockStart = other.nBlockStart;
    nBlockEnd = other.nBlockEnd;
    address = other.address;
    nAmount = other.nAmount;
    nTime = other.nTime;
    nFeeTXHash = other.nFeeTXHash;
    mapVotes = other.mapVotes;
    fValid = true;
}

CBudgetProposal::CBudgetProposal(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strURL = strURLIn;

    nBlockStart = nBlockStartIn;

    int nPaymentsStart = nBlockStart - nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    //calculate the end of the cycle for this vote, add half a cycle (vote will be deleted after that block)
    nBlockEnd = nPaymentsStart + Params().GetConsensus().nBudgetPaymentsCycleBlocks * nPaymentCount;

    address = addressIn;
    nAmount = nAmountIn;

    nFeeTXHash = nFeeTXHashIn;
}

bool CBudgetProposal::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    if(GetNoCount() - GetYesCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
         strError = "Active removal";
         return false;
    }

    if(nBlockStart < 0) {
        strError = "Invalid Proposal";
        return false;
    }

    if(!pindex) {
        strError = "Tip is NULL";
        return true;
    }

    if(nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks != 0){
        int nNext = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
        strError = strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nNext);
        return false;
    }

    if(nBlockEnd % Params().GetConsensus().nBudgetPaymentsCycleBlocks != Params().GetConsensus().nBudgetPaymentsCycleBlocks/2){
        strError = "Invalid block end";
        return false;
    }

    if(nBlockEnd < nBlockStart) {
        strError = "Invalid block end - must be greater then block start.";
        return false;
    }

    if(nAmount < 1*COIN) {
        strError = "Invalid proposal amount";
        return false;
    }

    if(strProposalName.size() > 20) {
        strError = "Invalid proposal name, limit of 20 characters.";
        return false;
    }

    if(strProposalName != SanitizeString(strProposalName)) {
        strError = "Invalid proposal name, unsafe characters found.";
        return false;
    }

    if(strURL.size() > 64) {
        strError = "Invalid proposal url, limit of 64 characters.";
        return false;
    }

    if(strURL != SanitizeString(strURL)) {
        strError = "Invalid proposal url, unsafe characters found.";
        return false;
    }

    if(address == CScript()) {
        strError = "Invalid proposal Payment Address";
        return false;
    }

    // 12.1 - add valid predicates
    //     this can be handled by configuration 
    // found = false
    // if strProposalName[:10] = "proposal="; found = true;
    // if strProposalName[:10] = "contract="; found = true;
    // if strProposalName[:10] = "project="; found = true;
    // if strProposalName[:10] = "employee="; found = true;
    // if strProposalName[:10] = "project-milestone="; found = true;
    // if strProposalName[:10] = "project-report="; found = true;

    // if not found: return false

    // automatically expire
    // if(GetTime() > nExpirationTime) return false;

    if(fCheckCollateral){
        int nConf = 0;
        if(!IsCollateralValid(nFeeTXHash, GetHash(), strError, nTime, nConf, GOVERNANCE_FEE_TX)){
            return false;
        }
    }

    /*
        TODO: There might be an issue with multisig in the coinbase on mainnet, we will add support for it in a future release.
    */
    if(address.IsPayToScriptHash()) {
        strError = "Multisig is not currently supported.";
        return false;
    }

    // -- If GetAbsoluteYesCount is more than -10% of the network, flag as invalid
    if(GetAbsoluteYesCount() < -(mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10)) {
        strError = "Voted Down";
        return false;
    }

    // todo 12.1 - setup a hard limit via spork or something? Maybe up to 1/4 of the total budget?
    //can only pay out 10% of the possible coins (min value of coins)
    if(nAmount > budget.GetTotalBudget(nBlockStart)) {
        strError = "Payment more than max";
        return false;
    }

    //* 
    //nAmount can be - and +, allows increasing budget dynamically. This is R&D mode
    //*

    if(GetBlockEnd() + Params().GetConsensus().nBudgetPaymentsWindowBlocks < pindex->nHeight) return false;

    return true;
}

bool CBudgetProposal::NetworkWillPay()
{
    /**
    * vote nVoteType 1 to 10
    * --------------------------
    *
    * 
    * // note: if the vote is not passing and we're before the starttime, it's invalid
    * // note: plain english version - if funding votes nocount > funding votes yescount and GetTime() < nStartTime: return false
    * if votecount(VOTE_TYPE_FUNDING, "no") > votecount(VOTE_TYPE_FUNDING, "yes") && GetTime() < nStartTime: return false
    */

}

bool CBudgetProposal::IsEstablished() {
    //Proposals must be established to make it into a budget
    return (nTime < GetTime() - Params().GetConsensus().nBudgetProposalEstablishingTime);
}

// If masternode voted for a proposal, but is now invalid -- remove the vote
void CBudgetProposal::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        (*it).second.fValid = (*it).second.IsValid(fSignatureCheck);
        ++it;
    }
}

double CBudgetProposal::GetRatio()
{
    int yeas = 0;
    int nays = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();

    while(it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES) yeas++;
        if ((*it).second.nVote == VOTE_NO) nays++;
        ++it;
    }

    if(yeas+nays == 0) return 0.0f;

    return ((double)(yeas) / (double)(yeas+nays));
}

int CBudgetProposal::GetAbsoluteYesCount()
{
    return GetYesCount() - GetNoCount();
}

int CBudgetProposal::GetYesCount()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_YES && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetNoCount()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_NO && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetAbstainCount()
{
    int ret = 0;

    std::map<uint256, CBudgetVote>::iterator it = mapVotes.begin();
    while(it != mapVotes.end()){
        if ((*it).second.nVote == VOTE_ABSTAIN && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CBudgetProposal::GetBlockStartCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockStart - nBlockStart % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CBudgetProposal::GetBlockCurrentCycle(const CBlockIndex* pindex)
{
    if(!pindex) return -1;

    if(pindex->nHeight >= GetBlockEndCycle()) return -1;

    return pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CBudgetProposal::GetBlockEndCycle()
{
    //end block is half way through the next cycle (so the proposal will be removed much after the payment is sent)

    return nBlockEnd - Params().GetConsensus().nBudgetPaymentsCycleBlocks/2;
}

int CBudgetProposal::GetTotalPaymentCount()
{
    return (GetBlockEndCycle() - GetBlockStartCycle()) / Params().GetConsensus().nBudgetPaymentsCycleBlocks;
}

int CBudgetProposal::GetRemainingPaymentCount(int nBlockHeight)
{
    int nPayments = 0;
    // printf("-> Budget Name : %s\n", strProposalName.c_str());
    // printf("------- nBlockStart : %d\n", nBlockStart);
    // printf("------- nBlockEnd : %d\n", nBlockEnd);
    while(nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks < GetBlockEndCycle())
    {
        // printf("------- P : %d %d - %d < %d - %d\n", nBlockHeight, nPayments, nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks, nBlockEnd, nBlockHeight + Params().GetConsensus().nBudgetPaymentsCycleBlocks < nBlockEnd);
        nBlockHeight += Params().GetConsensus().nBudgetPaymentsCycleBlocks;
        nPayments++;
    }
    return nPayments;
}

void CBudgetProposal::Relay()
{
    CInv inv(MSG_BUDGET_PROPOSAL, GetHash());
    RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
}

std::string CGovernanceManager::ToString() const
{
    std::ostringstream info;

    info << "Proposals: " << (int)mapProposals.size() <<
            ", Seen Budgets: " << (int)mapSeenMasternodeBudgetProposals.size() <<
            ", Seen Budget Votes: " << (int)mapSeenMasternodeBudgetVotes.size();

    return info.str();
}

void CGovernanceManager::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mnbudget", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

// 12.1 - add Priority sorting, add NetworkWillPay
std::vector<CBudgetProposal*> CBudgetManager::GetBudget()
{
    LOCK(cs);

    // ------- Sort budgets by Yes Count

    std::vector<std::pair<CBudgetProposal*, int> > vBudgetPorposalsSort;

    std::map<uint256, CBudgetProposal>::iterator it = governance.mapProposals.begin();
    while(it != governance.mapProposals.end()){
        (*it).second.CleanAndRemove(false);
        vBudgetPorposalsSort.push_back(make_pair(&((*it).second), (*it).second.GetYesCount()-(*it).second.GetNoCount()));
        ++it;
    }

    // 12.1 -- add priority
    std::sort(vBudgetPorposalsSort.begin(), vBudgetPorposalsSort.end(), sortProposalsByVotes());

    // ------- Grab The Budgets In Order

    std::vector<CBudgetProposal*> vBudgetProposalsRet;

    CAmount nBudgetAllocated = 0;
    if(!pCurrentBlockIndex) return vBudgetProposalsRet;

    int nBlockStart = pCurrentBlockIndex->nHeight - pCurrentBlockIndex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    int nBlockEnd  =  nBlockStart + Params().GetConsensus().nBudgetPaymentsWindowBlocks;
    CAmount nTotalBudget = GetTotalBudget(nBlockStart);


    std::vector<std::pair<CBudgetProposal*, int> >::iterator it2 = vBudgetPorposalsSort.begin();
    while(it2 != vBudgetPorposalsSort.end())
    {
        CBudgetProposal* pbudgetProposal = (*it2).first;

        // 12.1 - skip if NetworkWillPay == false


        printf("-> Budget Name : %s\n", pbudgetProposal->strProposalName.c_str());
        printf("------- nBlockStart : %d\n", pbudgetProposal->nBlockStart);
        printf("------- nBlockEnd : %d\n", pbudgetProposal->nBlockEnd);
        printf("------- nBlockStart2 : %d\n", nBlockStart);
        printf("------- nBlockEnd2 : %d\n", nBlockEnd);

        printf("------- 1 : %d\n", pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart);
        printf("------- 2 : %d\n", pbudgetProposal->nBlockEnd >= nBlockEnd);
        printf("------- 3 : %d\n", pbudgetProposal->GetYesCount() - pbudgetProposal->GetNoCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10);
        printf("------- 4 : %d\n", pbudgetProposal->IsEstablished());

        //prop start/end should be inside this period
        if(pbudgetProposal->fValid && pbudgetProposal->nBlockStart <= nBlockStart &&
                pbudgetProposal->nBlockEnd >= nBlockEnd &&
                pbudgetProposal->GetYesCount() - pbudgetProposal->GetNoCount() > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10 && 
                pbudgetProposal->IsEstablished())
        {
            printf("------- In range \n");

            if(pbudgetProposal->GetAmount() + nBudgetAllocated <= nTotalBudget) {
                pbudgetProposal->SetAllotted(pbudgetProposal->GetAmount());
                nBudgetAllocated += pbudgetProposal->GetAmount();
                vBudgetProposalsRet.push_back(pbudgetProposal);
                printf("------- YES \n");
            } else {
                pbudgetProposal->SetAllotted(0);
            }
        }

        ++it2;
    }

    return vBudgetProposalsRet;
}

