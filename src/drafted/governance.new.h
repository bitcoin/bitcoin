// // Copyright (c) 2014-2016 The Dash Core developers

// // Distributed under the MIT/X11 software license, see the accompanying
// // file COPYING or http://www.opensource.org/licenses/mit-license.php.
// #ifndef GOVERNANCE_H
// #define GOVERNANCE_H

// //todo: which of these do we need?
// #include "main.h"
// #include "sync.h"
// #include "net.h"
// #include "key.h"
// #include "util.h"
// #include "base58.h"
// #include "masternode.h"
// #include "governance-types.h"
// #include "governance-vote.h"
// #include <boost/lexical_cast.hpp>
// #include "init.h"
// #include <univalue.h>


// using namespace std;

// extern CCriticalSection cs_budget;

// class CGovernanceManager;
// class CGovernanceNode;
// class CTxBudgetPayment;
// class CFinalizedBudgetBroadcast;
// class CFinalizedBudget;

// extern CGovernanceManager govman;
// extern std::vector<CGovernanceNode> vecImmatureGovernanceNodes;


// void DumpBudgets();

// //Check the collateral transaction for the budget proposal/finalized budget
// bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf);
// std::string PrimaryTypeToString(GovernanceObjectType type);


// /*
//     GOVERNANCE CLASSES
// */


// /** Save Budget Manager (budget.dat)
//  */
// class CBudgetDB
// {
// private:
//     boost::filesystem::path pathDB;
//     std::string strMagicMessage;
// public:
//     enum ReadResult {
//         Ok,
//         FileError,
//         HashReadError,
//         IncorrectHash,
//         IncorrectMagicMessage,
//         IncorrectMagicNumber,
//         IncorrectFormat
//     };

//     CBudgetDB();
//     bool Write(const CGovernanceManager &objToSave);
//     ReadResult Read(CGovernanceManager& objToLoad, bool fDryRun = false);
// };


// //
// // Governance Manager : Contains all proposals for the budget
// //
// class CGovernanceManager
// {   // **** Objects and memory ****

// private:

//     //hold txes until they mature enough to use
//     map<uint256, CTransaction> mapCollateral;
//     // Keep track of current block index
//     const CBlockIndex *pCurrentBlockIndex;

// public:
//     // critical section to protect the inner data structures
//     mutable CCriticalSection cs;
    
//     // hold governance objects (proposals, contracts, settings and switches)
//     std::map<uint256, CGovernanceNode> mapGovernanceObjects;
//     // finalized budgets are kept in their own object
//     // std::map<uint256, CFinalizedBudget> mapFinalizedBudgets;

//     std::map<uint256, CGovernanceNode> mapSeenGovernanceObjects;
//     std::map<uint256, CGovernanceVote> mapSeenGovernanceVotes;
//     std::map<uint256, CGovernanceVote> mapOrphanGovernanceVotes;
//     //std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;

//     // VOTES <obj hash < vote hash <  Vote > >
//     std::map<uint256, std::map<uint256, CGovernanceVote> > mapVotes;

//     // **** Initialization ****

//     CGovernanceManager() {
//         mapGovernanceObjects.clear();
//         //mapFinalizedBudgets.clear();
//     }

//     void Clear(){
//         LOCK(cs);

//         LogPrintf("Governance object cleared\n");
//         mapGovernanceObjects.clear();
//         //mapFinalizedBudgets.clear();
//         mapSeenGovernanceObjects.clear();
//         mapSeenGovernanceVotes.clear();
//         //mapSeenFinalizedBudgets.clear();
//         mapOrphanGovernanceVotes.clear();
//     }

//     void ClearSeen() {
//         mapSeenGovernanceObjects.clear();
//         mapSeenGovernanceVotes.clear();
//         //mapSeenFinalizedBudgets.clear();
//     }

//     void Sync(CNode* node, uint256 nProp, bool fPartial=false);
//     void ResetSync();
//     void MarkSynced();

//     // **** Search / Statistics / Information ****

//     int CountProposalInventoryItems() { return mapSeenGovernanceObjects.size() + mapSeenGovernanceVotes.size(); }
//     //int CountFinalizedInventoryItems() { return mapSeenFinalizedBudgets.size(); }

//     CGovernanceNode *FindGovernanceObject(const std::string &strName);
//     CGovernanceNode *FindGovernanceObject(uint256 nHash);
//     //CFinalizedBudget *FindFinalizedBudget(uint256 nHash);
//     GovernanceObjectType GetGovernanceTypeByHash(uint256 nHash);
//     std::vector<CGovernanceNode*> GetBudget();
//     CAmount GetTotalBudget(int nHeight);
//     bool HasNextFinalizedBudget(); // Do we have the next finalized budget?
//     bool IsBudgetPaymentBlock(int nBlockHeight);
//     bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    
//     std::vector<CGovernanceNode*> FindMatchingGovernanceObjects(GovernanceObjectType type);
//     //std::vector<CFinalizedBudget*> GetFinalizedBudgets();
//     std::string GetRequiredPaymentsString(int nBlockHeight);
//     std::string ToString() const;

//     // **** Update ****

//     //bool AddFinalizedBudget(CFinalizedBudget& finalizedBudget);
//     bool AddGovernanceObject(CGovernanceNode& budgetProposal);
//     bool AddOrphanGovernanceVote(CGovernanceVote& vote, CNode* pfrom);
//     void CheckAndRemove();
//     void CheckOrphanVotes();
//     void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees);
//     void NewBlock();
//     void SubmitFinalBudget();
//     void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
//     void UpdatedBlockTip(const CBlockIndex *pindex);
//     bool UpdateGovernanceObjectVotes(CGovernanceVote& vote, CNode* pfrom, std::string& strError);

