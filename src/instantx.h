
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef INSTANTX_H
#define INSTANTX_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"

using namespace std;
using namespace boost;

class CConsensusVote;
class CTransaction;
class CTransactionLock;

static const int MIN_INSTANTX_PROTO_VERSION = 70047;

extern map<uint256, CTransaction> mapTxLockReq;
extern map<uint256, CTransactionLock> mapTxLocks;

void ProcessMessageInstantX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

//check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, bool approved, int64_t nBlockHeight);

//process consensus vote message
void ProcessConsensusVote(CConsensusVote& ctx);

// keep transaction locks in memory for an hour
void CleanTransactionLocksList();

class CConsensusVote
{
public:
    CTxIn vinMasternode;
    bool approved;
    uint256 txHash;
    std::vector<unsigned char> vchMasterNodeSignature;
    int nBlockHeight;

    bool SignatureValid();
    bool Sign();

    IMPLEMENT_SERIALIZE
    (
        READWRITE(txHash);
        READWRITE(vinMasternode);
        READWRITE(approved);
        READWRITE(vchMasterNodeSignature);
        READWRITE(nBlockHeight);
    )
};

class CTransactionLock
{
public:
    int nBlockHeight;
    CTransaction tx;
    std::vector<CConsensusVote> vecConsensusVotes;

    bool SignaturesValid();
    int CountSignatures();
    bool AllInFavor();
    void AddSignature(CConsensusVote cv);
    uint256 GetHash()
    {
        return tx.GetHash();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(tx);
        READWRITE(nBlockHeight);
        READWRITE(vecConsensusVotes);
    )
};


#endif