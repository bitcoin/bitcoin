// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
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
#include "init.h"

#include <boost/lexical_cast.hpp>

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

static const CAmount BUDGET_FEE_TX = (25*COIN);
static const int64_t BUDGET_FEE_CONFIRMATIONS = 6;
static const int64_t BUDGET_VOTE_UPDATE_MIN = 60*60;
static const int64_t FINAL_BUDGET_VOTE_UPDATE_MIN = 30*60;

extern std::vector<CBudgetProposalBroadcast> vecImmatureBudgetProposals;
extern std::vector<CFinalizedBudgetBroadcast> vecImmatureFinalizedBudgets;

extern CBudgetManager budget;

// Define amount of blocks in budget payment cycle
int GetBudgetPaymentCycleBlocks();

//Check the collateral transaction for the budget proposal/finalized budget
bool IsBudgetCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf);

//
// Budget Manager : Contains all proposals for the budget
//
class CBudgetManager
{
private:

    //hold txes until they mature enough to use
    map<uint256, uint256> mapCollateralTxids;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // keep track of the scanning errors I've seen
    map<uint256, CBudgetProposal> mapProposals;
    map<uint256, CFinalizedBudget> mapFinalizedBudgets;

    std::map<uint256, CBudgetProposalBroadcast> mapSeenMasternodeBudgetProposals;
    std::map<uint256, CBudgetVote> mapSeenMasternodeBudgetVotes;
    std::map<uint256, CBudgetVote> mapOrphanMasternodeBudgetVotes;
    std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;
    std::map<uint256, CFinalizedBudgetVote> mapSeenFinalizedBudgetVotes;
    std::map<uint256, CFinalizedBudgetVote> mapOrphanFinalizedBudgetVotes;

    CBudgetManager()
    {
        mapProposals.clear();
        mapFinalizedBudgets.clear();
    }

    void ClearSeen()
    {
        mapSeenMasternodeBudgetProposals.clear();
        mapSeenMasternodeBudgetVotes.clear();
        mapSeenFinalizedBudgets.clear();
        mapSeenFinalizedBudgetVotes.clear();
    }

    void ResetSync();
    void MarkSynced();
    void Sync(CNode *node, uint256 nProp, bool fPartial = false) const;

    void ProcessMessage(CNode *pfrom, const std::string &strCommand, CDataStream &vRecv);

    void NewBlock();

    CBudgetProposal *FindProposal(const std::string &strProposalName);
    CBudgetProposal *FindProposal(uint256 nHash);
    CFinalizedBudget *FindFinalizedBudget(uint256 nHash);

    CAmount GetTotalBudget(int nHeight);

    std::vector<CBudgetProposal *> GetBudget();
    std::vector<CBudgetProposal *> GetAllProposals();
    std::vector<CFinalizedBudget *> GetFinalizedBudgets();

    bool AddFinalizedBudget(const CFinalizedBudget &finalizedBudget, bool checkCollateral = true);
    bool UpdateFinalizedBudget(const CFinalizedBudgetVote &vote, CNode *pfrom, std::string &strError);
    void SubmitFinalBudget();

    bool AddProposal(const CBudgetProposal &budgetProposal, bool checkCollateral = true);

    // SubmitProposalVote is used when current node submits a vote. ReceiveProposalVote is used when
    // a vote is received from a peer
    bool CanSubmitVotes(int blockStart, int blockEnd) const;
    bool SubmitProposalVote(const CBudgetVote& vote, std::string& strError);
    bool ReceiveProposalVote(const CBudgetVote &vote, CNode *pfrom, std::string &strError);

    bool IsBudgetPaymentBlock(int nBlockHeight) const;
    bool IsTransactionValid(const CTransaction &txNew, int nBlockHeight) const;
    void FillBlockPayee(CMutableTransaction &txNew, CAmount nFees) const;

    std::string GetRequiredPaymentsString(int nBlockHeight) const;
    std::string ToString() const;

    void CheckOrphanVotes();
    void CheckAndRemove();

    void Clear()
    {
        LOCK(cs);

        LogPrintf("Budget object cleared\n");
        mapProposals.clear();
        mapFinalizedBudgets.clear();
        mapSeenMasternodeBudgetProposals.clear();
        mapSeenMasternodeBudgetVotes.clear();
        mapSeenFinalizedBudgets.clear();
        mapSeenFinalizedBudgetVotes.clear();
        mapOrphanMasternodeBudgetVotes.clear();
        mapOrphanFinalizedBudgetVotes.clear();
    }

    ADD_SERIALIZE_METHODS;

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(mapSeenMasternodeBudgetProposals);
        READWRITE(mapSeenMasternodeBudgetVotes);
        READWRITE(mapSeenFinalizedBudgets);
        READWRITE(mapSeenFinalizedBudgetVotes);
        READWRITE(mapOrphanMasternodeBudgetVotes);
        READWRITE(mapOrphanFinalizedBudgetVotes);

        READWRITE(mapProposals);
        READWRITE(mapFinalizedBudgets);
    }

