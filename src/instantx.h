// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef INSTANTX_H
#define INSTANTX_H

#include "net.h"
#include "primitives/transaction.h"

class CTransaction;
class CTxLockVote;
class CTxLockCandidate;

/*
    At 15 signatures, 1/2 of the masternode network can be owned by
    one party without comprimising the security of InstantSend
    (1000/2150.0)**10 = 0.00047382219560689856
    (1000/2900.0)**10 = 2.3769498616783657e-05

    ### getting 5 of 10 signatures w/ 1000 nodes of 2900
    (1000/2900.0)**5 = 0.004875397277841433
*/
static const int INSTANTSEND_SIGNATURES_REQUIRED    = 6;
static const int INSTANTSEND_SIGNATURES_TOTAL       = 10;
static const int DEFAULT_INSTANTSEND_DEPTH          = 5;

static const int MIN_INSTANTSEND_PROTO_VERSION      = 70204;
static const CAmount INSTANTSEND_MIN_FEE            = 0.001 * COIN;

extern bool fEnableInstantSend;
extern int nInstantSendDepth;
extern int nCompleteTXLocks;

extern std::map<uint256, CTransaction> mapLockRequestAccepted;
extern std::map<uint256, CTransaction> mapLockRequestRejected;
extern std::map<uint256, CTxLockVote> mapTxLockVotes;
extern std::map<COutPoint, uint256> mapLockedInputs;


void ProcessMessageInstantSend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

bool IsInstantSendTxValid(const CTransaction& txCandidate);

bool ProcessTxLockRequest(CNode* pfrom, const CTransaction &tx);

int64_t CreateTxLockCandidate(const CTransaction &tx);

//check if we need to vote on this transaction
void CreateTxLockVote(const CTransaction& tx, int nBlockHeight);

//process consensus vote message
bool ProcessTxLockVote(CNode *pnode, CTxLockVote& vote);

void ProcessOrphanTxLockVotes();

//update UI and notify external script if any
void UpdateLockedTransaction(const CTransaction& tx, bool fForceNotification = false);

void LockTransactionInputs(const CTransaction& tx);

// if two conflicting locks are approved by the network, they will cancel out
bool FindConflictingLocks(const CTransaction& tx);

//try to resolve conflicting locks
void ResolveConflicts(const CTransaction& tx);

// keep transaction locks in memory for an hour
void CleanTxLockCandidates();

// verify if transaction is currently locked
bool IsLockedInstandSendTransaction(const uint256 &txHash);

// get the actual uber og accepted lock signatures
int GetTransactionLockSignatures(const uint256 &txHash);

// verify if transaction lock timed out
bool IsTransactionLockTimedOut(const uint256 &txHash);

int64_t GetAverageMasternodeOrphanVoteTime();

class CTxLockVote
{
public:
    CTxIn vinMasternode;
    uint256 txHash;
    int nBlockHeight;
    std::vector<unsigned char> vchMasterNodeSignature;

    // local memory only
    int64_t nOrphanExpireTime;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(txHash);
        READWRITE(vinMasternode);
        READWRITE(vchMasterNodeSignature);
        READWRITE(nBlockHeight);
    }

    uint256 GetHash() const;

    bool Sign();
    bool CheckSignature() const;
};

class CTxLockCandidate
{
public:
    int nBlockHeight;
    uint256 txHash;
    std::vector<CTxLockVote> vecTxLockVotes;
    int nExpirationBlock;
    int nTimeout;

    uint256 GetHash() const { return txHash; }

    bool IsAllVotesValid();
    void AddVote(const CTxLockVote& vote);
    int CountVotes();
};


#endif
