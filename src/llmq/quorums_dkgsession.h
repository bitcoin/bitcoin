// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_DKGSESSION_H
#define BITCOIN_LLMQ_QUORUMS_DKGSESSION_H

#include <batchedlogger.h>

#include <bls/bls_ies.h>
#include <bls/bls_worker.h>

#include <llmq/quorums_utils.h>

class UniValue;

namespace llmq
{

class CFinalCommitment;
class CDKGSession;
class CDKGSessionManager;
class CDKGPendingMessages;

class CDKGLogger : public CBatchedLogger
{
public:
    CDKGLogger(const CDKGSession& _quorumDkg, const std::string& _func);
    CDKGLogger(const std::string& _llmqTypeName, const uint256& _quorumHash, int _height, bool _areWeMember, const std::string& _func);
};

class CDKGContribution
{
public:
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint256 proTxHash;
    BLSVerificationVectorPtr vvec;
    std::shared_ptr<CBLSIESMultiRecipientObjects<CBLSSecretKey>> contributions;
    CBLSSignature sig;

public:
    template<typename Stream>
    inline void SerializeWithoutSig(Stream& s) const
    {
        s << llmqType;
        s << quorumHash;
        s << proTxHash;
        s << *vvec;
        s << *contributions;
    }
    template<typename Stream>
    inline void Serialize(Stream& s) const
    {
        SerializeWithoutSig(s);
        s << sig;
    }
    template<typename Stream>
    inline void Unserialize(Stream& s)
    {
        BLSVerificationVector tmp1;
        CBLSIESMultiRecipientObjects<CBLSSecretKey> tmp2;

        s >> llmqType;
        s >> quorumHash;
        s >> proTxHash;
        s >> tmp1;
        s >> tmp2;
        s >> sig;

        vvec = std::make_shared<BLSVerificationVector>(std::move(tmp1));
        contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>(std::move(tmp2));
    }

    uint256 GetSignHash() const
    {
        CHashWriter hw(SER_GETHASH, 0);
        SerializeWithoutSig(hw);
        hw << CBLSSignature();
        return hw.GetHash();
    }
};

class CDKGComplaint
{
public:
    Consensus::LLMQType llmqType{Consensus::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> badMembers;
    std::vector<bool> complainForMembers;
    CBLSSignature sig;

public:
    CDKGComplaint() = default;
    explicit CDKGComplaint(const Consensus::LLMQParams& params);

    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(proTxHash);
        READWRITE(DYNBITSET(badMembers));
        READWRITE(DYNBITSET(complainForMembers));
        READWRITE(sig);
    }

    uint256 GetSignHash() const
    {
        CDKGComplaint tmp(*this);
        tmp.sig = CBLSSignature();
        return ::SerializeHash(tmp);
    }
};

class CDKGJustification
{
public:
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<std::pair<uint32_t, CBLSSecretKey>> contributions;
    CBLSSignature sig;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(proTxHash);
        READWRITE(contributions);
        READWRITE(sig);
    }

    uint256 GetSignHash() const
    {
        CDKGJustification tmp(*this);
        tmp.sig = CBLSSignature();
        return ::SerializeHash(tmp);
    }
};

// each member commits to a single set of valid members with this message
// then each node aggregate all received premature commitments
// into a single CFinalCommitment, which is only valid if
// enough (>=minSize) premature commitments were aggregated
class CDKGPrematureCommitment
{
public:
    Consensus::LLMQType llmqType{Consensus::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> validMembers;

    CBLSPublicKey quorumPublicKey;
    uint256 quorumVvecHash;

    CBLSSignature quorumSig; // threshold sig share of quorumHash+validMembers+pubKeyHash+vvecHash
    CBLSSignature sig; // single member sig of quorumHash+validMembers+pubKeyHash+vvecHash

public:
    CDKGPrematureCommitment() = default;
    explicit CDKGPrematureCommitment(const Consensus::LLMQParams& params);

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
        READWRITE(proTxHash);
        READWRITE(DYNBITSET(validMembers));
        READWRITE(quorumPublicKey);
        READWRITE(quorumVvecHash);
        READWRITE(quorumSig);
        READWRITE(sig);
    }

    uint256 GetSignHash() const
    {
        return CLLMQUtils::BuildCommitmentHash(llmqType, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);
    }
};

class CDKGMember
{
public:
    CDKGMember(CDeterministicMNCPtr _dmn, size_t _idx);

    CDeterministicMNCPtr dmn;
    size_t idx;
    CBLSId id;

    std::set<uint256> contributions;
    std::set<uint256> complaints;
    std::set<uint256> justifications;
    std::set<uint256> prematureCommitments;

    std::set<uint256> badMemberVotes;
    std::set<uint256> complaintsFromOthers;

    bool bad{false};
    bool badConnection{false};
    bool weComplain{false};
    bool someoneComplain{false};
};

/**
 * The DKG session is a single instance of the DKG process. It is owned and called by CDKGSessionHandler, which passes
 * received DKG messages to the session. The session is not persistent and will loose it's state (the whole object is
 * discarded) when it finishes (after the mining phase) or is aborted.
 *
 * When incoming contributions are received and the verification vector is valid, it is passed to CDKGSessionManager
 * which will store it in the evo DB. Secret key contributions which are meant for the local member are also passed
 * to CDKGSessionManager to store them in the evo DB. If verification of the SK contribution initially fails, it is
 * not passed to CDKGSessionManager. If the justification phase later gives a valid SK contribution from the same
 * member, it is then passed to CDKGSessionManager and after this handled the same way.
 *
 * The contributions stored by CDKGSessionManager are then later loaded by the quorum instances and used for signing
 * sessions, but only if the local node is a member of the quorum.
 */
