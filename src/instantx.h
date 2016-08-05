// Copyright (c) 2009-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef INSTANTX_H
#define INSTANTX_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"

class CConsensusVote;
class CTransaction;
class CTransactionLock;

/*
    At 15 signatures, 1/2 of the masternode network can be owned by
    one party without comprimising the security of InstantX
    (1000/2150.0)**10 = 0.00047382219560689856
    (1000/2900.0)**10 = 2.3769498616783657e-05

    ### getting 5 of 10 signatures w/ 1000 nodes of 2900
    (1000/2900.0)**5 = 0.004875397277841433
*/
static const int INSTANTSEND_SIGNATURES_REQUIRED    = 6;
static const int INSTANTSEND_SIGNATURES_TOTAL       = 10;
static const int DEFAULT_INSTANTSEND_DEPTH          = 5;

static const int MIN_INSTANTSEND_PROTO_VERSION      = 70201;
static const CAmount INSTANTSEND_MIN_FEE            = 1 * CENT;

extern bool fEnableInstantSend;
extern int nInstantSendDepth;
extern int nCompleteTXLocks;

extern std::map<uint256, CTransaction> mapTxLockReq;
extern std::map<uint256, CTransaction> mapTxLockReqRejected;
extern std::map<uint256, CConsensusVote> mapTxLockVote;
extern std::map<COutPoint, uint256> mapLockedInputs;


int64_t CreateNewLock(CTransaction tx);

bool IsInstantSendTxValid(const CTransaction& txCollateral);

void ProcessMessageInstantSend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

//check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, int64_t nBlockHeight);

//process consensus vote message
bool ProcessConsensusVote(CNode *pnode, CConsensusVote& vote);

//update UI and notify external script if any
void UpdateLockedTransaction(CTransaction& tx, bool fForceNotification = false);

void LockTransactionInputs(CTransaction& tx);

// if two conflicting locks are approved by the network, they will cancel out
bool FindConflictingLocks(CTransaction& tx);

//try to resolve conflicting locks
void ResolveConflicts(CTransaction& tx);

// keep transaction locks in memory for an hour
void CleanTransactionLocksList();

// verify if transaction is currently locked
bool IsLockedInstandSendTransaction(uint256 txHash);

// get the actual uber og accepted lock signatures
int GetTransactionLockSignatures(uint256 txHash);

// verify if transaction lock timed out
bool IsTransactionLockTimedOut(uint256 txHash);

int64_t GetAverageVoteTime();

class CConsensusVote
{
public:
    CTxIn vinMasternode;
    uint256 txHash;
    int nBlockHeight;
    std::vector<unsigned char> vchMasterNodeSignature;

    uint256 GetHash() const;

    bool SignatureValid();
    bool Sign();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(txHash);
        READWRITE(vinMasternode);
        READWRITE(vchMasterNodeSignature);
        READWRITE(nBlockHeight);
    }
};

class CTransactionLock
{
public:
    int nBlockHeight;
    uint256 txHash;
    std::vector<CConsensusVote> vecConsensusVotes;
    int nExpiration;
    int nTimeout;

    bool VotesValid();
    int CountVotes();
    void AddVote(CConsensusVote& vote);

    uint256 GetHash()
    {
        return txHash;
    }
};


#endif
