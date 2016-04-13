// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_FINALIZE_H
#define GOVERNANCE_FINALIZE_H

//todo: which of these do we need?
//#include "main.h"
//#include "sync.h"
//#include "net.h"
//#include "key.h"
//#include "util.h"
//#include "base58.h"
//#include "masternode.h"
#include "governance.h"
#include "governance-types.h"
//#include <boost/lexical_cast.hpp>
#include <univalue.h>


using namespace std;

class CGovernanceManager;

class CTxBudgetPayment;
class CFinalizedBudgetBroadcast;
class CFinalizedBudget;

extern std::vector<CGovernanceNode> vecImmatureGovernanceNodes;

//Check the collateral transaction for the budget proposal/finalized budget
bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf);
std::string PrimaryTypeToString(GovernanceObjectType type);

/*
    GOVERNANCE CLASSES
*/

/** Save Budget Manager (budget.dat)
 */
class CBudgetDB
{
private:
    boost::filesystem::path pathDB;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CBudgetDB();
    bool Write(const CGovernanceManager &objToSave);
    ReadResult Read(CGovernanceManager& objToLoad, bool fDryRun = false);
};


//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager
{   // **** Objects and memory ****

private:

    //hold txes until they mature enough to use
    map<uint256, CTransaction> mapCollateral;
    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    
    // hold governance objects (proposals, contracts, settings and switches)
    std::map<uint256, CGovernanceNode> mapGovernanceObjects;
    // finalized budgets are kept in their own object
    // std::map<uint256, CFinalizedBudget> mapFinalizedBudgets;

    std::map<uint256, CGovernanceNode> mapSeenGovernanceObjects;
    std::map<uint256, CGovernanceVote> mapSeenGovernanceVotes;
    std::map<uint256, CGovernanceVote> mapOrphanGovernanceVotes;
    //std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;

    // VOTES <obj hash < vote hash <  Vote > >
    std::map<uint256, std::map<uint256, CGovernanceVote> > mapVotes;

    // **** Initialization ****

    CGovernanceManager() {
        mapGovernanceObjects.clear();
        //mapFinalizedBudgets.clear();
    }

    void Clear(){
        LOCK(cs);

        LogPrintf("Governance object cleared\n");
        mapGovernanceObjects.clear();
        //mapFinalizedBudgets.clear();
        mapSeenGovernanceObjects.clear();
        mapSeenGovernanceVotes.clear();
        //mapSeenFinalizedBudgets.clear();
        mapOrphanGovernanceVotes.clear();
    }

    void ClearSeen() {
        mapSeenGovernanceObjects.clear();
        mapSeenGovernanceVotes.clear();
        //mapSeenFinalizedBudgets.clear();
    }

    void Sync(CNode* node, uint256 nProp, bool fPartial=false);
    void ResetSync();
    void MarkSynced();

    // **** Search / Statistics / Information ****

    int CountProposalInventoryItems() { return mapSeenGovernanceObjects.size() + mapSeenGovernanceVotes.size(); }
    //int CountFinalizedInventoryItems() { return mapSeenFinalizedBudgets.size(); }

    CGovernanceNode *FindGovernanceObject(const std::string &strName);
    CGovernanceNode *FindGovernanceObject(uint256 nHash);
    //CFinalizedBudget *FindFinalizedBudget(uint256 nHash);
    GovernanceObjectType GetGovernanceTypeByHash(uint256 nHash);
    std::vector<CGovernanceNode*> GetBudget();
    CAmount GetTotalBudget(int nHeight);
    bool HasNextFinalizedBudget(); // Do we have the next finalized budget?
    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    
    std::vector<CGovernanceNode*> FindMatchingGovernanceObjects(GovernanceObjectType type);
    //std::vector<CFinalizedBudget*> GetFinalizedBudgets();
    std::string GetRequiredPaymentsString(int nBlockHeight);
    std::string ToString() const;

    // **** Update ****

    //bool AddFinalizedBudget(CFinalizedBudget& finalizedBudget);
    bool AddGovernanceObject(CGovernanceNode& budgetProposal);
    bool AddOrphanGovernanceVote(CGovernanceVote& vote, CNode* pfrom);
    void CheckAndRemove();
    void CheckOrphanVotes();
    void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees);
    void NewBlock();
    void SubmitFinalBudget();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void UpdatedBlockTip(const CBlockIndex *pindex);
    bool UpdateGovernanceObjectVotes(CGovernanceVote& vote, CNode* pfrom, std::string& strError);

    // **** Serializer ****

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        // READWRITE(mapSeenGovernanceObjects);
        // READWRITE(mapSeenGovernanceVotes);
        // //READWRITE(mapSeenFinalizedBudgets);
        // READWRITE(mapOrphanGovernanceVotes);

        // READWRITE(mapGovernanceObjects);
        // //READWRITE(mapFinalizedBudgets);
    }

};

