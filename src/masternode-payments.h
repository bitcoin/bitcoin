

// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_PAYMENTS_H
#define MASTERNODE_PAYMENTS_H

#include "key.h"
#include "main.h"
#include "masternode.h"
#include <boost/lexical_cast.hpp>

using namespace std;

extern CCriticalSection cs_vecPayments;
extern CCriticalSection cs_mapMasternodeBlocks;
extern CCriticalSection cs_mapMasternodePayeeVotes;

class CMasternodePayments;
class CMasternodePaymentWinner;
class CMasternodeBlockPayees;

extern CMasternodePayments masternodePayments;

#define MNPAYMENTS_SIGNATURES_REQUIRED           6
#define MNPAYMENTS_SIGNATURES_TOTAL              10

void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
bool IsReferenceNode(CTxIn& vin);
bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight);
std::string GetRequiredPaymentsString(int nBlockHeight);
bool IsBlockValueValid(const CBlock& block, int64_t nExpectedValue);
void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees);

void DumpMasternodePayments();

/** Save Masternode Payment Data (mnpayments.dat)
 */
class CMasternodePaymentDB
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

    CMasternodePaymentDB();
    bool Write(const CMasternodePayments &objToSave);
    ReadResult Read(CMasternodePayments& objToLoad, bool fDryRun = false);
};

class CMasternodePayee
{
public:
    CScript scriptPubKey;
    int nVotes;
    uint256 nInvHash;
    int nMasternodeRank;

    CMasternodePayee() {
        scriptPubKey = CScript();
        nVotes = 0;
        nInvHash = 0;
        nMasternodeRank = 99999;
    }

    CMasternodePayee(CScript payee, int nVotesIn, uint256 nInvHashIn, int nMasternodeRankIn) {
        scriptPubKey = payee;
        nVotes = nVotesIn;
        nInvHash = nInvHashIn;
        nMasternodeRank = nMasternodeRankIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(scriptPubKey);
        READWRITE(nVotes);
        READWRITE(nInvHash);
        READWRITE(nMasternodeRank);
     }
};

// Keep track of votes for payees from masternodes
class CMasternodeBlockPayees
{
public:
    int nBlockHeight;
    std::vector<CMasternodePayee> vecPayments;

    CMasternodeBlockPayees(){
        nBlockHeight = 0;
        vecPayments.clear();
    }
    CMasternodeBlockPayees(int nBlockHeightIn) {
        nBlockHeight = nBlockHeightIn;
        vecPayments.clear();
    }

    void AddPayee(CScript payeeIn, int nIncrement, uint256 nInvHashIn, int nMasternodeRankIn){
        LOCK(cs_vecPayments);

        BOOST_FOREACH(CMasternodePayee& payee, vecPayments){
            if(payee.scriptPubKey == payeeIn) {
                payee.nVotes += nIncrement;
                //keep track of the "best" (highest rank) masternode's inventory item so we can send that one
                // when syncing, we will consume much less bandwidth because of this
                if(payee.nMasternodeRank > nMasternodeRankIn){
                    payee.nMasternodeRank = nMasternodeRankIn;
                    payee.nInvHash = nInvHashIn;
                }
                return;
            }
        }

        CMasternodePayee c(payeeIn, nIncrement, nInvHashIn, nMasternodeRankIn);
        vecPayments.push_back(c);
    }

    bool GetPayee(CScript& payee)
    {
        LOCK(cs_vecPayments);

        int nVotes = -1;
        BOOST_FOREACH(CMasternodePayee& p, vecPayments){
            if(p.nVotes > nVotes){
                payee = p.scriptPubKey;
                nVotes = p.nVotes;
            }
        }

        return (nVotes > -1);
    }

    int CountVotes()
    {
        int nVotes = 0;
        BOOST_FOREACH(CMasternodePayee& p, vecPayments)
            nVotes += p.nVotes;

        return nVotes;
    }

