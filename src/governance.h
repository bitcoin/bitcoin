// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERANCE_H
#define GOVERANCE_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include "governance-vote.h"
#include <boost/lexical_cast.hpp>
#include "init.h"

#include <stdio.h>
#include <string.h>

using namespace std;
extern CCriticalSection cs_budget;

class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class CNode;

static const CAmount GOVERNANCE_FEE_TX = (0.1*COIN);
static const int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static const int64_t GOVERNANCE_UPDATE_MIN = 60*60;

extern std::vector<CGovernanceObject> vecImmatureGovernanceObjects;
extern std::map<uint256, int64_t> mapAskedForGovernanceObject;
extern CGovernanceManager governance;

// FOR SEEN MAP ARRAYS - GOVERNANCE OBJECTS AND VOTES
#define SEEN_OBJECT_IS_VALID          0
#define SEEN_OBJECT_ERROR_INVALID     1
#define SEEN_OBJECT_ERROR_IMMATURE    2

//Check the collateral transaction for the budget proposal/finalized budget
extern bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf, CAmount minFee);


//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager
{
private:

    //hold txes until they mature enough to use
    map<uint256, CTransaction> mapCollateral;
    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    int64_t nTimeLastDiff;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    
    // keep track of the scanning errors I've seen
    map<uint256, CGovernanceObject> mapObjects;

    // todo - 12.1 - move to private for better encapsulation 
    std::map<uint256, int> mapSeenGovernanceObjects;
    std::map<uint256, int> mapSeenVotes;
    std::map<uint256, CGovernanceVote> mapOrphanVotes;
    std::map<uint256, CGovernanceVote> mapVotes;

    CGovernanceManager() {
        mapObjects.clear();
    }

    void ClearSeen() {
        mapSeenGovernanceObjects.clear();
        mapSeenVotes.clear();
    }

    int CountProposalInventoryItems()
    {
        return mapSeenGovernanceObjects.size() + mapSeenVotes.size();
    }

    int sizeProposals() {return (int)mapObjects.size();}

    // description: incremental sync with our peers
    // note: incremental syncing seems excessive, well just have clients ask for specific objects and their votes
    // note: 12.1 - remove
    //void ResetSync();
    //void MarkSynced();
    void Sync(CNode* node, uint256 nProp);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();

    CGovernanceObject *FindProposal(const std::string &strName);
    CGovernanceObject *FindProposal(uint256 nHash);
    
    std::vector<CGovernanceObject*> GetAllProposals(int64_t nMoreThanTime);

    int CountMatchingVotes(int nVoteSignalIn, int nVoteOutcomeIn);

    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool AddGovernanceObject (CGovernanceObject& budgetProposal);
    bool UpdateGovernanceObject(CGovernanceVote& vote, CNode* pfrom, std::string& strError);
    bool AddOrUpdateVote(CGovernanceVote& vote, std::string& strError);
    bool PropExists(uint256 nHash);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void CleanAndRemove(bool fSignatureCheck);
    void CheckAndRemove();

    void CheckOrphanVotes();
    void Clear(){
        LOCK(cs);

        LogPrintf("Budget object cleared\n");
        mapObjects.clear();
        mapSeenGovernanceObjects.clear();
        mapSeenVotes.clear();
        mapOrphanVotes.clear();
    }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapSeenGovernanceObjects);
        READWRITE(mapSeenVotes);
        READWRITE(mapOrphanVotes);
        READWRITE(mapObjects);
        READWRITE(mapVotes);
    }

    void UpdatedBlockTip(const CBlockIndex *pindex);
    int64_t GetLastDiffTime() {return nTimeLastDiff;}
    void UpdateLastDiffTime(int64_t nTimeIn) {nTimeLastDiff=nTimeIn;}
};

/**
* Generic Governance Object
*
*
*/

class CGovernanceObject
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    CAmount nAlloted;

