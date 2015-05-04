

// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_PAYMENTS_H
#define MASTERNODE_PAYMENTS_H

#include "key.h"
#include "main.h"
#include "masternode.h"

using namespace std;

class CMasternodePayments;
class CMasternodePaymentWinner;
class CMasternodeBlockPayees;

extern CMasternodePayments masternodePayments;
extern std::map<uint256, CMasternodePaymentWinner> mapMasternodePayeeVotes;
extern std::map<uint256, CMasternodeBlockPayees> mapMasternodeBlocks;

static const int MIN_MNPAYMENTS_PROTO_VERSION = 70066;
#define MNPAYMENTS_SIGNATURES_REQUIRED           11
#define MNPAYMENTS_SIGNATURES_TOTAL              20

void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
bool IsReferenceNode(CTxIn& vin);

class CMasternodePayee : public CTxOut
{
public:
    int nVotes;

    CMasternodePayee() {
        scriptPubKey = CScript();
        nValue = 0;
        nVotes = 0;
    }

    CMasternodePayee(CAmount nValue, CScript payee) {
        scriptPubKey = payee;
        nValue = nValue;
        nVotes = 0;
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

    void AddPayee(CScript payeeIn, int64_t nAmount, int nIncrement){
        BOOST_FOREACH(CMasternodePayee& payee, vecPayments){
            if(payee.scriptPubKey == payeeIn && payee.nValue == nAmount) {
                payee.nVotes += nIncrement;
                return;
            }
        }

        CMasternodePayee c((CAmount)nAmount, payeeIn);
        vecPayments.push_back(c);
    }

    bool GetPayee(CScript& payee)
    {
        int nVotes = -1;
        BOOST_FOREACH(CMasternodePayee& p, vecPayments){
            if(p.nVotes > nVotes){
                payee = p.scriptPubKey; 
                nVotes = p.nVotes;
            }
        }        

        return nVotes > -1;
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
    CTxOut payee;
    std::vector<unsigned char> vchSig;

    CMasternodePaymentWinner() {
        nBlockHeight = 0;
        vinMasternode = CTxIn();
        payee = CTxOut();
    }

    CMasternodePaymentWinner(CTxIn vinIn) {
        nBlockHeight = 0;
        vinMasternode = vinIn;
        payee = CTxOut();
    }

    uint256 GetHash(){
        return Hash(BEGIN(payee), END(payee));
    }

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid();
    bool SignatureValid();
    void Relay();

    void AddPayee(CScript payeeIn, int64_t nAmount){
        payee.scriptPubKey = payeeIn;
        payee.nValue = nAmount;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
        READWRITE(nBlockHeight);
        READWRITE(payee);
        READWRITE(vchSig);
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

    CMasternodePayments() {
        nSyncedFromPeer = 0;
        nLastBlockHeight = 0;
    }

    bool AddWinningMasternode(CMasternodePaymentWinner& winner);
    bool ProcessBlock(int nBlockHeight);
    
    void Sync(CNode* node);
    void CleanPaymentList();
    int LastPayment(CMasternode& mn);

    bool GetBlockPayee(int nBlockHeight, CScript& payee);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);

    void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    std::string GetRequiredPaymentsString(int nBlockHeight);

};


#endif