class CDKGSession
{
    friend class CDKGSessionHandler;
    friend class CDKGSessionManager;
    friend class CDKGLogger;

private:
    const Consensus::LLMQParams& params;

    CBLSWorker& blsWorker;
    CBLSWorkerCache cache;
    CDKGSessionManager& dkgManager;

    const CBlockIndex* pindexQuorum{nullptr};

private:
    std::vector<std::unique_ptr<CDKGMember>> members;
    std::map<uint256, size_t> membersMap;
    std::set<uint256> relayMembers;
    BLSVerificationVectorPtr vvecContribution;
    BLSSecretKeyVector skContributions;

    BLSIdVector memberIds;
    std::vector<BLSVerificationVectorPtr> receivedVvecs;
    // these are not necessarily verified yet. Only trust in what was written to the DB
    BLSSecretKeyVector receivedSkContributions;
    /// Contains the received unverified/encrypted DKG contributions
    std::vector<std::shared_ptr<CBLSIESMultiRecipientObjects<CBLSSecretKey>>> vecEncryptedContributions;

    uint256 myProTxHash;
    CBLSId myId;
    size_t myIdx{(size_t)-1};

    // all indexed by msg hash
    // we expect to only receive a single vvec and contribution per member, but we must also be able to relay
    // conflicting messages as otherwise an attacker might be able to broadcast conflicting (valid+invalid) messages
    // and thus split the quorum. Such members are later removed from the quorum.
    mutable CCriticalSection invCs;
    std::map<uint256, CDKGContribution> contributions GUARDED_BY(invCs);
    std::map<uint256, CDKGComplaint> complaints GUARDED_BY(invCs);
    std::map<uint256, CDKGJustification> justifications GUARDED_BY(invCs);
    std::map<uint256, CDKGPrematureCommitment> prematureCommitments GUARDED_BY(invCs);

    mutable CCriticalSection cs_pending;
    std::vector<size_t> pendingContributionVerifications GUARDED_BY(cs_pending);

    // filled by ReceivePrematureCommitment and used by FinalizeCommitments
    std::set<uint256> validCommitments GUARDED_BY(invCs);

public:
    CDKGSession(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
        params(_params), blsWorker(_blsWorker), cache(_blsWorker), dkgManager(_dkgManager) {}

    bool Init(const CBlockIndex* pindexQuorum, const std::vector<CDeterministicMNCPtr>& mns, const uint256& _myProTxHash);

    size_t GetMyMemberIndex() const { return myIdx; }

    /**
     * The following sets of methods are for the first 4 phases handled in the session. The flow of message calls
     * is identical for all phases:
     * 1. Execute local action (e.g. create/send own contributions)
     * 2. PreVerify incoming messages for this phase. Preverification means that everything from the message is checked
     *    that does not require too much resources for verification. This specifically excludes all CPU intensive BLS
     *    operations.
     * 3. CDKGSessionHandler will collect pre verified messages in batches and perform batched BLS signature verification
     *    on these.
     * 4. ReceiveMessage is called for each pre verified message with a valid signature. ReceiveMessage is also
     *    responsible for further verification of validity (e.g. validate vvecs and SK contributions).
     */

    // Phase 1: contribution
    void Contribute(CDKGPendingMessages& pendingMessages);
    void SendContributions(CDKGPendingMessages& pendingMessages);
    bool PreVerifyMessage(const CDKGContribution& qc, bool& retBan) const;
    void ReceiveMessage(const CDKGContribution& qc, bool& retBan);
    void VerifyPendingContributions();

    // Phase 2: complaint
    void VerifyAndComplain(CDKGPendingMessages& pendingMessages);
    void VerifyConnectionAndMinProtoVersions();
    void SendComplaint(CDKGPendingMessages& pendingMessages);
    bool PreVerifyMessage(const CDKGComplaint& qc, bool& retBan) const;
    void ReceiveMessage(const CDKGComplaint& qc, bool& retBan);

    // Phase 3: justification
    void VerifyAndJustify(CDKGPendingMessages& pendingMessages);
    void SendJustification(CDKGPendingMessages& pendingMessages, const std::set<uint256>& forMembers);
    bool PreVerifyMessage(const CDKGJustification& qj, bool& retBan) const;
    void ReceiveMessage(const CDKGJustification& qj, bool& retBan);

    // Phase 4: commit
    void VerifyAndCommit(CDKGPendingMessages& pendingMessages);
    void SendCommitment(CDKGPendingMessages& pendingMessages);
    bool PreVerifyMessage(const CDKGPrematureCommitment& qc, bool& retBan) const;
    void ReceiveMessage(const CDKGPrematureCommitment& qc, bool& retBan);

    // Phase 5: aggregate/finalize
    std::vector<CFinalCommitment> FinalizeCommitments();

    bool AreWeMember() const { return !myProTxHash.IsNull(); }
    void MarkBadMember(size_t idx);

    void RelayInvToParticipants(const CInv& inv) const;

public:
    CDKGMember* GetMember(const uint256& proTxHash) const;

private:
    bool ShouldSimulateError(const std::string& type);
};

void SetSimulatedDKGErrorRate(const std::string& type, double rate);

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_DKGSESSION_H
