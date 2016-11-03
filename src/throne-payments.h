

// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef THRONE_PAYMENTS_H
#define THRONE_PAYMENTS_H

#include "key.h"
#include "main.h"
#include "throne.h"
#include <boost/lexical_cast.hpp>

using namespace std;

extern CCriticalSection cs_vecPayments;
extern CCriticalSection cs_mapThroneBlocks;
extern CCriticalSection cs_mapThronePayeeVotes;

class CThronePayments;
class CThronePaymentWinner;
class CThroneBlockPayees;

extern CThronePayments thronePayments;

#define MNPAYMENTS_SIGNATURES_REQUIRED           6
#define MNPAYMENTS_SIGNATURES_TOTAL              10

void ProcessMessageThronePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
bool IsReferenceNode(CTxIn& vin);
bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight);
std::string GetRequiredPaymentsString(int nBlockHeight);
bool IsBlockValueValid(const CBlock& block, int64_t nExpectedValue);
void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees);

void DumpThronePayments();

/** Save Throne Payment Data (mnpayments.dat)
 */
class CThronePaymentDB
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

    CThronePaymentDB();
    bool Write(const CThronePayments &objToSave);
    ReadResult Read(CThronePayments& objToLoad, bool fDryRun = false);
};

class CThronePayee
{
public:
    CScript scriptPubKey;
    int nVotes;

    CThronePayee() {
        scriptPubKey = CScript();
        nVotes = 0;
    }

    CThronePayee(CScript payee, int nVotesIn) {
        scriptPubKey = payee;
        nVotes = nVotesIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(scriptPubKey);
        READWRITE(nVotes);
     }
};

// Keep track of votes for payees from thrones
class CThroneBlockPayees
{
public:
    int nBlockHeight;
    std::vector<CThronePayee> vecPayments;

    CThroneBlockPayees(){
        nBlockHeight = 0;
        vecPayments.clear();
    }
    CThroneBlockPayees(int nBlockHeightIn) {
        nBlockHeight = nBlockHeightIn;
        vecPayments.clear();
    }

    void AddPayee(CScript payeeIn, int nIncrement){
        LOCK(cs_vecPayments);

        BOOST_FOREACH(CThronePayee& payee, vecPayments){
            if(payee.scriptPubKey == payeeIn) {
                payee.nVotes += nIncrement;
                return;
            }
        }

        CThronePayee c(payeeIn, nIncrement);
        vecPayments.push_back(c);
    }

    bool GetPayee(CScript& payee)
    {
        LOCK(cs_vecPayments);

        int nVotes = -1;
        BOOST_FOREACH(CThronePayee& p, vecPayments){
            if(p.nVotes > nVotes){
                payee = p.scriptPubKey;
                nVotes = p.nVotes;
            }
        }

        return (nVotes > -1);
    }

    bool HasPayeeWithVotes(CScript payee, int nVotesReq)
    {
        LOCK(cs_vecPayments);

        BOOST_FOREACH(CThronePayee& p, vecPayments){
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
class CThronePaymentWinner
{
public:
    CTxIn vinThrone;

    int nBlockHeight;
    CScript payee;
    std::vector<unsigned char> vchSig;

    CThronePaymentWinner() {
        nBlockHeight = 0;
        vinThrone = CTxIn();
        payee = CScript();
    }

    CThronePaymentWinner(CTxIn vinIn) {
        nBlockHeight = 0;
        vinThrone = vinIn;
        payee = CScript();
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << payee;
        ss << nBlockHeight;
        ss << vinThrone.prevout;

        return ss.GetHash();
    }

    bool Sign(CKey& keyThrone, CPubKey& pubKeyThrone);
    bool IsValid(CNode* pnode, std::string& strError);
    bool SignatureValid();
    void Relay();

    void AddPayee(CScript payeeIn){
        payee = payeeIn;
    }


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinThrone);
        READWRITE(nBlockHeight);
        READWRITE(payee);
        READWRITE(vchSig);
    }

    std::string ToString()
    {
        std::string ret = "";
        ret += vinThrone.ToString();
        ret += ", " + boost::lexical_cast<std::string>(nBlockHeight);
        ret += ", " + payee.ToString();
        ret += ", " + boost::lexical_cast<std::string>((int)vchSig.size());
        return ret;
    }
};

//
// Throne Payments Class
// Keeps track of who should get paid for which blocks
//

class CThronePayments
{
private:
    int nSyncedFromPeer;
    int nLastBlockHeight;

public:
    std::map<uint256, CThronePaymentWinner> mapThronePayeeVotes;
    std::map<int, CThroneBlockPayees> mapThroneBlocks;
    std::map<uint256, int> mapThronesLastVote; //prevout.hash + prevout.n, nBlockHeight

    CThronePayments() {
        nSyncedFromPeer = 0;
        nLastBlockHeight = 0;
    }

    void Clear() {
        LOCK2(cs_mapThroneBlocks, cs_mapThronePayeeVotes);
        mapThroneBlocks.clear();
        mapThronePayeeVotes.clear();
    }

    bool AddWinningThrone(CThronePaymentWinner& winner);
    bool ProcessBlock(int nBlockHeight);

    void Sync(CNode* node, int nCountNeeded);
    void CleanPaymentList();
    int LastPayment(CThrone& mn);

    bool GetBlockPayee(int nBlockHeight, CScript& payee);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    bool IsScheduled(CThrone& mn, int nNotBlockHeight);

    bool CanVote(COutPoint outThrone, int nBlockHeight) {
        LOCK(cs_mapThronePayeeVotes);

        if(mapThronesLastVote.count(outThrone.hash + outThrone.n)) {
            if(mapThronesLastVote[outThrone.hash + outThrone.n] == nBlockHeight) {
                return false;
            }
        }

        //record this throne voted
        mapThronesLastVote[outThrone.hash + outThrone.n] = nBlockHeight;
        return true;
    }

    int GetMinThronePaymentsProto();
    void ProcessMessageThronePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees);
    std::string ToString() const;
    int GetOldestBlock();
    int GetNewestBlock();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapThronePayeeVotes);
        READWRITE(mapThroneBlocks);
    }
};



#endif
