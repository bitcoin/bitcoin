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
#include "governance.h"
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
std::vector<CGovernanceObject> vecImmatureBudgetProposals;

int nSubmittedFinalBudget;

bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf, CAmount minFee)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if(!GetTransaction(nTxCollateralHash, txCollateral, Params().GetConsensus(), nBlockHash, true)){
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
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
            LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
            return false;
        }
        if(o.scriptPubKey == findScript && o.nValue >= minFee) foundOpReturn = true;

    }
    if(!foundOpReturn){
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s\n", strError);
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
        LogPrintf ("CGovernanceObject::IsCollateralValid - %s - %d confirmations\n", strError, conf);
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

bool CGovernanceManager::AddProposal(CGovernanceObject& budgetProposal)
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

    std::map<uint256, CGovernanceObject>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end())
    {
        CGovernanceObject* pbudgetProposal = &((*it2).second);
        pbudgetProposal->fValid = pbudgetProposal->IsValid(pCurrentBlockIndex, strError);
        ++it2;
    }
}

CGovernanceObject *CGovernanceManager::FindProposal(const std::string &strName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CGovernanceObject* pbudgetProposal = NULL;

    std::map<uint256, CGovernanceObject>::iterator it = mapProposals.begin();
    while(it != mapProposals.end()){
        if((*it).second.strName == strName && (*it).second.GetYesCount(VOTE_ACTION_FUNDING) > nYesCount){
            pbudgetProposal = &((*it).second);
            nYesCount = pbudgetProposal->GetYesCount(VOTE_ACTION_FUNDING);
        }
        ++it;
    }

    if(nYesCount == -99999) return NULL;

    return pbudgetProposal;
}

CGovernanceObject *CGovernanceManager::FindProposal(uint256 nHash)
{
    LOCK(cs);

    if(mapProposals.count(nHash))
        return &mapProposals[nHash];

    return NULL;
}

std::vector<CGovernanceObject*> CGovernanceManager::GetAllProposals(int64_t nMoreThanTime)
{
    LOCK(cs);

    std::vector<CGovernanceObject*> vBudgetProposalRet;

    std::map<uint256, CGovernanceObject>::iterator it = mapProposals.begin();
    while(it != mapProposals.end())
    {
        if((*it).second.nTime < nMoreThanTime) {
            ++it;
            continue;
        }
        (*it).second.CleanAndRemove(false);

        CGovernanceObject* pbudgetProposal = &((*it).second);
        vBudgetProposalRet.push_back(pbudgetProposal);

        ++it;
    }

    return vBudgetProposalRet;
}

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

    std::map<uint256, CGovernanceObject>::iterator it2 = mapProposals.begin();
    while(it2 != mapProposals.end()){
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    std::vector<CGovernanceObject>::iterator it4 = vecImmatureBudgetProposals.begin();
    while(it4 != vecImmatureBudgetProposals.end())
    {
        std::string strError = "";
        int nConf = 0;
        if(!IsCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf, GOVERNANCE_FEE_TX)){
            ++it4;
            continue;
        }

        // 12.1 -- fix below
        // if(!(*it4).IsValid(pCurrentBlockIndex, strError)) {
        //     LogPrintf("mprop (immature) - invalid budget proposal - %s\n", strError);
        //     it4 = vecImmatureBudgetProposals.erase(it4); 
        //     continue;
        // }

        // CGovernanceObject budgetProposal((*it4));
        // if(AddProposal(budgetProposal)) {(*it4).Relay();}

        // LogPrintf("mprop (immature) - new budget - %s\n", (*it4).GetHash().ToString());
        // it4 = vecImmatureBudgetProposals.erase(it4); 
    }

}

void CGovernanceManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if(fLiteMode) return;
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_budget);

    // todo - 12.1 - change to MNGOVERNANCEVOTESYNC
    if (strCommand == NetMsgType::mngovernanceVOTESYNC) { //Masternode vote sync
        uint256 nProp;
        vRecv >> nProp;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(nProp == uint256()) {
                if(pfrom->HasFulfilledRequest(NetMsgType::mngovernanceVOTESYNC)) {
                    LogPrintf("mnvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest(NetMsgType::mngovernanceVOTESYNC);
            }
        }

        // ask for a specific proposal and it's votes
        Sync(pfrom, nProp);
        LogPrintf("mnvs - Sent Masternode votes to %s\n", pfrom->addr.ToString());
    }

    // todo - 12.1 - change to MNGOVERNANCEPROPOSAL
    if (strCommand == NetMsgType::mngovernancePROPOSAL) { //Masternode Proposal
        CGovernanceObject budgetProposalBroadcast;
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

        if(AddProposal(budgetProposalBroadcast))
        {
            budgetProposalBroadcast.Relay();
        }
        masternodeSync.AddedBudgetItem(budgetProposalBroadcast.GetHash());

        LogPrintf("mprop - new budget - %s\n", budgetProposalBroadcast.GetHash().ToString());
        //We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    // todo - 12.1 - change to MNGOVERNANCEVOTE
    if (strCommand == NetMsgType::mngovernanceVOTE) { //Masternode Vote
        CBudgetVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if(mapSeenMasternodeBudgetVotes.count(vote.GetHash())){
            masternodeSync.AddedBudgetItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vinMasternode);
        if(pmn == NULL) {
            LogPrint("mngovernance", "mvote - unknown masternode - vin: %s\n", vote.vinMasternode.ToString());
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
            return;
        }


        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        if(!vote.IsValid(true)){
            LogPrintf("mvote - signature invalid\n");
            if(masternodeSync.IsSynced()) Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vinMasternode);
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
    std::map<uint256, CGovernanceObject>::iterator it1 = mapSeenMasternodeBudgetProposals.begin();
    while(it1 != mapSeenMasternodeBudgetProposals.end()){
        CGovernanceObject* pbudgetProposal = FindProposal((*it1).first);
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
        {
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

    if(!mapProposals.count(vote.nParentHash)){
        if(pfrom){
            // only ask for missing items after our syncing process is complete -- 
            //   otherwise we'll think a full sync succeeded when they return a result
            if(!masternodeSync.IsSynced()) return false;

            LogPrintf("CGovernanceManager::UpdateProposal - Unknown proposal %d, asking for source proposal\n", vote.nParentHash.ToString());
            mapOrphanMasternodeBudgetVotes[vote.nParentHash] = vote;

            if(!askedForSourceProposalOrBudget.count(vote.nParentHash)){
                pfrom->PushMessage(NetMsgType::mngovernanceVOTESYNC, vote.nParentHash);
                askedForSourceProposalOrBudget[vote.nParentHash] = GetTime();
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

    uint256 hash = vote.vinMasternode.prevout.GetHash();
    uint256 hash2 = vote.nParentHash;

    if(mapVotes[hash2].count(hash)){
        if(mapVotes[hash2][hash].nTime > vote.nTime){
            strError = strprintf("new vote older than existing vote - %s", vote.GetHash().ToString());
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if(vote.nTime - mapVotes[hash2][hash].nTime < BUDGET_VOTE_UPDATE_MIN){
            strError = strprintf("time between votes is too soon - %s - %lli", vote.GetHash().ToString(), vote.nTime - mapVotes[hash2][hash].nTime);
            LogPrint("mngovernance", "CGovernanceObject::AddOrUpdateVote - %s\n", strError);
            return false;
        }
    }

    mapVotes[hash2][hash] = vote;
    return true;
}

CGovernanceObject::CGovernanceObject()
{
    strName = "unknown";
    nTime = 0;

    nHashParent = uint256(); //parent object, 0 is root
    nRevision = 0; //object revision in the system
    nFeeTXHash = uint256(); //fee-tx
    
    // caching
    fValid = false;
    nHash = uint256();
}

CGovernanceObject::CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTime, uint256 nFeeTXHashIn)
{
    strName = "unknown";
    nTime = 0;

    nHashParent = nHashParentIn; //parent object, 0 is root
    // nPriority = nPriorityIn;
    nRevision = nRevisionIn; //object revision in the system
    nFeeTXHash = nFeeTXHashIn; //fee-tx    
    // nTypeVersion = nTypeVersionIn;
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other)
{
    strName = other.strName;
    nTime = other.nTime;

    nHashParent = other.nHashParent; //parent object, 0 is root
    // nPriority = other.nPriorityIn;
    nRevision = other.nRevision; //object revision in the system
    nFeeTXHash = other.nFeeTXHash; //fee-tx    
    // nTypeVersion = other.nTypeVersion;
    //??
    mapRegister = other.mapRegister;
}

bool CGovernanceObject::IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral)
{
    if(GetNoCount(VOTE_ACTION_VALID) - GetYesCount(VOTE_ACTION_VALID) > mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10){
         strError = "Automated removal";
         return false;
    }

    // if(nEndTime < GetTime()) {
    //     strError = "Expired Proposal";
    //     return false;
    // }

    if(!pindex) {
        strError = "Tip is NULL";
        return true;
    }

    // if(nAmount < 10000) {
    //     strError = "Invalid proposal amount (minimum 10000)";
    //     return false;
    // }

    if(strName.size() > 20) {
        strError = "Invalid proposal name, limit of 20 characters.";
        return false;
    }

    if(strName != SanitizeString(strName)) {
        strError = "Invalid proposal name, unsafe characters found.";
        return false;
    }

    // if(address == CScript()) {
    //     strError = "Invalid proposal Payment Address";
    //     return false;
    // }

    // 12.1 - add valid predicates
    //     this can be handled by configuration 
    // found = false
    // if strName[:10] = "proposal="; found = true;
    // if strName[:10] = "contract="; found = true;
    // if strName[:10] = "project="; found = true;
    // if strName[:10] = "employee="; found = true;
    // if strName[:10] = "project-milestone="; found = true;
    // if strName[:10] = "project-report="; found = true;

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
    // if(address.IsPayToScriptHash()) {
    //     strError = "Multisig is not currently supported.";
    //     return false;
    // }

    // 12.1 move to pybrain
    // //can only pay out 10% of the possible coins (min value of coins)
    // if(nAmount > budget.GetTotalBudget(nBlockStart)) {
    //     strError = "Payment more than max";
    //     return false;
    // }

    return true;
}

bool CGovernanceObject::NetworkWillPay()
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

    std::string strError = "";

    // -- If GetAbsoluteYesCount is more than -10% of the network, flag as invalid
    // if(GetAbsoluteYesCount() < -(mnodeman.CountEnabled(MIN_BUDGET_PEER_PROTO_VERSION)/10)) {
    //     strError = "Voted Down";
    //     return false;
    // }

    return true;
}

bool CGovernanceObject::IsEstablished() {
    //Proposals must be established to make it into a budget
    return (nTime < GetTime() - Params().GetConsensus().nBudgetProposalEstablishingTime);
}

void CGovernanceObject::CleanAndRemove(bool fSignatureCheck) {
    // TODO: do smth here
}

void CGovernanceManager::CleanAndRemove(bool fSignatureCheck)
{
    /*
    *   
    *   Loop through each item and delete the items that have become invalid
    *
    */

    std::map<uint256, std::map<uint256, CBudgetVote> >::iterator it2 = mapVotes.begin();
    while(it2 != mapVotes.end()){
        std::map<uint256, CBudgetVote>::iterator it3 = mapVotes[(*it2).first].begin();
        while(it3 != mapVotes[(*it2).first].end())
        {
            if(!(*it3).second.IsValid(fSignatureCheck))
            {
                // 12.1 - log to specialized handle (govobj?)
                LogPrintf("CGovernanceManager::CleanAndRemove - Proposal/Budget is known, activating and removing orphan vote\n");
                mapVotes[(*it2).first].erase(it3++);
            } else {
                ++it3;
            }
        }
        ++it2;
    }
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(int nVoteOutcomeIn)
{
    return governance.CountMatchingVotes(nVoteOutcomeIn, VOTE_OUTCOME_YES) - governance.CountMatchingVotes(nVoteOutcomeIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetYesCount(int nVoteOutcomeIn)
{
    return governance.CountMatchingVotes(nVoteOutcomeIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(int nVoteOutcomeIn)
{
    return governance.CountMatchingVotes(nVoteOutcomeIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(int nVoteOutcomeIn)
{
    return governance.CountMatchingVotes(nVoteOutcomeIn, VOTE_OUTCOME_ABSTAIN);
}

void CGovernanceObject::Relay()
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
    LogPrint("mngovernance", "pCurrentBlockIndex->nHeight: %d\n", pCurrentBlockIndex->nHeight);

    if(!fLiteMode && masternodeSync.RequestedMasternodeAssets > MASTERNODE_SYNC_LIST)
        NewBlock();
}

int CGovernanceManager::CountMatchingVotes(int nVoteTypeIn, int nVoteOutcomeIn)
{
    /*
    *   
    *   Count matching votes and return
    *
    */

    int nMatching = 0;

    std::map<uint256, std::map<uint256, CBudgetVote> >::iterator it2 = mapVotes.begin();
    while(it2 != mapVotes.end()){
        std::map<uint256, CBudgetVote>::iterator it3 = mapVotes[(*it2).first].begin();
        while(it3 != mapVotes[(*it2).first].end())
        {
            if(!(*it3).second.IsValid(true))
            {
                // if((*it3).second.nVoteType == nVoteTypeIn && 
                //     (*it3).second.nVoteOutcome == nVoteOutcomeIn)
                //         nMatching++;
            } else {
                ++it3;
            }
        }
        ++it2;
    }

    return nMatching;
}

// template<typename T>
// bool CGovernanceManager::AddRegister(T& newValue)
// {

// }
