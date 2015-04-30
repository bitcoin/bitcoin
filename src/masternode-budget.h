
// Copyright (c) 2014-2015 The Dash developers
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
#include "masternode-pos.h"
//#include "timedata.h"

using namespace std;

class CBudgetManager;
class CBudgetProposal;
class CBudgetVote;

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

extern CBudgetManager budget;

void DumpBudgets();
uint256 GetBudgetProposalHash(std::string strProposalName);

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
    bool Write(const CBudgetManager &budgetToSave);
    ReadResult Read(CBudgetManager& budgetToLoad);
};


//
// Budget Manager : Contains all proposals for the budget
//
class CBudgetManager
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:
    // keep track of the scanning errors I've seen
    map<uint256, CBudgetProposal> mapProposals;
    
    CBudgetManager() {
        mapProposals.clear();
    }

    void Sync(CNode* node);

    void Calculate();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    CBudgetProposal *Find(const std::string &strProposalName);
    inline void NewBlock();
    std::pair<std::string, std::string> GetVotes(std::string strProposalName);
    
    void CleanUp();

    int64_t GetTotalBudget();
    std::vector<CBudgetProposal*> GetBudget();
    bool IsBudgetPaymentBlock(int nHeight);
    void AddOrUpdateProposal(CBudgetVote& vote);

    void Clear(){
        printf("Not implemented\n");
    }
    void CheckAndRemove(){
        printf("Not implemented\n");
        //replace cleanup
    }
    std::string ToString() {return "not implemented";}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapProposals);
    }
};


//
// Budget Proposal : Contains the masternode votes for each budget
//

class CBudgetProposal
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    int64_t nAlloted;

public:
    std::string strProposalName;

    map<uint256, CBudgetVote> mapVotes;
    //cache object

    CBudgetProposal();  
    CBudgetProposal(const CBudgetProposal& other);
    CBudgetProposal(std::string strProposalNameIn);

    void Sync(CNode* node);

    void Calculate();
    void AddOrUpdateVote(CBudgetVote& vote);
    bool HasMinimumRequiredSupport();
    std::pair<std::string, std::string> GetVotes();

    std::string GetName();
    int GetBlockStart();
    int GetBlockEnd();
    CScript GetPayee();
    double GetRatio();
    int GetYeas();
    int GetNays();
    int GetAbstains();
    int64_t GetAmount();

    void SetAllotted(int64_t nAllotedIn) {nAlloted = nAllotedIn;}
    int64_t GetAllotted() {return nAlloted;}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapVotes);
    }
};


//
// CBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CBudgetVote
{
public:
    std::string strProposalName;
    CTxIn vin;
    int nBlockStart;
    int nBlockEnd;
    int64_t nAmount;
    int nVote;
    int64_t nTime;
    CScript address;
    std::vector<unsigned char> vchSig;

    CBudgetVote();
    CBudgetVote(CTxIn vinIn, std::string strProposalNameIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn, int nVoteIn);

    bool Sign(CKey& keyCollateralAddress, CPubKey& pubKeyMasternode);
    bool SignatureValid();
    void Relay();

    std::string GetVoteString() {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash(){
        return Hash(BEGIN(vin), END(vin), BEGIN(nVote), END(nVote), BEGIN(nTime), END(nTime));
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(vin);
        READWRITE(nBlockStart);
        READWRITE(nBlockEnd);
        READWRITE(nAmount);
        READWRITE(nVote);
        READWRITE(nTime);
        READWRITE(address);
        READWRITE(vchSig);
    }

    

};

#endif