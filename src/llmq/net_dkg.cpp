// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/net_dkg.h>

#include <active/dkgsessionhandler.h>
#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <hash.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsession.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/net_quorum.h>
#include <llmq/options.h>
#include <llmq/quorumsman.h>
#include <llmq/utils.h>
#include <masternode/meta.h>
#include <msg_result.h>
#include <net.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <unordered_lru_cache.h>
#include <util/std23.h>
#include <util/thread.h>
#include <validation.h>

namespace llmq {

namespace {
// Upper bound on the serialized size of a well-formed DKG message of the given
// type for the given quorum params. Used to reject oversized payloads at intake
// before any deserialization or retention, which closes the low-cost memory
// amplification window (a legitimate message is bounded by quorum params, far
// below the 3 MiB transport cap). Generous slack is added and the result is
// clamped to a hard ceiling so a future params change can never silently re-open
// the full transport window.
size_t MaxDKGMessageSize(std::string_view msg_type, const Consensus::LLMQParams& params)
{
    constexpr size_t COMPACT = 5;          // max CompactSize for any realistic count
    constexpr size_t PREFIX = 1 + 32 + 32; // llmqType + quorumHash + proTxHash
    constexpr size_t PUBKEY = BLS_CURVE_PUBKEY_SIZE; // 48
    constexpr size_t SIG = BLS_CURVE_SIG_SIZE;       // 96
    constexpr size_t SECKEY = BLS_CURVE_SECKEY_SIZE; // 32
    constexpr size_t BLOB = COMPACT + 128; // encrypted seckey blob, generous
    constexpr size_t SLACK = 1024;
    constexpr size_t HARD_CEILING = size_t{1} << 20; // 1 MiB

    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    const size_t threshold = params.threshold > 0 ? static_cast<size_t>(params.threshold) : 0;

    size_t cap = 0;
    if (msg_type == NetMsgType::QCONTRIB) {
        // llmqType/quorumHash/proTxHash + vvec + contributions(IES) + sig
        cap = PREFIX + (COMPACT + threshold * PUBKEY) + (PUBKEY + 32 + COMPACT + size * BLOB) + SIG;
    } else if (msg_type == NetMsgType::QJUSTIFICATION) {
        // ... + contributions(index u32 + seckey) + sig
        cap = PREFIX + (COMPACT + size * (4 + SECKEY)) + SIG;
    } else if (msg_type == NetMsgType::QCOMPLAINT) {
        // ... + 2 dynamic bitsets (badMembers, complainForMembers) + sig
        cap = PREFIX + 2 * (COMPACT + (size + 7) / 8) + SIG;
    } else if (msg_type == NetMsgType::QPCOMMITMENT) {
        // ... + validMembers bitset + quorumPublicKey + quorumVvecHash + quorumSig + sig
        cap = PREFIX + (COMPACT + (size + 7) / 8) + PUBKEY + 32 + 2 * SIG;
    } else {
        return HARD_CEILING;
    }
    cap += SLACK;
    return cap < HARD_CEILING ? cap : HARD_CEILING;
}

constexpr size_t DKG_MSG_PREFIX_SIZE = 1 + 32 + 32; // llmqType + quorumHash + proTxHash

// The framing walks below use throwing stream primitives; truncation anywhere
// surfaces as an exception the dispatcher turns into a reject. They return
// false on a params bound violation and true only when the whole payload was
// consumed. BLS encodings have fixed wire sizes, so the walks only establish
// that the bytes are present; decoding happens once, on the DKG worker.

// Reads a DYNBITSET bounded by @p max_size and returns its bit count. Reuses
// ReadFixedBitSet -- the exact deserializer the typed path uses -- so
// truncation and padding-bit handling cannot diverge.
uint64_t ReadBoundedDynBitset(CDataStream& ds, size_t max_size)
{
    const uint64_t bit_count = ReadCompactSize(ds);
    if (bit_count > max_size) {
        throw std::ios_base::failure("dynamic bitset exceeds quorum size");
    }
    std::vector<bool> bits;
    ReadFixedBitSet(ds, bits, bit_count);
    return bit_count;
}

bool CheckContributionWireStructure(CDataStream& ds, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    const size_t min_size = params.minSize > 0 ? static_cast<size_t>(params.minSize) : 0;
    const size_t threshold = params.threshold > 0 ? static_cast<size_t>(params.threshold) : 0;

    ds.ignore(DKG_MSG_PREFIX_SIZE);
    if (ReadCompactSize(ds) != threshold) { // vvec
        return false;
    }
    ds.ignore(threshold * CBLSPublicKey::SerSize);
    ds.ignore(CBLSPublicKey::SerSize + 32); // IES ephemeralPubKey + ivSeed
    const uint64_t blob_count = ReadCompactSize(ds);
    if (blob_count < min_size || blob_count > size) {
        return false;
    }
    for (uint64_t i = 0; i < blob_count; ++i) {
        ds.ignore(ReadCompactSize(ds)); // encrypted contribution blob
    }
    ds.ignore(CBLSSignature::SerSize);
    return ds.empty();
}

bool CheckComplaintWireStructure(CDataStream& ds, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;

    ds.ignore(DKG_MSG_PREFIX_SIZE);
    const uint64_t bad_members = ReadBoundedDynBitset(ds, size);
    const uint64_t complain_for_members = ReadBoundedDynBitset(ds, size);
    if (bad_members != complain_for_members) {
        return false;
    }
    ds.ignore(CBLSSignature::SerSize);
    return ds.empty();
}

bool CheckJustificationWireStructure(CDataStream& ds, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;

    ds.ignore(DKG_MSG_PREFIX_SIZE);
    const uint64_t contribution_count = ReadCompactSize(ds);
    if (contribution_count > size) {
        return false;
    }
    ds.ignore(contribution_count * (4 + CBLSSecretKey::SerSize)); // {u32 index, encrypted key share}
    ds.ignore(CBLSSignature::SerSize);
    return ds.empty();
}

bool CheckPrematureCommitmentWireStructure(CDataStream& ds, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;

    ds.ignore(DKG_MSG_PREFIX_SIZE);
    ReadBoundedDynBitset(ds, size); // validMembers
    // quorumPublicKey + quorumVvecHash + quorumSig + sig
    ds.ignore(CBLSPublicKey::SerSize + 32 + 2 * CBLSSignature::SerSize);
    return ds.empty();
}

} // namespace

bool CheckDKGMessageWireStructure(std::string_view msg_type, const CDataStream& payload,
                                  const Consensus::LLMQParams& params)
{
    // Walk a copy so the caller's read position (and the bytes backing its
    // inventory hash) survive. The copy is a memcpy bounded by the size cap
    // applied just before this call -- negligible next to the BLS point
    // decompression that deserializing the payload here would have cost.
    CDataStream ds{payload};
    try {
        if (msg_type == NetMsgType::QCONTRIB) {
            return CheckContributionWireStructure(ds, params);
        } else if (msg_type == NetMsgType::QCOMPLAINT) {
            return CheckComplaintWireStructure(ds, params);
        } else if (msg_type == NetMsgType::QJUSTIFICATION) {
            return CheckJustificationWireStructure(ds, params);
        } else if (msg_type == NetMsgType::QPCOMMITMENT) {
            return CheckPrematureCommitmentWireStructure(ds, params);
        }
    } catch (const std::exception&) {
    }
    return false;
}

bool CheckDKGMessageStructure(const CDKGContribution& qc, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    const size_t min_size = params.minSize > 0 ? static_cast<size_t>(params.minSize) : 0;
    const size_t threshold = params.threshold > 0 ? static_cast<size_t>(params.threshold) : 0;
    return qc.vvec != nullptr && qc.vvec->size() == threshold && qc.contributions != nullptr &&
           qc.contributions->blobs.size() >= min_size && qc.contributions->blobs.size() <= size;
}

bool CheckDKGMessageStructure(const CDKGComplaint& qc, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    return qc.badMembers.size() == qc.complainForMembers.size() && qc.badMembers.size() <= size;
}

bool CheckDKGMessageStructure(const CDKGJustification& qj, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    return qj.contributions.size() <= size;
}

bool CheckDKGMessageStructure(const CDKGPrematureCommitment& qc, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    return qc.validMembers.size() <= size;
}

namespace {

// returns a set of NodeIds which sent invalid messages
template <typename Message>
std::unordered_set<NodeId> BatchVerifyMessageSigs(CDKGSession& session,
                                                  const std::vector<std::pair<NodeId, std::shared_ptr<Message>>>& messages)
{
    if (messages.empty()) {
        return {};
    }

    std::unordered_set<NodeId> ret;
    bool revertToSingleVerification = false;
    FastRandomContext rng;

    CBLSSignature aggSig;
    std::vector<CBLSPublicKey> pubKeys;
    std::vector<uint256> messageHashes;
    Uint256HashSet messageHashesSet;
    pubKeys.reserve(messages.size());
    messageHashes.reserve(messages.size());
    bool first = true;
    for (const auto& [nodeId, msg] : messages) {
        auto member = session.GetMember(msg->proTxHash);
        if (!member) {
            // should not happen as it was verified before
            ret.emplace(nodeId);
            continue;
        }

        // An invalid signature must never reach AggregateInsecure(), which asserts
        // that both operands are valid. Mark the sender bad and skip it instead of
        // aggregating it. This guard is mandatory: it covers every
        // message type the batch verifier is instantiated for, including those whose
        // per-message PreVerifyMessage does not (yet) reject invalid signatures.
        // Note: 'first' below tracks the first *accumulated* (valid) signature, not
        // the first *examined* message, so skipping leading invalid sigs is safe.
        if (!msg->sig.IsValid()) {
            ret.emplace(nodeId);
            continue;
        }
        CBLSPublicKey pubKey{member->dmn->pdmnState->pubKeyOperator.Get()};
        if (!pubKey.IsValid()) {
            // should not happen as operator keys are validated on registration
            ret.emplace(nodeId);
            continue;
        }

        const CBLSSecretKey weight{MakeBatchVerificationWeight(rng)};
        CBLSSignature weightedSig{msg->sig};
        weightedSig.MulInsecure(weight);
        pubKey.MulInsecure(weight);

        if (first) {
            aggSig = weightedSig;
        } else {
            aggSig.AggregateInsecure(weightedSig);
        }
        first = false;

        auto msgHash = msg->GetSignHash();
        if (!messageHashesSet.emplace(msgHash).second) {
            // can only happen in 2 cases:
            // 1. Someone sent us the same message twice but with differing signature, meaning that at least one of them
            //    must be invalid. In this case, we'd have to revert to single message verification nevertheless
            // 2. Someone managed to find a way to create two different binary representations of a message that deserializes
            //    to the same object representation. This would be some form of malleability. However, this shouldn't be
            //    possible as only deterministic/unique BLS signatures and very simple data types are involved
            revertToSingleVerification = true;
            break;
        }

        pubKeys.emplace_back(pubKey);
        messageHashes.emplace_back(msgHash);
    }
    if (!revertToSingleVerification) {
        if (pubKeys.empty()) {
            // Every message had an unknown member or invalid signature; all such
            // senders are already in ret. VerifyInsecureAggregated() asserts that
            // the pubkey/hash spans are non-empty, so bail out here.
            return ret;
        }
        if (aggSig.VerifyInsecureAggregated(pubKeys, messageHashes)) {
            // all good
            return ret;
        }

        // are all messages from the same node?
        bool nodeIdsAllSame = std::adjacent_find(messages.begin(), messages.end(),
                                                 [](const auto& first, const auto& second) {
                                                     return first.first != second.first;
                                                 }) == messages.end();

        // if yes, take a short path and return a set with only him
        if (nodeIdsAllSame) {
            ret.emplace(messages[0].first);
            return ret;
        }
        // different nodes, let's figure out who are the bad ones
    }

    for (const auto& [nodeId, msg] : messages) {
        if (ret.count(nodeId)) {
            continue;
        }

        auto member = session.GetMember(msg->proTxHash);
        if (member == nullptr || !msg->sig.IsValid()) {
            // Examined messages with these properties are already in ret, but the
            // early break on a duplicate hash above can leave some unexamined.
            // Stay defensive: never dereference a null member or verify an invalid sig.
            ret.emplace(nodeId);
            continue;
        }
        bool valid = msg->sig.VerifyInsecure(member->dmn->pdmnState->pubKeyOperator.Get(), msg->GetSignHash());
        if (!valid) {
            ret.emplace(nodeId);
        }
    }
    return ret;
}

void RelayInvToParticipants(const CDKGSession& session, const CConnman& connman, PeerManagerInternal& peerman,
                            const CInv& inv)
{
    CDKGLogger logger(session, __func__, __LINE__);
    std::stringstream ss;
    const auto& relayMembers = session.RelayMembers();
    for (const auto& r : relayMembers) {
        ss << r.ToString().substr(0, 4) << " | ";
    }
    logger.Batch("RelayInvToParticipants inv[%s] relayMembers[%d] GetNodeCount[%d] GetNetworkActive[%d] "
                 "HasMasternodeQuorumNodes[%d] for quorumHash[%s] forMember[%s] relayMembers[%s]",
                 inv.ToString(), relayMembers.size(), connman.GetNodeCount(ConnectionDirection::Both),
                 connman.GetNetworkActive(),
                 connman.HasMasternodeQuorumNodes(session.GetType(), session.BlockIndex()->GetBlockHash()),
                 session.BlockIndex()->GetBlockHash().ToString(), session.ProTx().ToString().substr(0, 4), ss.str());

    std::stringstream ss2;
    connman.ForEachNode([&](const CNode* pnode) {
        if (pnode->qwatch ||
            (!pnode->GetVerifiedProRegTxHash().IsNull() && (relayMembers.count(pnode->GetVerifiedProRegTxHash()) != 0))) {
            peerman.PeerPushInventory(pnode->GetId(), inv);
        }

        if (pnode->GetVerifiedProRegTxHash().IsNull()) {
            logger.Batch("node[%d:%s] not mn", pnode->GetId(), pnode->m_addr_name);
        } else if (relayMembers.count(pnode->GetVerifiedProRegTxHash()) == 0) {
            ss2 << pnode->GetVerifiedProRegTxHash().ToString().substr(0, 4) << " | ";
        }
    });
    logger.Batch("forMember[%s] NOTrelayMembers[%s]", session.ProTx().ToString().substr(0, 4), ss2.str());
    logger.Flush();
}

template <typename Message>
void EnqueueOwn(CDKGPendingMessages& pending, const uint256& own_protx, const Message& msg)
{
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << msg;
    auto pm = std::make_shared<CDataStream>(std::move(ds));
    CHashWriter hw(SER_GETHASH, 0);
    hw.write(AsWritableBytes(Span{*pm}));
    pending.PushPendingMessage(/*from=*/-1, own_protx, std::move(pm), hw.GetHash());
}

// The single typed deserialization pass over a queued payload, followed by the
// param-derived structural bounds. Returns nullptr on failure; caller scores.
template <typename Message>
std::shared_ptr<Message> DeserializeCheckedDKGMessage(CDataStream& ds, const Consensus::LLMQParams& params,
                                                      NodeId nodeId)
{
    auto msg = std::make_shared<Message>();
    try {
        ds >> *msg;
    } catch (...) {
        LogPrint(BCLog::LLMQ_DKG, "%s -- failed to deserialize message, peer=%d\n", __func__, nodeId);
        return nullptr;
    }
    if (!ds.empty()) {
        LogPrint(BCLog::LLMQ_DKG, "%s -- failed to deserialize message, peer=%d\n", __func__, nodeId);
        return nullptr;
    }
    if (!CheckDKGMessageStructure(*msg, params)) {
        LogPrint(BCLog::LLMQ_DKG, "%s -- message failed structure check, peer=%d\n", __func__, nodeId);
        return nullptr;
    }
    return msg;
}

template <typename Message>
bool ProcessPendingMessageBatch(const CConnman& connman, CDKGSession& session, const Consensus::LLMQParams& params,
                                CDKGPendingMessages& pendingMessages, PeerManagerInternal& peerman, size_t maxCount)
{
    auto msgs = pendingMessages.PopPendingMessages(maxCount);
    if (msgs.empty()) {
        return false;
    }

    std::vector<std::pair<NodeId, std::shared_ptr<Message>>> preverifiedMessages;
    preverifiedMessages.reserve(msgs.size());

    for (const auto& p : msgs) {
        const NodeId& nodeId = p.first;
        auto msg = DeserializeCheckedDKGMessage<Message>(*p.second, params, nodeId);
        if (!msg) {
            peerman.PeerMisbehaving(nodeId, 100);
            continue;
        }
        bool ban = false;
        if (!session.PreVerifyMessage(*msg, ban)) {
            if (ban) {
                LogPrint(BCLog::LLMQ_DKG, "%s -- banning node due to failed preverification, peer=%d\n", __func__, nodeId);
                peerman.PeerMisbehaving(nodeId, 100);
            }
            LogPrint(BCLog::LLMQ_DKG, "%s -- skipping message due to failed preverification, peer=%d\n", __func__, nodeId);
            continue;
        }
        preverifiedMessages.emplace_back(nodeId, std::move(msg));
    }
    if (preverifiedMessages.empty()) {
        return true;
    }

    auto badNodes = BatchVerifyMessageSigs(session, preverifiedMessages);
    if (!badNodes.empty()) {
        for (auto nodeId : badNodes) {
            LogPrint(BCLog::LLMQ_DKG, "%s -- failed to verify signature, peer=%d\n", __func__, nodeId);
            peerman.PeerMisbehaving(nodeId, 100);
        }
    }

    for (const auto& p : preverifiedMessages) {
        const NodeId& nodeId = p.first;
        if (badNodes.count(nodeId)) {
            continue;
        }
        const std::optional<CInv> inv = session.ReceiveMessage(*p.second);
        if (inv) {
            RelayInvToParticipants(session, connman, peerman, *inv);
        }
    }

    return true;
}
} // namespace


NetDKG::NetDKG(PeerManagerInternal* peer_manager, const CSporkManager& sporkman, CDKGSessionManager& qdkgsman,
               const ChainstateManager& chainman, CQuorumManager& qman, QuorumRole& role) :
    NetHandler(peer_manager),
    m_qdkgsman{qdkgsman},
    m_qman{qman},
    m_sporkman{sporkman},
    m_chainman{chainman},
    m_active{nullptr}
{
    m_qdkgsman.InitializeHandlers([](const Consensus::LLMQParams& llmq_params,
                                     [[maybe_unused]] int quorum_idx) -> std::unique_ptr<CDKGSessionHandler> {
        return std::make_unique<CDKGSessionHandler>(llmq_params);
    });
    m_qman.ConnectManagers(&role, &m_qdkgsman);
}

NetDKG::NetDKG(PeerManagerInternal* peer_manager, const CSporkManager& sporkman, CDKGSessionManager& qdkgsman,
               const ChainstateManager& chainman, bool quorums_watch, CQuorumManager& qman, QuorumRole& role,
               CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
               CDKGDebugManager& dkgdbgman, CQuorumBlockProcessor& qblockman, CQuorumSnapshotManager& qsnapman,
               const CActiveMasternodeManager& mn_activeman, CConnman& connman) :
    NetHandler(peer_manager),
    m_qdkgsman{qdkgsman},
    m_qman{qman},
    m_sporkman{sporkman},
    m_chainman{chainman},
    m_active{std::make_unique<ActiveDKG>(ActiveDKG{dmnman, mn_metaman, dkgdbgman, qblockman, qsnapman, connman})}
{
    m_qdkgsman.InitializeHandlers(
        [&](const Consensus::LLMQParams& llmq_params, int quorum_idx) -> std::unique_ptr<ActiveDKGSessionHandler> {
            return std::make_unique<ActiveDKGSessionHandler>(bls_worker, dmnman, mn_metaman, dkgdbgman, qdkgsman,
                                                             qblockman, qsnapman, mn_activeman, chainman, sporkman,
                                                             llmq_params, quorums_watch, quorum_idx);
        });
    m_qman.ConnectManagers(&role, &m_qdkgsman);
}

NetDKG::~NetDKG()
{
    Stop();
    m_qman.DisconnectManagers();
}

void NetDKG::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (!IsQuorumDKGEnabled(m_sporkman)) return;