private:
    const CFinalizedBudget *GetMostVotedBudget(int height) const;
};

class CTxBudgetPayment
{
public:
    uint256 nProposalHash;
    CScript payee;
    CAmount nAmount;

    CTxBudgetPayment()
        : nAmount(0)
    {
    }

    CTxBudgetPayment(uint256 nProposalHash, CScript payee, CAmount nAmount)
        : nProposalHash(nProposalHash)
        , payee(payee)
        , nAmount(nAmount)
    {
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

//
// Finalized Budget : Contains the suggested proposals to pay on a given block
//

class CFinalizedBudget
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    bool fAutoChecked; //If it matches what we see, we'll auto vote for it (masternode only)
    boost::optional<int> voteSubmittedTime;

protected:
    std::vector<CTxBudgetPayment> vecBudgetPayments;
    std::string strBudgetName;
    int nBlockStart;
    map<uint256, CFinalizedBudgetVote> mapVotes;
    uint256 nFeeTXHash;
    std::vector<unsigned char> signature;
    CTxIn masternodeSubmittedId;

    static bool ComparePayments(const CTxBudgetPayment& a, const CTxBudgetPayment& b)
    {
        if (a.nAmount != b.nAmount)
            return a.nAmount > b.nAmount;
        else if (a.nProposalHash != b.nProposalHash)
            return a.nProposalHash < b.nProposalHash;
        else if (a.payee != b.payee)
            return a.payee < b.payee;
        else
            return false;
    }

public:
    bool fValid;
    int64_t nTime;

    CFinalizedBudget();
    CFinalizedBudget(const CFinalizedBudget& other);
    CFinalizedBudget(std::string strBudgetName, int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, uint256 nFeeTXHash);
    CFinalizedBudget(std::string strBudgetName, int nBlockStart, const std::vector<CTxBudgetPayment>& vecBudgetPayments, const CTxIn& masternodeId, const CKey& keyMasternode);

    void CleanAndRemove(bool fSignatureCheck);
    bool AddOrUpdateVote(const CFinalizedBudgetVote& vote, std::string& strError);

    bool IsValid(std::string& strError, bool fCheckCollateral=true) const;
    bool IsValid(bool fCheckCollateral=true) const;
    bool VerifySignature(const CPubKey& pubKey) const;

    bool IsVoteSubmitted() const { return voteSubmittedTime.is_initialized(); }
    void ResetAutoChecked();

    std::string GetName() const { return strBudgetName; }
    uint256 GetFeeTxHash() const { return nFeeTXHash; }
    const std::map<uint256, CFinalizedBudgetVote>& GetVotes() const { return mapVotes; }
    std::string GetProposals() const;
    int GetBlockStart() const {return nBlockStart;}
    int GetBlockEnd() const {return nBlockStart;} // Paid in single block
    int GetVoteCount() const {return (int)mapVotes.size();}
    const std::vector<CTxBudgetPayment>& GetBudgetPayments() const;
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const;

    const CTxIn& MasternodeSubmittedId() const { return masternodeSubmittedId; }

    //check to see if we should vote on this
    bool AutoCheck();
    bool IsAutoChecked() const { return fAutoChecked; }
    //total crown paid out by this budget
    CAmount GetTotalPayout() const;
    //vote on this finalized budget as a masternode
    void SubmitVote();

    void MarkSynced();
    int Sync(CNode* pfrom, bool fPartial) const;
    void ResetSync();

    //checks the hashes to make sure we know about them
    std::string GetStatus() const;

    uint256 GetHash() const{
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << nBlockStart;
        ss << vecBudgetPayments;

        uint256 h1 = ss.GetHash();
        return h1;
    }

    ADD_SERIALIZE_METHODS;

    //for saving to the serialized db
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(LIMITED_STRING(strBudgetName, 20));
        READWRITE(nFeeTXHash);
        READWRITE(nTime);
        READWRITE(nBlockStart);
        READWRITE(vecBudgetPayments);
        READWRITE(fAutoChecked);
        READWRITE(signature);
        READWRITE(masternodeSubmittedId);

        READWRITE(mapVotes);
    }
};

// FinalizedBudget are cast then sent to peers with this object, which leaves the votes out
class CFinalizedBudgetBroadcast : private CFinalizedBudget
{
public:
    CFinalizedBudgetBroadcast();
    CFinalizedBudgetBroadcast(std::string strBudgetNameIn, int nBlockStartIn, const std::vector<CTxBudgetPayment>& vecBudgetPaymentsIn, uint256 nFeeTXHashIn);
    CFinalizedBudgetBroadcast(std::string strBudgetNameIn, int nBlockStartIn, const std::vector<CTxBudgetPayment>& vecBudgetPaymentsIn, const CTxIn& masternodeId, const CKey& keyMasternode);

    CFinalizedBudget Budget() const;

    void swap(CFinalizedBudgetBroadcast& first, CFinalizedBudgetBroadcast& second); // nothrow