//
// Finalized Budget : Contains the suggested proposals to pay on a given block
//

class CFinalizedBudget : public CGovernanceObject
{   // **** Objects and memory ****

private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    bool fAutoChecked; //If it matches what we see, we'll auto vote for it (masternode only)

public:
    bool fValid;
    std::string strBudgetName;
    int nBlockStart;
    std::vector<CTxBudgetPayment> vecBudgetPayments;
    map<uint256, CGovernanceVote> mapVotes;
    uint256 nFeeTXHash;
    int64_t nTime;

    // **** Initialization ****

    CFinalizedBudget();
    CFinalizedBudget(const CFinalizedBudget& other);

    // **** Update ****

    bool AddOrUpdateVote(CGovernanceVote& vote, std::string& strError);
    void AutoCheckSuperBlockVoting(); //check to see if we should vote on new superblock proposals
    void CleanAndRemove(bool fSignatureCheck);
    void SubmitVote(); //vote on this finalized budget as a masternode

    // **** Statistics / Information ****
    int GetBlockStart() {return nBlockStart;}
    int GetBlockEnd() {return nBlockStart + (int)(vecBudgetPayments.size() - 1);}
    bool GetBudgetPaymentByBlock(int64_t nBlockHeight, CTxBudgetPayment& payment)
    {
        LOCK(cs);

        int i = nBlockHeight - GetBlockStart();
        if(i < 0) return false;
        if(i > (int)vecBudgetPayments.size() - 1) return false;
        payment = vecBudgetPayments[i];
        return true;
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strBudgetName;
        ss << nBlockStart;
        ss << vecBudgetPayments;

        uint256 h1 = ss.GetHash();
        return h1;
    }

    std::string GetName() {return strBudgetName; }
    bool GetPayeeAndAmount(int64_t nBlockHeight, CScript& payee, CAmount& nAmount)
    {
        LOCK(cs);

        int i = nBlockHeight - GetBlockStart();
        if(i < 0) return false;
        if(i > (int)vecBudgetPayments.size() - 1) return false;
        payee = vecBudgetPayments[i].payee;
        nAmount = vecBudgetPayments[i].nAmount;
        return true;
    }
    std::string GetProposals();
    double GetScore();
    string GetStatus();
    CAmount GetTotalPayout(); //total dash paid out by this budget
    int64_t GetValidEndTimestamp() {return 0;}
    int64_t GetValidStartTimestamp() {return 32503680000;}
    int GetVoteCount() {return (int)mapVotes.size();}

    bool HasMinimumRequiredSupport();
    bool IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral=true);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);

    // **** Serializer ****

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        // TODO: Do we need names for these? I don't think so
        READWRITE(LIMITED_STRING(strBudgetName, 20));
        READWRITE(nFeeTXHash);
        READWRITE(nTime);
        READWRITE(nBlockStart);
        READWRITE(vecBudgetPayments);
        READWRITE(fAutoChecked);

        READWRITE(mapVotes);
    }
};


// FinalizedBudget are cast then sent to peers with this object, which leaves the votes out
class CFinalizedBudgetBroadcast : public CFinalizedBudget
{
private:
    std::vector<unsigned char> vchSig;

public:
    CFinalizedBudgetBroadcast();
    CFinalizedBudgetBroadcast(const CFinalizedBudget& other);
    CFinalizedBudgetBroadcast(std::string strBudgetNameIn, int nBlockStartIn, std::vector<CTxBudgetPayment> vecBudgetPaymentsIn, uint256 nFeeTXHashIn);

    void swap(CFinalizedBudgetBroadcast& first, CFinalizedBudgetBroadcast& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.strBudgetName, second.strBudgetName);
        swap(first.nBlockStart, second.nBlockStart);
        first.mapVotes.swap(second.mapVotes);
        first.vecBudgetPayments.swap(second.vecBudgetPayments);
        swap(first.nFeeTXHash, second.nFeeTXHash);
        swap(first.nTime, second.nTime);
    }

    CFinalizedBudgetBroadcast& operator=(CFinalizedBudgetBroadcast from)
    {
        swap(*this, from);
        return *this;
    }

    void Relay();

    ADD_SERIALIZE_METHODS;

    //for propagating messages
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients
        READWRITE(LIMITED_STRING(strBudgetName, 20));
        READWRITE(nBlockStart);
        READWRITE(vecBudgetPayments);
        READWRITE(nFeeTXHash);
    }
};

//
// CFinalizedBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CFinalizedBudgetVote
{
public:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    CTxIn vin;
    uint256 nBudgetHash;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CFinalizedBudgetVote();
    CFinalizedBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid(bool fSignatureCheck);
    void Relay();

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nBudgetHash;
        ss << nTime;
        return ss.GetHash();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nBudgetHash);
        READWRITE(nTime);
        READWRITE(vchSig);
    }

};
