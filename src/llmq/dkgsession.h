// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSION_H
#define BITCOIN_LLMQ_DKGSESSION_H

#include <llmq/commitment.h>

#include <batchedlogger.h>
#include <bls/bls.h>
#include <bls/bls_ies.h>
#include <bls/bls_worker.h>
#include <evo/types.h>

#include <saltedhasher.h>
#include <sync.h>
#include <util/underlying.h>

#include <optional>
#include <unordered_set>

class CActiveMasternodeManager;
class CInv;
class CConnman;
class CDeterministicMN;
class CMasternodeMetaMan;
class CSporkManager;
class PeerManager;
namespace llmq {
class ActiveDKGSession;
class ActiveDKGSessionHandler;
class CDKGDebugManager;
class CDKGPendingMessages;
class CDKGSession;
class CDKGSessionManager;
class CFinalCommitment;
class CQuorumSnapshotManager;
} // namespace llmq

namespace llmq {
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
        s << ToUnderlying(llmqType);
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
        std::vector<CBLSPublicKey> tmp1;
        CBLSIESMultiRecipientObjects<CBLSSecretKey> tmp2;

        s >> llmqType;
        s >> quorumHash;
        s >> proTxHash;
        s >> tmp1;
        s >> tmp2;
        s >> sig;

        vvec = std::make_shared<std::vector<CBLSPublicKey>>(std::move(tmp1));
        contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>(std::move(tmp2));
    }

    [[nodiscard]] uint256 GetSignHash() const
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
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> badMembers;
    std::vector<bool> complainForMembers;
    CBLSSignature sig;

public:
    CDKGComplaint() = default;
    explicit CDKGComplaint(const Consensus::LLMQParams& params) :
            badMembers((size_t)params.size), complainForMembers((size_t)params.size) {};

    SERIALIZE_METHODS(CDKGComplaint, obj)
    {
        READWRITE(
                obj.llmqType,
                obj.quorumHash,
                obj.proTxHash,
                DYNBITSET(obj.badMembers),
                DYNBITSET(obj.complainForMembers),
                obj.sig
                );
    }

    [[nodiscard]] uint256 GetSignHash() const
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
    struct Contribution {
        uint32_t index;
        CBLSSecretKey key;
        SERIALIZE_METHODS(Contribution, obj)
        {
            READWRITE(obj.index, obj.key);
        }
    };
    std::vector<Contribution> contributions;
    CBLSSignature sig;

public:
    SERIALIZE_METHODS(CDKGJustification, obj)
    {
        READWRITE(obj.llmqType, obj.quorumHash, obj.proTxHash, obj.contributions, obj.sig);
    }

    [[nodiscard]] uint256 GetSignHash() const
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
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> validMembers;

    CBLSPublicKey quorumPublicKey;
    uint256 quorumVvecHash;

    CBLSSignature quorumSig; // threshold sig share of quorumHash+validMembers+pubKeyHash+vvecHash
    CBLSSignature sig; // single member sig of quorumHash+validMembers+pubKeyHash+vvecHash

public:
    CDKGPrematureCommitment() = default;
    explicit CDKGPrematureCommitment(const Consensus::LLMQParams& params) :
            validMembers((size_t)params.size) {};

    [[nodiscard]] int CountValidMembers() const
    {
        return int(std::count(validMembers.begin(), validMembers.end(), true));
    }

public:
    SERIALIZE_METHODS(CDKGPrematureCommitment, obj)
    {
        READWRITE(
                obj.llmqType,
                obj.quorumHash,
                obj.proTxHash,
                DYNBITSET(obj.validMembers),
                obj.quorumPublicKey,
                obj.quorumVvecHash,
                obj.quorumSig,
                obj.sig
                );
    }

    [[nodiscard]] uint256 GetSignHash() const
    {
        return BuildCommitmentHash(llmqType, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);
    }
};

