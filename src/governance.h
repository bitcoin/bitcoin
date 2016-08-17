// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERANCE_H
#define GOVERANCE_H

//#define ENABLE_DASH_DEBUG

#include "util.h"
#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include "governance-vote.h"
#include "masternodeman.h"
#include <boost/lexical_cast.hpp>
#include "init.h"
#include <univalue.h>
#include "utilstrencodings.h"

#include <stdio.h>
#include <string.h>

using namespace std;

static const int GOVERNANCE_OBJECT_UNKNOWN = 0;
static const int GOVERNANCE_OBJECT_PROPOSAL = 1;
static const int GOVERNANCE_OBJECT_TRIGGER = 2;

extern CCriticalSection cs_budget;

class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class CNode;

static const CAmount GOVERNANCE_FEE_TX = (0.1*COIN);
static const int64_t GOVERNANCE_FEE_CONFIRMATIONS = 1; //todo 12.1 -- easy testing
static const int64_t GOVERNANCE_UPDATE_MIN = 60*60;

extern std::map<uint256, int64_t> mapAskedForGovernanceObject;
extern CGovernanceManager governance;

// FOR SEEN MAP ARRAYS - GOVERNANCE OBJECTS AND VOTES
static const int SEEN_OBJECT_IS_VALID = 0;
static const int SEEN_OBJECT_ERROR_INVALID = 1;
static const int SEEN_OBJECT_ERROR_IMMATURE = 2;
static const int SEEN_OBJECT_EXECUTED = 3; //used for triggers
static const int SEEN_OBJECT_UNKNOWN = 4; // the default

//Check the collateral transaction for the budget proposal/finalized budget
extern bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int& nConf, CAmount minFee);


//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager
{
public: // Types

    typedef std::map<uint256, CGovernanceObject> object_m_t;

    typedef object_m_t::iterator object_m_it;

    typedef object_m_t::const_iterator object_m_cit;

    typedef std::map<uint256, int> count_m_t;

    typedef count_m_t::iterator count_m_it;

    typedef count_m_t::const_iterator count_m_cit;

    typedef std::map<uint256, CGovernanceVote> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

    typedef std::map<uint256, CTransaction> transaction_m_t;

    typedef transaction_m_t::iterator transaction_m_it;

    typedef transaction_m_t::const_iterator transaction_m_cit;

    typedef object_m_t::size_type size_type;

private:

    //hold txes until they mature enough to use
    transaction_m_t mapCollateral;
    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    int64_t nTimeLastDiff;
    int nCachedBlockHeight;

    // keep track of the scanning errors
    object_m_t mapObjects;

    // note: move to private for better encapsulation 
    count_m_t mapSeenGovernanceObjects;
    count_m_t mapSeenVotes;
    vote_m_t mapOrphanVotes;

    // todo: one of these should point to the other
    //   -- must be carefully managed while adding/removing/updating
    vote_m_t mapVotesByHash;
    vote_m_t mapVotesByType;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    
    CGovernanceManager()
        : mapCollateral(),
          pCurrentBlockIndex(NULL),
          nTimeLastDiff(0),
          nCachedBlockHeight(0),
          mapObjects(),
          cs()
    {}

    void ClearSeen() {
        LOCK(cs);
        mapSeenGovernanceObjects.clear();
        mapSeenVotes.clear();
    }

    int CountProposalInventoryItems()
    {
        return mapSeenGovernanceObjects.size() + mapSeenVotes.size();
    }

    int sizeProposals() {return (int)mapObjects.size();}

    void Sync(CNode* node, uint256 nProp);
    void SyncParentObjectByVote(CNode* pfrom, const CGovernanceVote& vote);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();

    CGovernanceObject *FindGovernanceObject(const std::string &strName);
    CGovernanceObject *FindGovernanceObject(const uint256& nHash);

    std::vector<CGovernanceVote*> GetMatchingVotes(const uint256& nParentHash);
    std::vector<CGovernanceObject*> GetAllNewerThan(int64_t nMoreThanTime);

    int CountMatchingVotes(CGovernanceObject& govobj, int nVoteSignalIn, int nVoteOutcomeIn);

    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool AddGovernanceObject (CGovernanceObject& govobj);
    bool AddOrUpdateVote(const CGovernanceVote& vote, CNode* pfrom, std::string& strError);

    std::string GetRequiredPaymentsString(int nBlockHeight);
    void CleanAndRemove(bool fSignatureCheck);
    void UpdateCachesAndClean();
    void CheckAndRemove() {UpdateCachesAndClean();}

    void CheckOrphanVotes();

    void Clear(){
        LOCK(cs);

        LogPrint("gobject", "Governance object manager was cleared\n");
        mapObjects.clear();
        mapSeenGovernanceObjects.clear();
        mapSeenVotes.clear();
        mapOrphanVotes.clear();
        mapVotesByType.clear();
        mapVotesByHash.clear();
    }
    
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        LOCK(cs);
        READWRITE(mapSeenGovernanceObjects);
        READWRITE(mapSeenVotes);
        READWRITE(mapOrphanVotes);
        READWRITE(mapObjects);
        READWRITE(mapVotesByHash);
        READWRITE(mapVotesByType);
    }

    void UpdatedBlockTip(const CBlockIndex *pindex);
    int64_t GetLastDiffTime() {return nTimeLastDiff;}
    void UpdateLastDiffTime(int64_t nTimeIn) {nTimeLastDiff=nTimeIn;}

    int GetCachedBlockHeight() { return nCachedBlockHeight; }

    // Accessors for thread-safe access to maps
    bool HaveObjectForHash(uint256 nHash);

    bool HaveVoteForHash(uint256 nHash);

    // int GetVoteCountByHash(uint256 nHash);

    bool SerializeObjectForHash(uint256 nHash, CDataStream& ss);

    bool SerializeVoteForHash(uint256 nHash, CDataStream& ss);

    void AddSeenGovernanceObject(uint256 nHash, int status);

    void AddSeenVote(uint256 nHash, int status);

};

