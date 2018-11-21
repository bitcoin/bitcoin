// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSTEMNODE_PAYMENTS_H
#define SYSTEMNODE_PAYMENTS_H

#include "key.h"
#include "main.h"
#include "systemnode.h"
#include <boost/lexical_cast.hpp>

using namespace std;

extern CCriticalSection cs_vecSNPayments;
extern CCriticalSection cs_mapSystemnodeBlocks;
extern CCriticalSection cs_mapSystemnodePayeeVotes;

class CSystemnodePayments;
class CSystemnodePaymentWinner;
class CSystemnodeBlockPayees;

extern CSystemnodePayments systemnodePayments;

#define SNPAYMENTS_SIGNATURES_REQUIRED           6
#define SNPAYMENTS_SIGNATURES_TOTAL              10
#define SN_PMT_SLOT                              2

void SNFillBlockPayee(CMutableTransaction& txNew, int64_t nFees);
bool SNIsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight);
std::string SNGetRequiredPaymentsString(int nBlockHeight);


class CSystemnodePayee
{
public:
    CScript scriptPubKey;
    int nVotes;

    CSystemnodePayee() {
        scriptPubKey = CScript();
        nVotes = 0;
    }

    CSystemnodePayee(CScript payee, int nVotesIn) {
        scriptPubKey = payee;
        nVotes = nVotesIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CScriptBase*)(&scriptPubKey));
        READWRITE(nVotes);
     }
};

// Keep track of votes for payees from systemnodes
class CSystemnodeBlockPayees
{
public:
    int nBlockHeight;
    std::vector<CSystemnodePayee> vecPayments;

    CSystemnodeBlockPayees(){
        nBlockHeight = 0;
        vecPayments.clear();
    }
    CSystemnodeBlockPayees(int nBlockHeightIn) {
        nBlockHeight = nBlockHeightIn;
        vecPayments.clear();
    }

    void AddPayee(CScript payeeIn, int nIncrement){
        LOCK(cs_vecSNPayments);

        BOOST_FOREACH(CSystemnodePayee& payee, vecPayments){
            if(payee.scriptPubKey == payeeIn) {
                payee.nVotes += nIncrement;
                return;
            }
        }

        CSystemnodePayee c(payeeIn, nIncrement);
        vecPayments.push_back(c);
    }

    bool GetPayee(CScript& payee)
    {
        LOCK(cs_vecSNPayments);

        int nVotes = -1;
        BOOST_FOREACH(CSystemnodePayee& p, vecPayments){
            if(p.nVotes > nVotes){
                payee = p.scriptPubKey;
                nVotes = p.nVotes;
            }
        }

        return (nVotes > -1);
    }

    bool HasPayeeWithVotes(CScript payee, int nVotesReq)
    {
        LOCK(cs_vecSNPayments);

        BOOST_FOREACH(CSystemnodePayee& p, vecPayments){
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
class CSystemnodePaymentWinner
{
public:
    CTxIn vinSystemnode;

    int nBlockHeight;
    CScript payee;
    std::vector<unsigned char> vchSig;

    CSystemnodePaymentWinner() {
        nBlockHeight = 0;
        vinSystemnode = CTxIn();
        payee = CScript();
    }

    CSystemnodePaymentWinner(CTxIn vinIn) {
        nBlockHeight = 0;
        vinSystemnode = vinIn;
        payee = CScript();
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << *(CScriptBase*)(&payee);
        ss << nBlockHeight;
        ss << vinSystemnode.prevout;

        return ss.GetHash();
    }

    void AddPayee(CScript payeeIn){
        payee = payeeIn;
    }


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinSystemnode);
        READWRITE(nBlockHeight);
        READWRITE(*(CScriptBase*)(&payee));
        READWRITE(vchSig);
    }
    bool Sign(CKey& keySystemnode, CPubKey& pubKeySystemnode);
    bool IsValid(CNode* pnode, std::string& strError);
    bool SignatureValid();
    void Relay();

    std::string ToString()
    {
        std::string ret = "";
        ret += vinSystemnode.ToString();
        ret += ", " + boost::lexical_cast<std::string>(nBlockHeight);
        ret += ", " + payee.ToString();
        ret += ", " + boost::lexical_cast<std::string>((int)vchSig.size());
        return ret;
    }
};

//
// Systemnode Payments Class
// Keeps track of who should get paid for which blocks
//

class CSystemnodePayments
{
private:
    int nSyncedFromPeer;
    int nLastBlockHeight;

public:
    std::map<uint256, CSystemnodePaymentWinner> mapSystemnodePayeeVotes;
    std::map<int, CSystemnodeBlockPayees> mapSystemnodeBlocks;
    std::map<COutPoint, int> mapSystemnodesLastVote;

    CSystemnodePayments() {
        nSyncedFromPeer = 0;
        nLastBlockHeight = 0;
    }
    bool AddWinningSystemnode(CSystemnodePaymentWinner& winner);

    void Clear() {
        LOCK2(cs_mapSystemnodeBlocks, cs_mapSystemnodePayeeVotes);
        mapSystemnodeBlocks.clear();
        mapSystemnodePayeeVotes.clear();
    }

    bool ProcessBlock(int nBlockHeight);
    int GetMinSystemnodePaymentsProto() const;
    void ProcessMessageSystemnodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void Sync(CNode* node, int nCountNeeded);
    void CheckAndRemove();
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    bool GetBlockPayee(int nBlockHeight, CScript& payee);
    bool IsScheduled(CSystemnode& sn, int nNotBlockHeight);
    bool CanVote(COutPoint outSystemnode, int nBlockHeight);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees);
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapSystemnodePayeeVotes);
        READWRITE(mapSystemnodeBlocks);
    }
};



#endif