class CDKGMember
{
public:
    CDKGMember(const CDeterministicMNCPtr& _dmn, size_t _idx);

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

class DKGError {
public:
    enum type {
        COMPLAIN_LIE = 0,
        COMMIT_OMIT,
        COMMIT_LIE,
        CONTRIBUTION_OMIT,
        CONTRIBUTION_LIE,
        JUSTIFY_OMIT,
        JUSTIFY_LIE,
        _COUNT
    };
    static constexpr DKGError::type from_string(std::string_view in) {
        if (in == "complain-lie") return COMPLAIN_LIE;
        if (in == "commit-omit") return COMMIT_OMIT;
        if (in == "commit-lie") return COMMIT_LIE;
        if (in == "contribution-omit") return CONTRIBUTION_OMIT;
        if (in == "contribution-lie") return CONTRIBUTION_LIE;
        if (in == "justify-lie") return JUSTIFY_LIE;
        if (in == "justify-omit") return JUSTIFY_OMIT;
        return _COUNT;
    }
};

class CDKGLogger : public CBatchedLogger
{
public:
    CDKGLogger(const CDKGSession& _quorumDkg, std::string_view _func, int source_line);
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
    friend class ActiveDKGSession;
    friend class ActiveDKGSessionHandler;
    friend class CDKGSessionHandler;
    friend class CDKGSessionManager;
    friend class CDKGLogger;

private:
    CBLSWorker& blsWorker;
    CBLSWorkerCache cache;
    CDeterministicMNManager& m_dmnman;
    CDKGDebugManager& dkgDebugManager;
    CDKGSessionManager& dkgManager;
    CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const Consensus::LLMQParams& params;
    const CBlockIndex* const m_quorum_base_block_index;

private:
    int quorumIndex{0};
    std::vector<std::unique_ptr<CDKGMember>> members;
    std::map<uint256, size_t> membersMap;
    Uint256HashSet relayMembers;
    BLSVerificationVectorPtr vvecContribution;
    std::vector<CBLSSecretKey> m_sk_contributions;

    std::vector<CBLSId> memberIds;
    std::vector<BLSVerificationVectorPtr> receivedVvecs;
    // these are not necessarily verified yet. Only trust in what was written to the DB
    std::vector<CBLSSecretKey> receivedSkContributions;
    /// Contains the received unverified/encrypted DKG contributions
    std::vector<std::shared_ptr<CBLSIESMultiRecipientObjects<CBLSSecretKey>>> vecEncryptedContributions;

    uint256 myProTxHash;
    CBLSId myId;
    std::optional<size_t> myIdx;

    // all indexed by msg hash
    // we expect to only receive a single vvec and contribution per member, but we must also be able to relay
    // conflicting messages as otherwise an attacker might be able to broadcast conflicting (valid+invalid) messages
    // and thus split the quorum. Such members are later removed from the quorum.
    mutable Mutex invCs;
    std::map<uint256, CDKGContribution> contributions GUARDED_BY(invCs);
    std::map<uint256, CDKGComplaint> complaints GUARDED_BY(invCs);
    std::map<uint256, CDKGJustification> justifications GUARDED_BY(invCs);
    std::map<uint256, CDKGPrematureCommitment> prematureCommitments GUARDED_BY(invCs);

    mutable Mutex cs_pending;
    std::vector<size_t> pendingContributionVerifications GUARDED_BY(cs_pending);

    // filled by ReceivePrematureCommitment and used by FinalizeCommitments
    std::set<uint256> validCommitments GUARDED_BY(invCs);

public:
    CDKGSession(CBLSWorker& _blsWorker, CDeterministicMNManager& dmnman, CDKGDebugManager& _dkgDebugManager,
                CDKGSessionManager& _dkgManager, CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                const CBlockIndex* pQuorumBaseBlockIndex, const Consensus::LLMQParams& _params);
    virtual ~CDKGSession();