/**
* Governance Object
*
*/

class CGovernanceObject
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:

    uint256 nHashParent; //parent object, 0 is root
    int nRevision; //object revision in the system
    std::string strName; //org name, username, prop name, etc. 
    int64_t nTime; //time this object was created
    uint256 nCollateralHash; //fee-tx
    std::string strData; // Data field - can be used for anything
    int nObjectType;

    bool fCachedLocalValidity; // is valid by blockchain 
    std::string strLocalValidityError;

    // VARIOUS FLAGS FOR OBJECT / SET VIA MASTERNODE VOTING

    bool fCachedFunding; // true == minimum network support has been reached for this object to be funded (doesn't mean it will for sure though)
    bool fCachedValid; // true == minimum network has been reached flagging this object as a valid and understood goverance object (e.g, the serialized data is correct format, etc)
    bool fCachedDelete; // true == minimum network support has been reached saying this object should be deleted from the system entirely
    bool fCachedEndorsed; // true == minimum network support has been reached flagging this object as endorsed by an elected representative body (e.g. business review board / technecial review board /etc)
    bool fDirtyCache; // object was updated and cached values should be updated soon
    bool fUnparsable; // data field was unparsible, object will be rejected
    bool fExpired; // Object is no longer of interest

    CGovernanceObject();
    CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, std::string strNameIn, int64_t nTime, uint256 nCollateralHashIn, std::string strDataIn);
    CGovernanceObject(const CGovernanceObject& other);
    void swap(CGovernanceObject& first, CGovernanceObject& second); // nothrow

    // CORE OBJECT FUNCTIONS

    bool IsValidLocally(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral);
    void UpdateLocalValidity(const CBlockIndex *pCurrentBlockIndex);
    void UpdateSentinelVariables(const CBlockIndex *pCurrentBlockIndex);
    int GetObjectType();
    int GetObjectSubtype();
    std::string GetName() {return strName; }

    UniValue GetJSONObject();

    void Relay();
    uint256 GetHash();

    // GET VOTE COUNT FOR SIGNAL

    int GetAbsoluteYesCount(int nVoteSignalIn);
    int GetAbsoluteNoCount(int nVoteSignalIn);
    int GetYesCount(int nVoteSignalIn);
    int GetNoCount(int nVoteSignalIn);
    int GetAbstainCount(int nVoteSignalIn);

    // FUNCTIONS FOR DEALING WITH DATA STRING 

    void LoadData();
    bool SetData(std::string& strError, std::string strDataIn);
    bool GetData(UniValue& objResult);
    std::string GetDataAsHex();
    std::string GetDataAsString();

    // SERIALIZER

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        // SERIALIZE DATA FOR SAVING/LOADING OR NETWORK FUNCTIONS

        READWRITE(nHashParent);
        READWRITE(nRevision);
        READWRITE(LIMITED_STRING(strName, 64));
        READWRITE(nTime);
        READWRITE(nCollateralHash);
        READWRITE(strData);
        READWRITE(nObjectType);

        // AFTER DESERIALIZATION OCCURS, CACHED VARIABLES MUST BE CALCULATED MANUALLY
    }
};


#endif