    CFinalizedBudgetBroadcast& operator=(CFinalizedBudgetBroadcast from);

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
        if (nFeeTXHash == uint256())
        {
            READWRITE(signature);
            READWRITE(masternodeSubmittedId);
        }
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

    uint256 GetHash() const{
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

//
// Budget Proposal : Contains the masternode votes for each budget
//

class CBudgetProposal
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    CAmount nAlloted;

public:
    bool fValid;
    std::string strProposalName;

    /*
        json object with name, short-description, long-description, pdf-url and any other info
        This allows the proposal website to stay 100% decentralized
    */
    std::string strURL;
    int nBlockStart;
    int nBlockEnd;
    CAmount nAmount;
    CScript address;
    mutable int64_t nTime;
    uint256 nFeeTXHash;

    map<uint256, CBudgetVote> mapVotes;
    //cache object

    CBudgetProposal();
    CBudgetProposal(const CBudgetProposal& other);
    CBudgetProposal(std::string strProposalNameIn, std::string strURLIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn, uint256 nFeeTXHashIn);

    void Calculate();
    bool AddOrUpdateVote(const CBudgetVote& vote, std::string& strError);
    bool HasMinimumRequiredSupport();
    std::pair<std::string, std::string> GetVotes();

    bool IsValid(std::string& strError, bool fCheckCollateral=true) const;

    int IsEstablished() const
    {
        //Proposals must be at least a day old to make it into a budget
        if(Params().NetworkID() == CBaseChainParams::MAIN)
            return nTime < GetTime() - 24 * 60 * 60;
        else
            return nTime < GetTime() - 15 * 60; // 15 minutes for testing purposes
    }

    std::string GetName() const {return strProposalName; }
    std::string GetURL() const {return strURL; }
    int GetBlockStart() const {return nBlockStart;}
    int GetBlockEnd() const {return nBlockEnd;}
    CScript GetPayee() const {return address;}
    int GetTotalPaymentCount() const;
    int GetRemainingPaymentCount() const;
    int GetBlockStartCycle() const;
    int GetBlockCurrentCycle() const;
    int GetBlockEndCycle() const;
    double GetRatio() const;
    int GetYeas() const;
    int GetNays() const;
    int GetAbstains() const;
    CAmount GetAmount() const {return nAmount;}
    void SetAllotted(CAmount nAllotedIn) {nAlloted = nAllotedIn;}
    CAmount GetAllotted() const {return nAlloted;}

    void CleanAndRemove(bool fSignatureCheck);

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strProposalName;
        ss << strURL;
        ss << nBlockStart;
        ss << nBlockEnd;
        ss << nAmount;
        ss << *(CScriptBase*)(&address);
        uint256 h1 = ss.GetHash();

        return h1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients
        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strURL, 64));
        READWRITE(nTime);
        READWRITE(nBlockStart);
        READWRITE(nBlockEnd);
        READWRITE(nAmount);
        READWRITE(*(CScriptBase*)(&address));
        READWRITE(nTime);
        READWRITE(nFeeTXHash);

        //for saving to the serialized db
        READWRITE(mapVotes);
    }
};

// Proposals are cast then sent to peers with this object, which leaves the votes out
class CBudgetProposalBroadcast : public CBudgetProposal
{
public:
    CBudgetProposalBroadcast() : CBudgetProposal(){}
    CBudgetProposalBroadcast(const CBudgetProposal& other) : CBudgetProposal(other){}
    CBudgetProposalBroadcast(const CBudgetProposalBroadcast& other) : CBudgetProposal(other){}
    CBudgetProposalBroadcast(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn);

    void swap(CBudgetProposalBroadcast& first, CBudgetProposalBroadcast& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.strProposalName, second.strProposalName);
        swap(first.nBlockStart, second.nBlockStart);
        swap(first.strURL, second.strURL);
        swap(first.nBlockEnd, second.nBlockEnd);
        swap(first.nAmount, second.nAmount);
        swap(first.address, second.address);
        swap(first.nTime, second.nTime);
        swap(first.nFeeTXHash, second.nFeeTXHash);
        first.mapVotes.swap(second.mapVotes);
    }

    CBudgetProposalBroadcast& operator=(CBudgetProposalBroadcast from)
    {
        swap(*this, from);
        return *this;
    }

    void Relay();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients

        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strURL, 64));
        READWRITE(nTime);
        READWRITE(nBlockStart);
        READWRITE(nBlockEnd);
        READWRITE(nAmount);
        READWRITE(*(CScriptBase*)(&address));
        READWRITE(nFeeTXHash);
    }
};

//
// CBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CBudgetVote
{
public:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    CTxIn vin;
    uint256 nProposalHash;
    int nVote;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CBudgetVote();
    CBudgetVote(CTxIn vin, uint256 nProposalHash, int nVoteIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid(bool fSignatureCheck) const;
    void Relay();

    std::string GetVoteString() const {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nProposalHash;
        ss << nVote;
        ss << nTime;
        return ss.GetHash();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nProposalHash);
        READWRITE(nVote);
        READWRITE(nTime);
        READWRITE(vchSig);
    }



};

#endif