    bool GetPayeeInventoryItems(vector<uint256>& vecHash)
    {
        LOCK(cs_vecPayments);

        BOOST_FOREACH(CMasternodePayee& p, vecPayments){
            //if we're syncing another client from a partial list, send everything.
            // otherwise, we'll require 2 votes per item (those are the ones that count on the winners list)
            if(p.nVotes >= 2 || CountVotes() <= 5){
                vecHash.push_back(p.nInvHash);
            }
        }

        return vecHash.size() >= 1;
    }

    bool HasPayeeWithVotes(CScript payee, int nVotesReq)
    {
        LOCK(cs_vecPayments);

        BOOST_FOREACH(CMasternodePayee& p, vecPayments){
            if(p.nVotes >= nVotesReq && p.scriptPubKey == payee) return true;
        }

        return false;
    }

    bool IsTransactionValid(const CTransaction& txNew);
    std::string GetRequiredPaymentsString();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nBlockHeight);
        READWRITE(vecPayments);
     }
};

// for storing the winning payments
class CMasternodePaymentWinner
{
public:
    CTxIn vinMasternode;

    int nBlockHeight;
    CScript payee;
    std::vector<unsigned char> vchSig;

    CMasternodePaymentWinner() {
        nBlockHeight = 0;
        vinMasternode = CTxIn();
        payee = CScript();
    }

    CMasternodePaymentWinner(CTxIn vinIn) {
        nBlockHeight = 0;
        vinMasternode = vinIn;
        payee = CScript();
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << payee;
        ss << nBlockHeight;
        ss << vinMasternode.prevout;

        return ss.GetHash();
    }

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid(CNode* pnode, std::string& strError, int& nRank);
    bool SignatureValid();
    void Relay();

    void AddPayee(CScript payeeIn){
        payee = payeeIn;
    }


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
        READWRITE(nBlockHeight);
        READWRITE(payee);
        READWRITE(vchSig);
    }

    std::string ToString()
    {
        std::string ret = "";
        ret += vinMasternode.ToString();
        ret += ", " + boost::lexical_cast<std::string>(nBlockHeight);
        ret += ", " + payee.ToString();
        ret += ", " + boost::lexical_cast<std::string>((int)vchSig.size());
        return ret;
    }
};

//
// Masternode Payments Class
// Keeps track of who should get paid for which blocks
//

class CMasternodePayments
{
private:
    int nSyncedFromPeer;
    int nLastBlockHeight;

public:
    std::map<uint256, CMasternodePaymentWinner> mapMasternodePayeeVotes;
    std::map<int, CMasternodeBlockPayees> mapMasternodeBlocks;
    std::map<uint256, int> mapMasternodesLastVote; //prevout.hash + prevout.n, nBlockHeight

    CMasternodePayments() {
        nSyncedFromPeer = 0;
        nLastBlockHeight = 0;
    }

    void Clear() {
        LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePayeeVotes);
        mapMasternodeBlocks.clear();
        mapMasternodePayeeVotes.clear();
    }

    bool AddWinningMasternode(CMasternodePaymentWinner& winner, int nRank);
    bool ProcessBlock(int nBlockHeight);

    void Sync(CNode* node, int nCountNeeded);
    void CleanPaymentList();
    int LastPayment(CMasternode& mn);

    bool GetBlockPayee(int nBlockHeight, CScript& payee);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    bool IsScheduled(CMasternode& mn, int nNotBlockHeight);

    bool CanVote(COutPoint outMasternode, int nBlockHeight) {
        LOCK(cs_mapMasternodePayeeVotes);

        if(mapMasternodesLastVote.count(outMasternode.hash + outMasternode.n)) {
            if(mapMasternodesLastVote[outMasternode.hash + outMasternode.n] == nBlockHeight) {
                return false;
            }
        }

        //record this masternode voted
        mapMasternodesLastVote[outMasternode.hash + outMasternode.n] = nBlockHeight;
        return true;
    }

    int GetMinMasternodePaymentsProto();
    void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees);
    std::string ToString() const;
    int GetOldestBlock();
    int GetNewestBlock();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapMasternodePayeeVotes);
        READWRITE(mapMasternodeBlocks);
    }
};



#endif