    if (msg_type != NetMsgType::QCONTRIB && msg_type != NetMsgType::QCOMPLAINT && msg_type != NetMsgType::QJUSTIFICATION &&
        msg_type != NetMsgType::QPCOMMITMENT && msg_type != NetMsgType::QWATCH) {
        return;
    }

    if (msg_type == NetMsgType::QWATCH) {
        if (m_active == nullptr) {
            // non-masternodes should never receive this
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 10);
            return;
        }
        pfrom.qwatch = true;
        return;
    }

    // Observer handlers have no DKG worker and therefore cannot validate or
    // drain queued payloads. Observers learn completed quorums through the
    // quorum-data path; retaining round messages here would only create
    // process-lifetime state.
    if (m_active == nullptr) {
        LogPrint(BCLog::LLMQ_DKG, "NetDKG -- ignoring %s in observer mode\n", msg_type);
        return;
    }

    // Pushed DKG messages (QCONTRIB/QCOMPLAINT/QJUSTIFICATION/QPCOMMITMENT) retain
    // attacker-controlled payloads, so they must originate from an MNAuth-verified
    // masternode. qwatch is unauthenticated (any peer can set it via QWATCH) and is
    // only meaningful for pull/observation paths; it must not bypass this gate.
    // Read once: the same value keys the retention quota below
    const uint256 sender_protx = pfrom.GetVerifiedProRegTxHash();
    if (sender_protx.IsNull()) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 10, "DKG message from non-verified peer");
        return;
    }

    if (vRecv.size() < sizeof(uint8_t) + sizeof(uint256)) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100, "malformed DKG message");
        return;
    }

    // Peek the prefix without touching vRecv's read position; the full payload
    // stays available for the framing walk, inventory hash, and retention below
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    SpanReader{vRecv.GetType(), vRecv.GetVersion(), MakeUCharSpan(vRecv)} >> llmqType >> quorumHash;

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintf("NetDKG -- invalid llmqType [%d]\n", std23::to_underlying(llmqType));
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
        return;
    }
    const auto& llmq_params = llmq_params_opt.value();

    int quorumIndex{-1};
    {
        LOCK(cs_indexed_quorums_cache);
        if (!indexed_quorums_cache.IsInitialized()) {
            indexed_quorums_cache.Init(m_chainman.GetConsensus());
        }
        indexed_quorums_cache.get(llmqType, quorumHash, quorumIndex);
    }

    if (quorumIndex == -1) {
        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main,
                                                             return m_chainman.m_blockman.LookupBlockIndex(quorumHash));
        if (pQuorumBaseBlockIndex == nullptr) {
            LogPrintf("NetDKG -- unknown quorumHash %s\n", quorumHash.ToString());
            // NOTE: do not insta-ban for this, we might be lagging behind
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 10);
            return;
        }
        // Genesis (or any parentless index) is never a valid quorum base.
        if (pQuorumBaseBlockIndex->pprev == nullptr) {
            LogPrintf("NetDKG -- parentless quorumHash %s\n", quorumHash.ToString());
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
            return;
        }
        if (!m_chainman.IsQuorumTypeEnabled(llmqType, pQuorumBaseBlockIndex->pprev)) {
            LogPrintf("NetDKG -- llmqType [%d] quorums aren't active\n", std23::to_underlying(llmqType));
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
            return;
        }
        quorumIndex = pQuorumBaseBlockIndex->nHeight % llmq_params.dkgInterval;
        const int quorumIndexMax = IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex)
                                       ? llmq_params.signingActiveQuorumCount - 1
                                       : 0;
        if (quorumIndex > quorumIndexMax) {
            LogPrintf("NetDKG -- invalid quorumHash %s\n", quorumHash.ToString());
            m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
            return;
        }
    }

    // Reject oversized payloads before any deserialization or retention. A
    // well-formed DKG message is bounded by quorum params; anything larger is an
    // amplification attempt against the per-peer pending queue.
    if (vRecv.size() > MaxDKGMessageSize(msg_type, llmq_params)) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100, "oversized DKG message");
        return;
    }

    // Framing-only pre-validation before retention; BLS decoding happens
    // exactly once, on the DKG worker, after retention.
    if (!CheckDKGMessageWireStructure(msg_type, vRecv, llmq_params)) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100, "malformed DKG message");
        return;
    }

    int inv_type = 0;
    if (msg_type == NetMsgType::QCONTRIB)
        inv_type = MSG_QUORUM_CONTRIB;
    else if (msg_type == NetMsgType::QCOMPLAINT)
        inv_type = MSG_QUORUM_COMPLAINT;
    else if (msg_type == NetMsgType::QJUSTIFICATION)
        inv_type = MSG_QUORUM_JUSTIFICATION;
    else if (msg_type == NetMsgType::QPCOMMITMENT)
        inv_type = MSG_QUORUM_PREMATURE_COMMITMENT;
    Assume(inv_type != 0); // guarded by the early-return above

    auto pm = std::make_shared<CDataStream>(std::move(vRecv));
    CHashWriter hw(SER_GETHASH, 0);
    hw.write(AsWritableBytes(Span{*pm}));
    const uint256 hash = hw.GetHash();

    const NodeId from = pfrom.GetId();

    // DKG messages are only ever sent in reply to a GETDATA (see NetDKG::ProcessGetData), so one we
    // never asked this peer for was pushed at us and must not reach the pending queues, where it
    // would be retained until a worker gets around to verifying its signature. A bare announcement
    // deliberately does not qualify: it would let the peer authorise its own payload by sending INV
    // first.
    //
    // This check runs last so that every pre-existing rejection above -- and the heavier penalty it
    // carries -- is unchanged. It therefore bounds retention and signature verification, not the
    // parsing and structural validation above, which an unsolicited sender still gets to trigger.
    const CInv inv{static_cast<uint32_t>(inv_type), hash};
    if (WITH_LOCK(::cs_main, return m_peer_manager->PeerConsumeGetDataResponse(from, inv)) ==
        GetDataResponse::UNREQUESTED) {
        LogPrint(BCLog::LLMQ_DKG, "NetDKG -- received unrequested %s %s, peer=%d\n", msg_type, hash.ToString(), from);
        m_peer_manager->PeerMisbehaving(from, UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE, "unrequested DKG message");
        return;
    }

    const bool dispatched = m_qdkgsman.DoForHandler({llmqType, quorumIndex}, [&](CDKGSessionHandler& handler) {
        CDKGPendingMessages* pending = nullptr;
        switch (inv_type) {
        case MSG_QUORUM_CONTRIB:
            pending = &handler.pendingContributions;
            break;
        case MSG_QUORUM_COMPLAINT:
            pending = &handler.pendingComplaints;
            break;
        case MSG_QUORUM_JUSTIFICATION:
            pending = &handler.pendingJustifications;
            break;
        case MSG_QUORUM_PREMATURE_COMMITMENT:
            pending = &handler.pendingPrematureCommitments;
            break;
        }
        Assume(pending != nullptr);
        pending->PushPendingMessage(from, sender_protx, std::move(pm), hash);
    });
    if (!dispatched) {
        LogPrintf("NetDKG -- no session handlers for quorumIndex [%d]\n", quorumIndex);
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 100);
        return;
    }

    WITH_LOCK(cs_indexed_quorums_cache, indexed_quorums_cache.insert(llmqType, quorumHash, quorumIndex));
}

