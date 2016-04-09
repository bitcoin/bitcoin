// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_BUDGET_H
#define MASTERNODE_BUDGET_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include <boost/lexical_cast.hpp>
#include "init.h"

using namespace std;

extern CCriticalSection cs_budget;

class CBudgetManager;
class CFinalizedBudgetBroadcast;
class CFinalizedBudget;
class CFinalizedBudgetVote;
class CBudgetProposal;
class CBudgetProposalBroadcast;
class CBudgetVote;
class CTxBudgetPayment;

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

static const CAmount BUDGET_FEE_TX = (5*COIN);
static const int64_t BUDGET_FEE_CONFIRMATIONS = 6;
static const int64_t BUDGET_VOTE_UPDATE_MIN = 60*60;

extern std::vector<CBudgetProposalBroadcast> vecImmatureBudgetProposals;
extern std::vector<CFinalizedBudgetBroadcast> vecImmatureFinalizedBudgets;

extern CBudgetManager budget;
void DumpBudgets();

//Check the collateral transaction for the budget proposal/finalized budget
bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf);

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
    bool Write(const CBudgetManager &objToSave);
    ReadResult Read(CBudgetManager& objToLoad, bool fDryRun = false);
};


//
// Budget Manager : Contains all proposals for the budget
//
class CBudgetManager
{
private:

    //hold txes until they mature enough to use
    map<uint256, CTransaction> mapCollateral;
    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    
    map<uint256, CFinalizedBudget> mapFinalizedBudgets;

    std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;
    std::map<uint256, CFinalizedBudgetVote> mapSeenFinalizedBudgetVotes;
    std::map<uint256, CFinalizedBudgetVote> mapOrphanFinalizedBudgetVotes;

    CBudgetManager() {
        mapFinalizedBudgets.clear();
    }

    void ClearSeen() {
        mapSeenFinalizedBudgets.clear();
        mapSeenFinalizedBudgetVotes.clear();
    }

    int CountFinalizedInventoryItems()
    {
        return mapSeenFinalizedBudgets.size() + mapSeenFinalizedBudgetVotes.size();
    }

    int sizeFinalized() {return (int)mapFinalizedBudgets.size();}

    void ResetSync(); 
    void MarkSynced();
    void Sync(CNode* node, uint256 nProp, bool fPartial=false);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();

    CFinalizedBudget *FindFinalizedBudget(uint256 nHash);

    CAmount GetTotalBudget(int nHeight);
    
    std::vector<CFinalizedBudget*> GetFinalizedBudgets();

    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool AddFinalizedBudget(CFinalizedBudget& finalizedBudget);
    void SubmitFinalBudget();
    bool HasNextFinalizedBudget();

    bool UpdateFinalizedBudget(CFinalizedBudgetVote& vote, CNode* pfrom, std::string& strError);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees);

    void CheckOrphanVotes();
    void Clear(){
        LOCK(cs);

        LogPrintf("Budget object cleared\n");
        mapFinalizedBudgets.clear();
        mapSeenFinalizedBudgets.clear();
        mapSeenFinalizedBudgetVotes.clear();
        mapOrphanFinalizedBudgetVotes.clear();
    }
    void CheckAndRemove();
    std::string ToString() const;
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapSeenFinalizedBudgets);
        READWRITE(mapSeenFinalizedBudgetVotes);
        READWRITE(mapOrphanFinalizedBudgetVotes);
        READWRITE(mapFinalizedBudgets);
    }

    void UpdatedBlockTip(const CBlockIndex *pindex);
    //# ----
};


class CTxBudgetPayment
{
    //# ----
public:
    uint256 nProposalHash;
    CScript payee;
    CAmount nAmount;

    CTxBudgetPayment() {
        payee = CScript();
        nAmount = 0;
        nProposalHash = uint256();
    }

    ADD_SERIALIZE_METHODS;

    //for saving to the serialized db
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CScriptBase*)(&payee));
        READWRITE(nAmount);
        READWRITE(nProposalHash);
    }
};