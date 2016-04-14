// // Copyright (c) 2014-2016 The Dash Core developers

// // Distributed under the MIT/X11 software license, see the accompanying
// // file COPYING or http://www.opensource.org/licenses/mit-license.php.
// #ifndef GOVERNANCE_FINALIZE_H
// #define GOVERNANCE_FINALIZE_H

// //todo: which of these do we need?
// //#include "main.h"
// //#include "sync.h"
// //#include "net.h"
// //#include "key.h"
// //#include "util.h"
// //#include "base58.h"
// //#include "masternode.h"
// #include "governance.h"
// //#include "governance-types.h"
// //#include <boost/lexical_cast.hpp>
// #include <univalue.h>



// //
// // Finalized Budget : Contains the suggested proposals to pay on a given block
// //

// class CFinalizedBudget : public CGovernanceObject
// {   // **** Objects and memory ****

// private:
//     // critical section to protect the inner data structures
//     mutable CCriticalSection cs;
//     bool fAutoChecked; //If it matches what we see, we'll auto vote for it (masternode only)

// public:
//     bool fValid;
//     std::string strBudgetName;
//     int nBlockStart;
//     std::vector<CTxBudgetPayment> vecBudgetPayments;
//     map<uint256, CGovernanceVote> mapVotes;
//     uint256 nFeeTXHash;
//     int64_t nTime;

//     // **** Initialization ****

//     CFinalizedBudget();
//     CFinalizedBudget(const CFinalizedBudget& other);

//     // **** Update ****

//     bool AddOrUpdateVote(CGovernanceVote& vote, std::string& strError);
//     void AutoCheckSuperBlockVoting(); //check to see if we should vote on new superblock proposals
//     void CleanAndRemove(bool fSignatureCheck);
//     void SubmitVote(); //vote on this finalized budget as a masternode

//     // **** Statistics / Information ****
//     int GetBlockStart() {return nBlockStart;}
//     int GetBlockEnd() {return nBlockStart + (int)(vecBudgetPayments.size() - 1);}
//     bool GetBudgetPaymentByBlock(int64_t nBlockHeight, CTxBudgetPayment& payment)
//     {
//         LOCK(cs);

//         int i = nBlockHeight - GetBlockStart();
//         if(i < 0) return false;
//         if(i > (int)vecBudgetPayments.size() - 1) return false;
//         payment = vecBudgetPayments[i];
//         return true;
//     }

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strBudgetName;
//         ss << nBlockStart;
//         ss << vecBudgetPayments;

//         uint256 h1 = ss.GetHash();
//         return h1;
//     }

//     std::string GetName() {return strBudgetName; }
//     bool GetPayeeAndAmount(int64_t nBlockHeight, CScript& payee, CAmount& nAmount)
//     {
//         LOCK(cs);

//         int i = nBlockHeight - GetBlockStart();
//         if(i < 0) return false;
//         if(i > (int)vecBudgetPayments.size() - 1) return false;
//         payee = vecBudgetPayments[i].payee;
//         nAmount = vecBudgetPayments[i].nAmount;
//         return true;
//     }
//     std::string GetProposals();
//     double GetScore();
//     string GetStatus();
//     CAmount GetTotalPayout(); //total dash paid out by this budget
//     int64_t GetValidEndTimestamp() {return 0;}
//     int64_t GetValidStartTimestamp() {return 32503680000;}
//     int GetVoteCount() {return (int)mapVotes.size();}

//     bool HasMinimumRequiredSupport();
//     bool IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral=true);
//     bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);

//     // **** Serializer ****

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO: Do we need names for these? I don't think so
//         READWRITE(LIMITED_STRING(strBudgetName, 20));
//         READWRITE(nFeeTXHash);
//         READWRITE(nTime);
//         READWRITE(nBlockStart);
//         READWRITE(vecBudgetPayments);
//         READWRITE(fAutoChecked);

//         READWRITE(mapVotes);
//     }
// };