bool NetDKG::AlreadyHave(const CInv& inv)
{
    switch (inv.type) {
    case MSG_QUORUM_CONTRIB:
    case MSG_QUORUM_COMPLAINT:
    case MSG_QUORUM_JUSTIFICATION:
    case MSG_QUORUM_PREMATURE_COMMITMENT: {
        // Observers have no worker that could consume or validate DKG round
        // payloads, so do not request them in response to inventory.
        if (m_active == nullptr) return true;
        if (!IsQuorumDKGEnabled(m_sporkman)) return false;
        bool seen = false;
        m_qdkgsman.ForEachHandler([&](const CDKGSessionHandler& h) {
            if (seen) return;
            if (h.pendingContributions.HasSeen(inv.hash) || h.pendingComplaints.HasSeen(inv.hash) ||
                h.pendingJustifications.HasSeen(inv.hash) || h.pendingPrematureCommitments.HasSeen(inv.hash)) {
                seen = true;
            }
        });
        return seen;
    }
    }
    return false;
}

bool NetDKG::ProcessGetData(CNode& pfrom, const CInv& inv, const CNetMsgMaker& msgMaker)
{
    // Observer mode holds no CConnman, so this null check guards the replies
    // below rather than merely short-circuiting them. It costs no coverage: the
    // Get* calls return false by construction in observer mode.
    if (m_active == nullptr) return false;

    switch (inv.type) {
    case MSG_QUORUM_CONTRIB: {
        CDKGContribution o;
        if (m_qdkgsman.GetContribution(inv.hash, o)) {
            m_active->connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QCONTRIB, o));
            return true;
        }
        return false;
    }
    case MSG_QUORUM_COMPLAINT: {
        CDKGComplaint o;
        if (m_qdkgsman.GetComplaint(inv.hash, o)) {
            m_active->connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QCOMPLAINT, o));
            return true;
        }
        return false;
    }
    case MSG_QUORUM_JUSTIFICATION: {
        CDKGJustification o;
        if (m_qdkgsman.GetJustification(inv.hash, o)) {
            m_active->connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QJUSTIFICATION, o));
            return true;
        }
        return false;
    }
    case MSG_QUORUM_PREMATURE_COMMITMENT: {
        CDKGPrematureCommitment o;
        if (m_qdkgsman.GetPrematureCommitment(inv.hash, o)) {
            m_active->connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::QPCOMMITMENT, o));
            return true;
        }
        return false;
    }
    }
    return false;
}

