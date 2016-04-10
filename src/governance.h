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
#include <boost/lexical_cast.hpp>
#include "init.h"

using namespace std;

extern CCriticalSection cs_budget;

class CGovernanceManager;
class CBudgetProposal;
class CBudgetProposalBroadcast;
class CBudgetVote;

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

// todo - 12.1 - change BUDGET_ to GOVERNANCE_ (cherry pick)
static const CAmount GOVERNANCE_FEE_TX = (5*COIN);
static const int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static const int64_t GOVERNANCE_UPDATE_MIN = 60*60;

extern std::vector<CBudgetProposalBroadcast> vecImmatureBudgetProposals;
extern CGovernanceManager governator;

//Check the collateral transaction for the budget proposal/finalized budget
bool IsCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf, CAmount minFee);

/** Save Budget Manager (budget.dat)
 */
// todo - 12.1 - rename to CGovernanceDB (cherry pick)
// todo - 12.1 - move to goverance.dat
class CBudgetDB
{
    //# ----
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

//# ----

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

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    
    // keep track of the scanning errors I've seen
    map<uint256, CBudgetProposal> mapProposals;

    std::map<uint256, CBudgetProposalBroadcast> mapSeenMasternodeBudgetProposals;
    std::map<uint256, CBudgetVote> mapSeenMasternodeBudgetVotes;
    std::map<uint256, CBudgetVote> mapOrphanMasternodeBudgetVotes;

    CGovernanceManager() {
        mapProposals.clear();
    }

    void ClearSeen() {
        mapSeenMasternodeBudgetProposals.clear();
        mapSeenMasternodeBudgetVotes.clear();
    }

    int CountProposalInventoryItems()
    {
        return mapSeenMasternodeBudgetProposals.size() + mapSeenMasternodeBudgetVotes.size();
    }

    int sizeProposals() {return (int)mapProposals.size();}

    void ResetSync();
    void MarkSynced();
    void Sync(CNode* node, uint256 nProp, bool fPartial=false);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();

    CBudgetProposal *FindProposal(const std::string &strProposalName);
    CBudgetProposal *FindProposal(uint256 nHash);
    
    std::vector<CBudgetProposal*> GetBudget();
    std::vector<CBudgetProposal*> GetAllProposals();

    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool AddProposal(CBudgetProposal& budgetProposal);

    bool UpdateProposal(CBudgetVote& vote, CNode* pfrom, std::string& strError);
    bool PropExists(uint256 nHash);
    std::string GetRequiredPaymentsString(int nBlockHeight);

    void CheckOrphanVotes();
    void Clear(){
        LOCK(cs);

        LogPrintf("Budget object cleared\n");
        mapProposals.clear();
        mapSeenMasternodeBudgetProposals.clear();
        mapSeenMasternodeBudgetVotes.clear();
        mapOrphanMasternodeBudgetVotes.clear();
    }
    void CheckAndRemove();
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapSeenMasternodeBudgetProposals);
        READWRITE(mapSeenMasternodeBudgetVotes);
        READWRITE(mapOrphanMasternodeBudgetVotes);
        READWRITE(mapProposals);
    }

    void UpdatedBlockTip(const CBlockIndex *pindex);
    //# ----
};

//
// Budget Proposal : Contains the masternode votes for each budget
//

class CBudgetProposal
{
    //# ----
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
    int64_t nTime;
    uint256 nFeeTXHash;

    map<uint256, CBudgetVote> mapVotes;
    //cache object

    CBudgetProposal();
    CBudgetProposal(const CBudgetProposal& other);
    CBudgetProposal(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn);

    bool AddOrUpdateVote(CBudgetVote& vote, std::string& strError);
    bool HasMinimumRequiredSupport();
    std::pair<std::string, std::string> GetVotes();

    bool IsValid(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral=true);
    bool IsEstablished();

    std::string GetName() {return strProposalName; }
    std::string GetURL() {return strURL; }
    int GetBlockStart() {return nBlockStart;}
    int GetBlockEnd() {return nBlockEnd;}
    CScript GetPayee() {return address;}
    int GetTotalPaymentCount();
    int GetRemainingPaymentCount(int nBlockHeight);
    int GetBlockStartCycle();
    int GetBlockCurrentCycle(const CBlockIndex* pindex);
    int GetBlockEndCycle();
    double GetRatio();
    int GetAbsoluteYesCount();
    int GetYesCount();
    int GetNoCount();
    int GetAbstainCount();
    CAmount GetAmount() {return nAmount;}
    void SetAllotted(CAmount nAllotedIn) {nAlloted = nAllotedIn;}
    CAmount GetAllotted() {return nAlloted;}

    void CleanAndRemove(bool fSignatureCheck);

    uint256 GetHash(){
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
    //# ----
public:
    CBudgetProposalBroadcast() : CBudgetProposal(){}
    CBudgetProposalBroadcast(const CBudgetProposal& other) : CBudgetProposal(other){}
    CBudgetProposalBroadcast(const CBudgetProposalBroadcast& other) : CBudgetProposal(other){}
    CBudgetProposalBroadcast(std::string strProposalNameIn, std::string strURLIn, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn, uint256 nFeeTXHashIn) {}

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
    //# ----
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
    bool IsValid(bool fSignatureCheck);
    void Relay();

    std::string GetVoteString() {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash(){
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
