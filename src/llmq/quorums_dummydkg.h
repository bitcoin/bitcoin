// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_DUMMYDKG_H
#define DASH_QUORUMS_DUMMYDKG_H

#include "llmq/quorums_commitment.h"

#include "consensus/params.h"
#include "net.h"
#include "primitives/transaction.h"
#include "sync.h"

#include "bls/bls.h"

#include <map>

class CNode;
class CConnman;

/**
 * Implementation of an insecure dummy DKG
 *
 * This is only used on testnet/devnet/regtest and will NEVER be used on
 * mainnet. It is NOT SECURE AT ALL! It will actually be removed later when the real DKG is introduced.
 *
 * It works by using a deterministic secure vector as the secure polynomial. Everyone can calculate this
 * polynomial by himself, which makes it insecure by definition.
 *
 * The purpose of this dummy implementation is to test final LLMQ commitments and simple PoSe on-chain.
 * The dummy DKG first creates dummy commitments and propagates these to all nodes. They can then create
 * a valid LLMQ commitment from these, which validates with the normal commitment validation code.
 *
 * After these have been mined on-chain, they are indistinguishable from commitments created from the real
 * DKG, making them good enough for testing.
 *
 * The validMembers bitset is created from information of past dummy DKG sessions. If nodes failed to provide
 * the dummy commitments, they will be marked as bad in the next session. This might create some chaos and
 * finalizable commitments, but this is ok and will sort itself out after some sessions.
 */

namespace llmq
{

// This message is only allowed on testnet/devnet/regtest
// If any peer tries to send this message on mainnet, it is banned immediately
// It is used to test commitments on testnet without actually running a full-blown DKG.
class CDummyCommitment
{
public:
    uint8_t llmqType{Consensus::LLMQ_NONE};
    uint256 quorumHash;
    uint16_t signer{(uint16_t)-1};

    std::vector<bool> validMembers;

    CBLSSignature quorumSig;
    CBLSSignature membersSig;

public:
    int CountValidMembers() const
    {
        return (int)std::count(validMembers.begin(), validMembers.end(), true);
    }

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(signer);
        READWRITE(DYNBITSET(validMembers));
        READWRITE(quorumSig);
        READWRITE(membersSig);
    }
};

class CDummyDKGSession
{
public:
    std::set<uint256> badMembers;
    std::map<uint256, CDummyCommitment> dummyCommitments;
    std::map<uint256, std::map<uint256, uint256>> dummyCommitmentsFromMembers;
};

// It simulates the result of a DKG session by deterministically calculating a secret/public key vector
// !!!THIS IS NOT SECURE AT ALL AND WILL NEVER BE USED ON MAINNET!!!
// The whole dummy DKG will be removed when we add the real DKG
class CDummyDKG
{
private:
    CCriticalSection sessionCs;
    std::map<Consensus::LLMQType, CDummyDKGSession> prevSessions;
    std::map<Consensus::LLMQType, CDummyDKGSession> curSessions;

public:
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void ProcessDummyCommitment(NodeId from, const CDummyCommitment& qc);

    void UpdatedBlockTip(const CBlockIndex* pindex, bool fInitialDownload);
    void CreateDummyCommitment(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
    void CreateFinalCommitment(Consensus::LLMQType llmqType, const CBlockIndex* pindex);

    bool HasDummyCommitment(const uint256& hash);
    bool GetDummyCommitment(const uint256& hash, CDummyCommitment& ret);

private:
    std::vector<bool> GetValidMembers(Consensus::LLMQType llmqType, const std::vector<CDeterministicMNCPtr>& members);
    BLSSecretKeyVector BuildDeterministicSvec(Consensus::LLMQType llmqType, const uint256& quorumHash);
    BLSPublicKeyVector BuildVvec(const BLSSecretKeyVector& svec);
};

extern CDummyDKG* quorumDummyDKG;

}

#endif//DASH_QUORUMS_DUMMYDKG_H