void NetDKG::Start()
{
    if (m_active == nullptr) return;
    if (!m_phase_threads.empty()) {
        throw std::runtime_error("Tried to start PhaseHandlerThreads again.");
    }

    m_qdkgsman.ForEachHandler([this](CDKGSessionHandler& base) {
        auto& handler = dynamic_cast<ActiveDKGSessionHandler&>(base);
        std::string thread_name = strprintf("llmq-%d-%d", std23::to_underlying(handler.params.type), handler.QuorumIndex());
        m_phase_threads.emplace_back([this, name = std::move(thread_name), &handler] {
            util::TraceThread(name.c_str(), [this, &handler] { PhaseHandlerThread(handler); });
        });
    });
}

void NetDKG::Stop()
{
    Interrupt();
    for (auto& t : m_phase_threads) {
        if (t.joinable()) t.join();
    }
    m_phase_threads.clear();
}

void NetDKG::Interrupt()
{
    if (m_active == nullptr) return;
    m_qdkgsman.ForEachHandler([](CDKGSessionHandler& base) {
        if (auto* handler = dynamic_cast<ActiveDKGSessionHandler*>(&base)) {
            handler->RequestStop();
        }
    });
}

void NetDKG::PhaseHandlerThread(ActiveDKGSessionHandler& handler)
{
    while (!handler.IsStopRequested()) {
        try {
            LogPrint(BCLog::LLMQ_DKG, "NetDKG::%s -- %s qi[%d] - starting HandleDKGRound\n", __func__,
                     handler.params.name, handler.QuorumIndex());
            HandleDKGRound(handler);
        } catch (AbortPhaseException& e) {
            m_active->dkgdbgman.MarkAborted(handler.params.type, handler.QuorumIndex());
            LogPrint(BCLog::LLMQ_DKG, "NetDKG::%s -- %s qi[%d] - aborted current DKG session\n", __func__,
                     handler.params.name, handler.QuorumIndex());
        }
    }
}