    // TODO: remove Init completely
    bool Init(const uint256& _myProTxHash, int _quorumIndex);

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
    virtual void Contribute(CDKGPendingMessages& pendingMessages, PeerManager& peerman) {}
    virtual void SendContributions(CDKGPendingMessages& pendingMessages, PeerManager& peerman) {}
    bool PreVerifyMessage(const CDKGContribution& qc, bool& retBan) const;
    std::optional<CInv> ReceiveMessage(const CDKGContribution& qc) EXCLUSIVE_LOCKS_REQUIRED(!invCs, !cs_pending);
    virtual void VerifyPendingContributions() EXCLUSIVE_LOCKS_REQUIRED(cs_pending) {}

    // Phase 2: complaint
    virtual void VerifyAndComplain(CConnman& connman, CDKGPendingMessages& pendingMessages, PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending) {}
    virtual void VerifyConnectionAndMinProtoVersions(CConnman& connman) const {}
    virtual void SendComplaint(CDKGPendingMessages& pendingMessages, PeerManager& peerman) {}
    bool PreVerifyMessage(const CDKGComplaint& qc, bool& retBan) const;
    std::optional<CInv> ReceiveMessage(const CDKGComplaint& qc) EXCLUSIVE_LOCKS_REQUIRED(!invCs);

    // Phase 3: justification
    virtual void VerifyAndJustify(CDKGPendingMessages& pendingMessages, PeerManager& peerman) EXCLUSIVE_LOCKS_REQUIRED(!invCs) {}
    virtual void SendJustification(CDKGPendingMessages& pendingMessages, PeerManager& peerman, const std::set<uint256>& forMembers) {}
    bool PreVerifyMessage(const CDKGJustification& qj, bool& retBan) const;
    std::optional<CInv> ReceiveMessage(const CDKGJustification& qj) EXCLUSIVE_LOCKS_REQUIRED(!invCs);

    // Phase 4: commit
    virtual void VerifyAndCommit(CDKGPendingMessages& pendingMessages, PeerManager& peerman) {}
    virtual void SendCommitment(CDKGPendingMessages& pendingMessages, PeerManager& peerman) {}
    bool PreVerifyMessage(const CDKGPrematureCommitment& qc, bool& retBan) const;
    std::optional<CInv> ReceiveMessage(const CDKGPrematureCommitment& qc) EXCLUSIVE_LOCKS_REQUIRED(!invCs);

    // Phase 5: aggregate/finalize
    virtual std::vector<CFinalCommitment> FinalizeCommitments() EXCLUSIVE_LOCKS_REQUIRED(!invCs) { return {}; }

    // All Phases 5-in-1 for single-node-quorum
    virtual CFinalCommitment FinalizeSingleCommitment() { return {}; }

public:
    [[nodiscard]] bool AreWeMember() const { return !myProTxHash.IsNull(); }
    [[nodiscard]] CDKGMember* GetMember(const uint256& proTxHash) const;
    [[nodiscard]] CDKGMember* GetMemberAtIndex(size_t index) const;
    [[nodiscard]] std::optional<size_t> GetMyMemberIndex() const { return myIdx; }
    [[nodiscard]] const Uint256HashSet& RelayMembers() const { return relayMembers; }
    [[nodiscard]] const CBlockIndex* BlockIndex() const { return m_quorum_base_block_index; }
    [[nodiscard]] const uint256& ProTx() const { return myProTxHash; }
    [[nodiscard]] Consensus::LLMQType GetType() const { return params.type; }

protected:
    virtual bool MaybeDecrypt(const CBLSIESMultiRecipientObjects<CBLSSecretKey>& obj, size_t idx,
                              CBLSSecretKey& ret_obj, int version)
    {
        return false;
    }

private:
    [[nodiscard]] bool ShouldSimulateError(DKGError::type type) const;
    void MarkBadMember(size_t idx);
};

void SetSimulatedDKGErrorRate(DKGError::type type, double rate);
double GetSimulatedErrorRate(DKGError::type type);

} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSION_H
