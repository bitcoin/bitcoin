// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MASTERNODEPAYMENTS_H
#define SYSCOIN_MASTERNODEPAYMENTS_H

#include <core_io.h>
#include <key.h>
#include <masternode.h>
#include <net_processing.h>
#include <util/strencodings.h>

class CMasternodePayments;
class CMasternodePaymentVote;
class CMasternodeBlockPayees;

static const int MNPAYMENTS_SIGNATURES_REQUIRED         = 6;
static const int MNPAYMENTS_SIGNATURES_TOTAL            = 10;

//! minimum peer version that can receive and send masternode payment messages,
//  vote for masternode and be elected as a payment winner
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2 = MIN_PEER_PROTO_VERSION;

extern CCriticalSection cs_vecPayees;
extern CCriticalSection cs_mapMasternodeBlocks;
extern CCriticalSection cs_mapMasternodePayeeVotes;

extern CMasternodePayments mnpayments;

/// TODO: all 4 functions do not belong here really, they should be refactored/moved somewhere (main.cpp ?)
bool IsBlockValueValid(const CBlock& block, int nBlockHeight,  const CAmount &blockReward, const CAmount &nFee, std::string& strErrorRet);
bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward, const CAmount& fees, CAmount& nTotalRewardWithMasternodes);
void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount &blockReward, CAmount& fees, CTxOut& txoutMasternodeRet, std::vector<CTxOut>& voutSuperblockRet);
std::string GetRequiredPaymentsString(int nBlockHeight);

class CMasternodePayee
{
private:
    CScript scriptPubKey;
    std::vector<uint256> vecVoteHashes;
public:
	int nStartHeight;
    CMasternodePayee() :
        scriptPubKey(),
        vecVoteHashes(),
		nStartHeight(0)
        {}

    CMasternodePayee(CScript payee, uint256 hashIn, int nStart) :
        scriptPubKey(payee),
        vecVoteHashes(),
		nStartHeight(nStart)
    {
        vecVoteHashes.push_back(hashIn);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CScriptBase*)(&scriptPubKey));
        READWRITE(vecVoteHashes);
		READWRITE(nStartHeight);
    }

    CScript GetPayee() const { return scriptPubKey; }

    void AddVoteHash(uint256 hashIn) { vecVoteHashes.push_back(hashIn); }
    std::vector<uint256> GetVoteHashes() const { return vecVoteHashes; }
    int GetVoteCount() const { return vecVoteHashes.size(); }
};

// Keep track of votes for payees from masternodes
class CMasternodeBlockPayees
{
public:
    int nBlockHeight;
    std::vector<CMasternodePayee> vecPayees;

    CMasternodeBlockPayees() :
        nBlockHeight(0),
        vecPayees()
        {}
    CMasternodeBlockPayees(int nBlockHeightIn) :
        nBlockHeight(nBlockHeightIn),
        vecPayees()
        {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nBlockHeight);
        READWRITE(vecPayees);
    }

    void AddPayee(const CMasternodePaymentVote& vote, CConnman& connman);
    bool GetBestPayee(CScript& payeeRet) const;
	bool GetBestPayee(CScript& payeeRet, int &nStartHeightBlock) const;
    bool HasPayeeWithVotes(const CScript& payeeIn, int nVotesReq, CMasternodePayee& payee) const;

    bool IsTransactionValid(const CTransaction& txNew, const int64_t &nHeight, const CAmount& fee, CAmount& nTotalRewardWithMasternodes) const;

    std::string GetRequiredPaymentsString() const;
};

// vote for the winning payment
class CMasternodePaymentVote
{
public:
    COutPoint masternodeOutpoint;
    int nBlockHeight;
    int nStartHeight;
    CScript payee;
    std::vector<unsigned char> vchSig;

    CMasternodePaymentVote() :
        masternodeOutpoint(),
        nBlockHeight(0),
		nStartHeight(0),
        payee(),
        vchSig()
        {}

    CMasternodePaymentVote(COutPoint outpoint, int nBlockHeight, CScript payee, int nStart) :
        masternodeOutpoint(outpoint),
        nBlockHeight(nBlockHeight),
		nStartHeight(nStart),
        payee(payee),
        vchSig()
        {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
       
        // using new format directly
        READWRITE(masternodeOutpoint);
        
        READWRITE(nBlockHeight);
		READWRITE(nStartHeight);
        READWRITE(*(CScriptBase*)(&payee));
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(vchSig);
        }
    }

    uint256 GetHash() const;
    uint256 GetSignatureHash() const;

    bool Sign();
    bool CheckSignature(const CPubKey& pubKeyMasternode, int nValidationHeight, int &nDos) const;

    bool IsValid(CNode* pnode, int nValidationHeight, std::string& strError, CConnman& connman) const;
    void Relay(CConnman& connman) const;

    bool IsVerified() const { return !vchSig.empty(); }
    void MarkAsNotVerified() { vchSig.clear(); }

    std::string ToString() const;
};

//
// Masternode Payments Class
// Keeps track of who should get paid for which blocks
//

class CMasternodePayments
{
private:
    // masternode count times nStorageCoeff payments blocks should be stored ...
    const float nStorageCoeff;
    // ... but at least nMinBlocksToStore (payments blocks)
    const int nMinBlocksToStore;

    // Keep track of current block height
    int nCachedBlockHeight;

public:
    std::map<uint256, CMasternodePaymentVote> mapMasternodePaymentVotes;
    std::map<int, CMasternodeBlockPayees> mapMasternodeBlocks;
    std::map<COutPoint, int> mapMasternodesLastVote;
    std::map<COutPoint, int> mapMasternodesDidNotVote;

    CMasternodePayments() : nStorageCoeff(1.25), nMinBlocksToStore(5000) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(mapMasternodePaymentVotes);
        READWRITE(mapMasternodeBlocks);
    }

    void Clear();

    bool AddOrUpdatePaymentVote(const CMasternodePaymentVote& vote, CConnman& connman);
    bool HasVerifiedPaymentVote(const uint256& hashIn) const;
    bool ProcessBlock(int nBlockHeight, CConnman& connman);
    void CheckBlockVotes(int nBlockHeight);

    void Sync(CNode* node, CConnman& connman) const;
    void RequestLowDataPaymentBlocks(CNode* pnode, CConnman& connman) const;
    void CheckAndRemove();

    bool GetBlockPayee(int nBlockHeight, CScript& payeeRet) const;
	bool GetBlockPayee(int nBlockHeight, CScript& payee, int &nStartHeightBlock) const;
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight, const CAmount& fee, CAmount& nTotalRewardWithMasternodes) const;
    bool IsScheduled(const masternode_info_t& mnInfo, int nNotBlockHeight) const;

    bool UpdateLastVote(const CMasternodePaymentVote& vote);

    int GetMinMasternodePaymentsProto() const;
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    std::string GetRequiredPaymentsString(int nBlockHeight) const;
    void FillBlockPayee(CMutableTransaction& txNew, int nBlockHeight, CAmount &blockReward, CAmount &fees,  CTxOut& txoutMasternodeRet) const;
    std::string ToString() const;

    int GetBlockCount() const { return mapMasternodeBlocks.size(); }
    int GetVoteCount() const { return mapMasternodePaymentVotes.size(); }

    bool IsEnoughData() const;
    int GetStorageLimit() const;

    void UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman);

    void DoMaintenance() { CheckAndRemove(); }
};

#endif // SYSCOIN_MASTERNODEPAYMENTS_H