static void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman,
                                      const CMasternodeMetaMan& mn_metaman, const CSporkManager& sporkman,
                                      const UtilParameters& util_params, const CDeterministicMNList& tip_mn_list,
                                      const uint256& myProTxHash)
{
    assert(mn_metaman.IsValid());

    if (!IsQuorumPoseEnabled(llmqParams.type, sporkman)) {
        return;
    }

    auto members = utils::GetAllQuorumMembers(llmqParams.type, util_params);
    auto curTime = GetTime<std::chrono::seconds>().count();

    Uint256HashSet probeConnections;
    for (const auto& dmn : members) {
        if (dmn->proTxHash == myProTxHash) {
            continue;
        }
        auto lastOutbound = mn_metaman.GetLastOutboundSuccess(dmn->proTxHash);
        if (curTime - lastOutbound < 10 * 60) {
            // avoid re-probing nodes too often
            continue;
        }
        probeConnections.emplace(dmn->proTxHash);
    }

    if (!probeConnections.empty()) {
        if (LogAcceptDebug(BCLog::LLMQ)) {
            std::string debugMsg = strprintf("%s -- adding masternodes probes for quorum %s:\n", __func__,
                                             util_params.m_base_index->GetBlockHash().ToString());
            for (const auto& c : probeConnections) {
                auto dmn = tip_mn_list.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)\n", c.ToString(),
                                          dmn->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
                }
            }
            LogPrint(BCLog::NET_NETCONN, debugMsg.c_str()); /* Continued */
        }
        connman.AddPendingProbeConnections(probeConnections);
    }
}