public:

    uint256 nHashParent; //parent object, 0 is root
    int nRevision; //object revision in the system
    std::string strName; //org name, username, prop name, etc. 
    int64_t nTime; //time this object was created
    uint256 nFeeTXHash; //fee-tx
    std::string strData; // Data field - can be used for anything

    // caching -- one per voting mechanism -- see governance-vote.h for more information
    bool fCachedFunding;
    bool fCachedValid;
    bool fCachedDelete;
    bool fCachedClearRegisters;
    bool fCachedEndorsed;
    bool fCachedReleaseBounty1;
    bool fCachedReleaseBounty2;
    bool fCachedReleaseBounty3;

    CGovernanceObject();
    CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTime, uint256 nFeeTXHashIn);
    CGovernanceObject(const CGovernanceObject& other);


    void swap(CGovernanceObject& first, CGovernanceObject& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.strName, second.strName);
        swap(first.nHashParent, second.nHashParent);
        swap(first.nRevision, second.nRevision);
        swap(first.nTime, second.nTime);
        swap(first.nFeeTXHash, second.nFeeTXHash);
        swap(first.strData, second.strData);     

        // swap all cached valid flags
        swap(first.fCachedFunding, second.fCachedFunding);
        swap(first.fCachedValid, second.fCachedValid);
        swap(first.fCachedDelete, second.fCachedDelete);
        swap(first.fCachedClearRegisters, second.fCachedClearRegisters);
        swap(first.fCachedEndorsed, second.fCachedEndorsed);
        swap(first.fCachedReleaseBounty1, second.fCachedReleaseBounty1);
        swap(first.fCachedReleaseBounty2, second.fCachedReleaseBounty2);
        swap(first.fCachedReleaseBounty3, second.fCachedReleaseBounty3);

    }

    bool HasMinimumRequiredSupport();
    bool IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral=true);

    std::string GetName() {return strName; }

    // get vote counts on each outcome
    int GetAbsoluteYesCount(int nVoteSignalIn);
    int GetYesCount(int nVoteSignalIn);
    int GetNoCount(int nVoteSignalIn);
    int GetAbstainCount(int nVoteSignalIn);

    void CleanAndRemove(bool fSignatureCheck);
    void Relay();

    uint256 GetHash(){

        /*
        uint256 nHashParent; //parent object, 0 is root
        int nRevision; //object revision in the system
        std::string strName; //org name, username, prop name, etc. 
        int64_t nTime; //time this object was created
        uint256 nFeeTXHash; //fee-tx
        */

        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strName;
        ss << strData;
        uint256 h1 = ss.GetHash();

        return h1;
    }

    /**
    *   AddRegister - Example usage:
    *   --------------------------------------------------------
    * 
    *   We don't really care what's in these, as long as the masternode network
    *   believes they're accurate. Otherwise the masternodes will vote them down 
    *   and we'll delete them from memory (fee-loss attack). 
    *
    
    */

    bool SetData(std::string& strError, std::string strDataIn)
    {
        if(strDataIn.size() > 512*4)
        {
            strError = "Too big.";
            return false;
        }

        strData = strDataIn;
        return true;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        /**
        *   Store all major data items in serialization for other clients
        *   --
        *
        *
        *    uint256 nHashParent; //parent object, 0 is root
        *    int nRevision; //object revision in the system
        *    std::string strName; //org name, username, prop name, etc. 
        *    int64_t nTime; //time this object was created
        *    uint256 nFeeTXHash; //fee-tx
        *   
        *    // caching
        *    bool fValid;
        *    uint256 nHash;
        *
        *    // Registers, these can be used for anything
        *    //   -- check governance wiki for correct usage
        *    std::map<int, CGovernanceObjectRegister> mapRegister;
        *
        *
        */

        READWRITE(nHashParent);
        READWRITE(nRevision);
        READWRITE(LIMITED_STRING(strName, 64));
        READWRITE(nTime);
        READWRITE(nFeeTXHash);
        READWRITE(strData);
    }
};


#endif