//     // **** Serializer ****

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // READWRITE(mapSeenGovernanceObjects);
//         // READWRITE(mapSeenGovernanceVotes);
//         // //READWRITE(mapSeenFinalizedBudgets);
//         // READWRITE(mapOrphanGovernanceVotes);

//         // READWRITE(mapGovernanceObjects);
//         // //READWRITE(mapFinalizedBudgets);
//     }

// };

// // used purely for readablility
// typedef std::vector<unsigned char> signature;

// /*
//  * CGovernanceNode
//  * --------------------
//  *
//  * This is the base class of the goverance system.
//  *
// */


// class CGovernanceNode
// {   // **** Objects and memory ****

// private:
//     // critical section to protect the inner data structures
//     mutable CCriticalSection cs;
//     CAmount nAlloted;

// public:
//     bool fValid;
//     std::string strName;

//     /*
//         BASE DATA that all governance objects have in common
//     */
    
//     int nGovernanceType;
//     int64_t nTime;
//     uint256 nFeeTXHash;

//     // CATEGORY is mapped to the hash of a category node, see governance-categories.h 
//     uint256 category;

//     // SIGNATURES
//     std::vector<signature> vecSig; // USER SIGNATURE

//     // CHILDREN NODES -- automatically sorted by votes desc
//     vector<CGovernanceNode*> vecChildren;

//     // **** v12.1 additions ***

//     // //do we have sign off from the important nodes for this action
//     // bool IsChildValid(CGovernanceNode* child)
//     // {
//     //     if(child->GetSignaturePubkey() == GetPubkey()) return true;
//     // }

//     GovernanceObjectType GetType()
//     {
//         return (GovernanceObjectType)nGovernanceType;
//     }

//     // **** Initialization ****

//     CGovernanceNode();
//     CGovernanceNode(const CGovernanceNode& other);
//     CGovernanceNode(UniValue obj);

//     // **** Update ****
    
//     //bool AddOrUpdateVote(CGovernanceVote& vote, std::string& strError);
//     void CleanAndRemove(bool fSignatureCheck);
//     bool HasMinimumRequiredSupport();

//     void SetAllotted(CAmount nAllotedIn) {nAlloted = nAllotedIn;}
//     void SetNull();
//     bool SignKeySpace(int nKeySpace, CBitcoinSecret key);

//     // **** Statistics / Information ****
    
//     int GetAbsoluteYesCount();
//     int GetAbstainCount();
//     CAmount GetAllotted() {return nAlloted;}
    
//     int GetBlockCurrentCycle(const CBlockIndex* pindex);
    
//     GovernanceObjectType GetGovernanceType();
//     std::string GetGovernanceTypeAsString();

//     int GetNoCount();
//     int GetRemainingPaymentCount(int nBlockHeight);
//     double GetRatio();
//     int GetTotalPaymentCount();
//     int64_t GetValidEndTimestamp();
//     int64_t GetValidStartTimestamp();
//     int GetYesCount();

//     bool IsCategoryValid();
//     bool IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral=true);
//     bool IsEstablished();

//     virtual uint256 GetHash()
//     {
//         return uint256();
//     }
// };

// /*
//  * Class : CGovernanceNodeBroadcast
//  * --------------------
//  *  Broadcasts have moved to governance-classes.h
//  *
// */

// // Proposals are cast then sent to peers with this object, which leaves the votes out
// // class CGovernanceNodeBroadcast : public CGovernanceNode
// // {
// // public:
// //     CGovernanceNodeBroadcast() : CGovernanceNode(){}
// //     CGovernanceNodeBroadcast(const CGovernanceNode& other) : CGovernanceNode(other){}
// //     CGovernanceNodeBroadcast(const CGovernanceNodeBroadcast& other) : CGovernanceNode(other){}

// //     void CreateProposalOrContract(GovernanceObjectType nTypeIn, std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn);
// //     void CreateProposal(std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn);
// //     void CreateContract(std::string strNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn);
// //     void CreateSetting(std::string strNameIn, std::string strURLIn, std::string strSuggestedValueIn, uint256 nFeeTXHashIn);

// //     void swap(CGovernanceNodeBroadcast& first, CGovernanceNodeBroadcast& second) // nothrow
// //     {
// //         // enable ADL (not necessary in our case, but good practice)
// //         using std::swap;

// //         // by swapping the members of two classes,
// //         // the two classes are effectively swapped
// //         swap(first.nGovernanceType, second.nGovernanceType);
// //         swap(first.strName, second.strName);
// //         swap(first.nBlockStart, second.nBlockStart);
// //         swap(first.strURL, second.strURL);
// //         swap(first.nBlockEnd, second.nBlockEnd);
// //         swap(first.nAmount, second.nAmount);
// //         swap(first.address, second.address);
// //         swap(first.nTime, second.nTime);
// //         swap(first.nFeeTXHash, second.nFeeTXHash);
// //         swap(first.nGovernanceType, second.nGovernanceType);
// //         first.mapVotes.swap(second.mapVotes);
// //     }

// //     CGovernanceNodeBroadcast& operator=(CGovernanceNodeBroadcast from)
// //     {
// //         swap(*this, from);
// //         return *this;
// //     }

// //     void Relay();
// // };

// #endif