void NetDKG::HandleDKGRound(ActiveDKGSessionHandler& handler)
{
    auto& active = *Assert(m_active);

    handler.WaitForNextPhase(std::nullopt, QuorumPhase::Initialized);

    // Leftovers missed their phase; discard raw bytes without BLS work
    handler.ClearPendingMessages();
    uint256 curQuorumHash = handler.GetCurrentQuorumHash();

    const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main,
                                                         return m_chainman.m_blockman.LookupBlockIndex(curQuorumHash));

    if (!pQuorumBaseBlockIndex || !handler.InitNewQuorum(pQuorumBaseBlockIndex)) {
        // should actually never happen
        handler.WaitForNewQuorum(curQuorumHash);
        throw AbortPhaseException();
    }

    active.dkgdbgman.MarkPhaseAdvanced(handler.params.type, handler.QuorumIndex(), QuorumPhase::Initialized);

    const auto curSession = handler.GetCurSession();
    if (handler.params.is_single_member()) {
        auto finalCommitment = curSession->FinalizeSingleCommitment();
        if (!finalCommitment.IsNull()) { // it can be null only if we are not member
            if (auto inv_opt = active.qblockman.AddMineableCommitment(finalCommitment); inv_opt.has_value()) {
                m_peer_manager->PeerRelayInv(inv_opt.value());
            }
        }
        active.dkgdbgman.MarkPhaseAdvanced(handler.params.type, handler.QuorumIndex(), QuorumPhase::Finalize);
        handler.WaitForNextPhase(QuorumPhase::Initialized, QuorumPhase::Contribute, curQuorumHash);
        return;
    }

    const auto tip_mn_list = active.dmnman.GetListAtChainTip();
    llmq::EnsureQuorumConnections(handler.params, active.connman, m_sporkman,
                                  {active.dmnman, active.qsnapman, m_chainman, pQuorumBaseBlockIndex}, tip_mn_list,
                                  curSession->ProTx(), /*is_masternode=*/true, handler.QuorumsWatch());
    if (curSession->AreWeMember()) {
        AddQuorumProbeConnections(handler.params, active.connman, active.mn_metaman, m_sporkman,
                                  {active.dmnman, active.qsnapman, m_chainman, pQuorumBaseBlockIndex}, tip_mn_list,
                                  curSession->ProTx());
    }

    handler.WaitForNextPhase(QuorumPhase::Initialized, QuorumPhase::Contribute, curQuorumHash);

    // Contribute
    auto fContributeStart = [curSession, &handler]() {
        if (auto qc = curSession->Contribute(); qc) {
            EnqueueOwn(handler.pendingContributions, curSession->ProTx(), *qc);
        }
    };
    auto fContributeWait = [this, curSession, &handler, &active] {
        return ProcessPendingMessageBatch<CDKGContribution>(active.connman, *curSession, handler.params,
                                                            handler.pendingContributions, *m_peer_manager, 8);
    };
    handler.HandlePhase(QuorumPhase::Contribute, QuorumPhase::Complain, curQuorumHash, 0.05, fContributeStart,
                        fContributeWait);

    // Complain
    auto fComplainStart = [curSession, &handler, &active]() {
        if (auto qc = curSession->VerifyAndComplain(active.connman); qc) {
            EnqueueOwn(handler.pendingComplaints, curSession->ProTx(), *qc);
        }
    };
    auto fComplainWait = [this, curSession, &handler, &active] {
        return ProcessPendingMessageBatch<CDKGComplaint>(active.connman, *curSession, handler.params,
                                                         handler.pendingComplaints, *m_peer_manager, 8);
    };
    handler.HandlePhase(QuorumPhase::Complain, QuorumPhase::Justify, curQuorumHash, 0.05, fComplainStart, fComplainWait);

    // Justify
    auto fJustifyStart = [curSession, &handler]() {
        if (auto qj = curSession->VerifyAndJustify(); qj) {
            EnqueueOwn(handler.pendingJustifications, curSession->ProTx(), *qj);
        }
    };
    auto fJustifyWait = [this, curSession, &handler, &active] {
        return ProcessPendingMessageBatch<CDKGJustification>(active.connman, *curSession, handler.params,
                                                             handler.pendingJustifications, *m_peer_manager, 8);
    };
    handler.HandlePhase(QuorumPhase::Justify, QuorumPhase::Commit, curQuorumHash, 0.05, fJustifyStart, fJustifyWait);

    // Commit
    auto fCommitStart = [curSession, &handler]() {
        if (auto qc = curSession->VerifyAndCommit(); qc) {
            EnqueueOwn(handler.pendingPrematureCommitments, curSession->ProTx(), *qc);
        }
    };
    auto fCommitWait = [this, curSession, &handler, &active] {
        return ProcessPendingMessageBatch<CDKGPrematureCommitment>(active.connman, *curSession, handler.params,
                                                                   handler.pendingPrematureCommitments, *m_peer_manager,
                                                                   8);
    };
    handler.HandlePhase(QuorumPhase::Commit, QuorumPhase::Finalize, curQuorumHash, 0.1, fCommitStart, fCommitWait);

    auto finalCommitments = curSession->FinalizeCommitments();
    for (const auto& fqc : finalCommitments) {
        if (auto inv_opt = active.qblockman.AddMineableCommitment(fqc); inv_opt.has_value()) {
            m_peer_manager->PeerRelayInv(inv_opt.value());
        }
    }
    active.dkgdbgman.MarkPhaseAdvanced(handler.params.type, handler.QuorumIndex(), QuorumPhase::Finalize);
}

void NetDKGStub::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type == NetMsgType::QCONTRIB || msg_type == NetMsgType::QCOMPLAINT || msg_type == NetMsgType::QJUSTIFICATION ||
        msg_type == NetMsgType::QPCOMMITMENT || msg_type == NetMsgType::QWATCH) {
        m_peer_manager->PeerMisbehaving(pfrom.GetId(), 10);
    }
}

} // namespace llmq
