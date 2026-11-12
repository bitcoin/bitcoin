// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/blockprocessor.h>

#include <evo/evodb.h>
#include <evo/specialtx.h>
#include <llmq/commitment.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <util/check.h>
#include <util/helpers.h>
#include <util/std23.h>

#include <chain.h>
#include <chainparams.h>
#include <checkqueue.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <saltedhasher.h>
#include <streams.h>
#include <sync.h>
#include <validation.h>

#include <algorithm>
#include <map>
#include <set>
#include <utility>

static void PreComputeQuorumMembers(CDeterministicMNManager& dmnman, llmq::CQuorumSnapshotManager& qsnapman,
                                    const ChainstateManager& chainman, const CBlockIndex* pindex, bool reset_cache)
{
    for (const Consensus::LLMQParams& params : llmq::GetEnabledQuorumParams(chainman, pindex->pprev)) {
        if (llmq::IsQuorumRotationEnabled(params, pindex) && (pindex->nHeight % params.dkgInterval == 0)) {
            llmq::utils::GetAllQuorumMembers(params.type, {dmnman, qsnapman, chainman, pindex}, reset_cache);
        }
    }
}

namespace llmq
{
static const std::string DB_MINED_COMMITMENT = "q_mc";
static const std::string DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT = "q_mcih";
static const std::string DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT_Q_INDEXED = "q_mcihi";

static const std::string DB_BEST_BLOCK_UPGRADE = "q_bbu2";

bool EraseMinedCommitmentIfUnreferenced(CEvoDB& evo_db, const Chainstate& chainstate,
                                        gsl::not_null<const CBlockIndex*> pindex,
                                        Consensus::LLMQType llmq_type, const uint256& quorum_hash)
{
    AssertLockHeld(::cs_main);
    const auto chainstates = chainstate.m_chainman.GetAll();
    const bool block_used_by_other_chainstate = std::any_of(
        chainstates.begin(), chainstates.end(),
        [&](const Chainstate* other) {
            return other != &chainstate && other->m_chain.Contains(pindex);
        });
    if (block_used_by_other_chainstate) {
        return false;
    }
    evo_db.Erase(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmq_type, quorum_hash)));
    return true;
}

template <typename T>
static bool SerializedEqual(const T& lhs, const T& rhs)
{
    CDataStream lhs_stream{SER_DISK, CLIENT_VERSION};
    CDataStream rhs_stream{SER_DISK, CLIENT_VERSION};
    lhs_stream << lhs;
    rhs_stream << rhs;
    return lhs_stream.size() == rhs_stream.size() &&
           std::equal(lhs_stream.begin(), lhs_stream.end(), rhs_stream.begin());
}

CQuorumBlockProcessor::CQuorumBlockProcessor(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CEvoDB& evoDb,
                                             CQuorumSnapshotManager& qsnapman, int8_t bls_threads) :
    m_chainman{chainman},
    m_dmnman{dmnman},
    m_evoDb{evoDb},
    m_qsnapman{qsnapman}
{
    mapMinedCommitmentBlockCache.Init(m_chainman.GetConsensus());
    LogPrintf("BLS verification uses %d additional threads\n", bls_threads);
    m_bls_queue.StartWorkerThreads(bls_threads);
}

CQuorumBlockProcessor::~CQuorumBlockProcessor()
{
    m_bls_queue.StopWorkerThreads();
}

MessageProcessingResult CQuorumBlockProcessor::ProcessMessage(const CNode& peer, std::string_view msg_type,
                                                              CDataStream& vRecv,
                                                              const ConsumeRequestFn& consume_request)
{
    if (msg_type != NetMsgType::QFCOMMITMENT) {
        return {};
    }

    CFinalCommitment qc;
    vRecv >> qc;

    // A QFCOMMITMENT is only ever sent in reply to a GETDATA (see ProcessGetData), so one we never
    // asked this peer for was pushed at us. Drop it up front: most of the checks below reject
    // without scoring the peer -- deliberately, since we may just be lagging behind -- so an
    // unsolicited peer could otherwise repeat the block lookups and map probes indefinitely. A bare
    // announcement deliberately does not qualify: it would let the peer authorise its own payload by
    // sending INV first.
    if (!consume_request(CInv{MSG_QUORUM_FINAL_COMMITMENT, ::SerializeHash(qc)})) {
        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- unrequested commitment from peer=%d\n", __func__,
                 peer.GetId());
        return MisbehavingError{UNREQUESTED_OBJECT_MISBEHAVIOR_SCORE, "unrequested quorum commitment"};
    }

    // Note: no m_to_erase, the request was already consumed by the solicitation check above.
    MessageProcessingResult ret;

    if (qc.IsNull()) {
        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, peer.GetId());
        ret.m_error = MisbehavingError{100};
        return ret;
    }

    const auto& llmq_params_opt = Params().GetLLMQ(qc.llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- invalid commitment type %d from peer=%d\n", __func__,
                 std23::to_underlying(qc.llmqType), peer.GetId());
        ret.m_error = MisbehavingError{100};
        return ret;
    }
    auto type = qc.llmqType;

    // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
    const CBlockIndex* pQuorumBaseBlockIndex;
    {
        LOCK(::cs_main);
        const auto& active_chainstate = m_chainman.ActiveChainstate();
        pQuorumBaseBlockIndex = active_chainstate.m_blockman.LookupBlockIndex(qc.quorumHash);
        if (pQuorumBaseBlockIndex == nullptr) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                     qc.quorumHash.ToString(), peer.GetId());
            // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
            // fully synced
            return ret;
        }
        if (active_chainstate.m_chain.Tip()->GetAncestor(pQuorumBaseBlockIndex->nHeight) != pQuorumBaseBlockIndex) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                     qc.quorumHash.ToString(), peer.GetId());
            // same, can't punish
            return ret;
        }
        if (int quorumHeight = pQuorumBaseBlockIndex->nHeight - (pQuorumBaseBlockIndex->nHeight % llmq_params_opt->dkgInterval) + int{qc.quorumIndex};
                quorumHeight != pQuorumBaseBlockIndex->nHeight) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
                     qc.quorumHash.ToString(), peer.GetId());
            ret.m_error = MisbehavingError{100};
            return ret;
        }
        if (pQuorumBaseBlockIndex->nHeight < (active_chainstate.m_chain.Height() - llmq_params_opt->dkgInterval)) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is too old, peer=%d\n", __func__,
                     qc.quorumHash.ToString(), peer.GetId());
            if (peer.GetCommonVersion() >= QFCOMMIT_STALE_REPROP_BAN_VERSION) {
                ret.m_error = MisbehavingError{100};
            }
            return ret;
        }
        if (HasMinedCommitment(type, qc.quorumHash)) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum hash[%s], type[%d], quorumIndex[%d] is already mined, peer=%d\n",
                     __func__, qc.quorumHash.ToString(), std23::to_underlying(type), qc.quorumIndex, peer.GetId());
            // NOTE: do not punish here
            return ret;
        }
    }

    {
        // Check if we already got a better one locally
        // We do this before verifying the commitment to avoid DoS
        LOCK(minableCommitmentsCs);
        auto k = std::make_pair(type, qc.quorumHash);
        auto it = minableCommitmentsByQuorum.find(k);
        if (it != minableCommitmentsByQuorum.end()) {
            auto jt = minableCommitments.find(it->second);
            if (jt != minableCommitments.end() && jt->second.CountSigners() <= qc.CountSigners()) {
                return ret;
            }
        }
    }

    if (!qc.Verify({m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex}, /*checkSigs=*/true)) {
        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum %s:%d is not valid quorumIndex[%d] nversion[%d], peer=%d\n",
                 __func__, qc.quorumHash.ToString(),
                 std23::to_underlying(qc.llmqType), qc.quorumIndex, qc.nVersion, peer.GetId());
        ret.m_error = MisbehavingError{100};
        return ret;
    }

    LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- received commitment for quorum %s:%d, validMembers=%d, signers=%d, peer=%d\n", __func__,
             qc.quorumHash.ToString(), std23::to_underlying(qc.llmqType), qc.CountValidMembers(), qc.CountSigners(), peer.GetId());

    if (auto inv_opt = AddMineableCommitment(qc)) {
        ret.m_inventory.emplace_back(inv_opt.value());
    }
    return ret;
}

bool CQuorumBlockProcessor::ProcessBlock(Chainstate& chainstate, const CBlock& block, gsl::not_null<const CBlockIndex*> pindex, BlockValidationState& state, bool fJustCheck, bool fBLSChecks)
{
    AssertLockHeld(::cs_main);

    const auto blockHash = pindex->GetBlockHash();

    if (!DeploymentActiveAt(*pindex, m_chainman.GetConsensus(), Consensus::DEPLOYMENT_DIP0003)) {
        m_evoDb.Write(DB_BEST_BLOCK_UPGRADE, blockHash);
        return true;
    }

    PreComputeQuorumMembers(m_dmnman, m_qsnapman, m_chainman, pindex, /*reset_cache=*/false);

    std::multimap<Consensus::LLMQType, CFinalCommitment> qcs;
    if (!GetCommitmentsFromBlock(block, pindex, qcs, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    // Note: must only check quorums that were enabled at the _previous_ block height to match mining logic
    for (const Consensus::LLMQParams& params : GetEnabledQuorumParams(m_chainman, pindex->pprev)) {
        // skip these checks when replaying blocks after the crash
        if (chainstate.m_chain.Tip() == nullptr) {
            break;
        }

        const size_t numCommitmentsRequired = GetNumCommitmentsRequired(params, chainstate.m_chain, pindex->nHeight);
        const auto numCommitmentsInNewBlock = qcs.count(params.type);

        if (numCommitmentsRequired < numCommitmentsInNewBlock) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-not-allowed");
        }

        if (numCommitmentsRequired > numCommitmentsInNewBlock) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-missing");
        }
        if (IsQuorumRotationEnabled(params, pindex)) {
            LogPrintf("[ProcessBlock] h[%d] numCommitmentsRequired[%d] numCommitmentsInNewBlock[%d]\n", pindex->nHeight, numCommitmentsRequired, numCommitmentsInNewBlock);
        }
    }

    // ProcessCommitment accepts a commitment identical to the stored one, so
    // rotation's multiple commitments per type need an explicit per-block check.
    // It runs before the signature checks, so a duplicate is rejected cheaply.
    // Null commitments stay exempt: they were never subject to bad-qc-dup.
    std::set<std::pair<Consensus::LLMQType, uint256>> seen_quorums;
    for (const auto& [_, qc] : qcs) {
        if (!qc.IsNull() && !seen_quorums.emplace(qc.llmqType, qc.quorumHash).second) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
        }
    }

    if (fBLSChecks) {
        CCheckQueueControl<utils::BlsCheck> queue_control(&m_bls_queue);
        for (const auto& [_, qc] : qcs) {
            if (qc.IsNull()) continue;
            const auto* pQuorumBaseBlockIndex = chainstate.m_blockman.LookupBlockIndex(qc.quorumHash);
            if (pQuorumBaseBlockIndex == nullptr) {
                LogPrint(BCLog::LLMQ, "[ProcessBlock] h[%d] unexpectedly failed due to no known pindex for hash[%s]\n",
                         pindex->nHeight, qc.quorumHash.ToString());
                return false;
            }
            qc.VerifySignatureAsync({m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex}, &queue_control);
        }

        if (!queue_control.Wait()) {
            // at least one check failed
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid");
        }
    }
    for (const auto& [_, qc] : qcs) {
        if (!ProcessCommitment(chainstate, pindex->nHeight, blockHash, qc, state, fJustCheck)) {
            LogPrintf("[ProcessBlock] failed h[%d] llmqType[%d] version[%d] quorumIndex[%d] quorumHash[%s]\n", pindex->nHeight, std23::to_underlying(qc.llmqType), qc.nVersion, qc.quorumIndex, qc.quorumHash.ToString());
            return false;
        }
    }

    m_evoDb.Write(DB_BEST_BLOCK_UPGRADE, blockHash);

    return true;
}

// We store a mapping from minedHeight->quorumHeight in the DB
// minedHeight is inversed so that entries are traversable in reversed order
static std::tuple<std::string, Consensus::LLMQType, uint32_t> BuildInversedHeightKey(Consensus::LLMQType llmqType, int nMinedHeight)
{
    // nMinedHeight must be converted to big endian to make it comparable when serialized
    return std::make_tuple(DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT, llmqType, htobe32_internal(std::numeric_limits<uint32_t>::max() - nMinedHeight));
}

static std::tuple<std::string, Consensus::LLMQType, int, uint32_t> BuildInversedHeightKeyIndexed(Consensus::LLMQType llmqType, int nMinedHeight, int quorumIndex)
{
    // nMinedHeight must be converted to big endian to make it comparable when serialized
    return std::make_tuple(DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT_Q_INDEXED, llmqType, quorumIndex, htobe32_internal(std::numeric_limits<uint32_t>::max() - nMinedHeight));
}

static bool IsMiningPhase(const Consensus::LLMQParams& llmqParams, const CChain& active_chain, int nHeight)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    // Note: This function can be called for new blocks
    assert(nHeight <= active_chain.Height() + 1);

    int quorumCycleStartHeight = nHeight - (nHeight % llmqParams.dkgInterval);
    int quorumCycleMiningStartHeight = quorumCycleStartHeight + llmqParams.dkgMiningWindowStart;
    int quorumCycleMiningEndHeight = quorumCycleStartHeight + llmqParams.dkgMiningWindowEnd;

    return nHeight >= quorumCycleMiningStartHeight && nHeight <= quorumCycleMiningEndHeight;
}

bool CQuorumBlockProcessor::ProcessCommitment(Chainstate& chainstate, int nHeight, const uint256& blockHash, const CFinalCommitment& qc,
                                              BlockValidationState& state, bool fJustCheck)
{
    AssertLockHeld(::cs_main);

    const auto& llmq_params_opt = Params().GetLLMQ(qc.llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "%s -- invalid commitment type %d\n", __func__, std23::to_underlying(qc.llmqType));
        return false;
    }
    const auto& llmq_params = llmq_params_opt.value();

    uint256 quorumHash = GetQuorumBlockHash(llmq_params, chainstate.m_chain, nHeight, qc.quorumIndex);

    LogPrint(BCLog::LLMQ, /* Continued */
             "%s -- processing commitment for block height=%d, type=%d, quorumIndex=%d, quorumHash=%s, signers=%s, "
             "validMembers=%d, quorumPublicKey=%s "
             "fJustCheck[%d] processing commitment from block.\n",
             __func__, nHeight, std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString(), qc.CountSigners(),
             qc.CountValidMembers(), qc.quorumPublicKey.ToString(), fJustCheck);

    // skip `bad-qc-block` checks below when replaying blocks after the crash
    if (chainstate.m_chain.Tip() == nullptr) {
        quorumHash = qc.quorumHash;
    }

    if (quorumHash.IsNull()) {
        LogPrint(BCLog::LLMQ, /* Continued */
                 "%s -- height=%d, type=%d, quorumIndex=%d, quorumHash=%s, signers=%s, validMembers=%d, "
                 "quorumPublicKey=%s quorumHash is null.\n",
                 __func__, nHeight, std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString(), qc.CountSigners(),
                 qc.CountValidMembers(), qc.quorumPublicKey.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block");
    }
    if (quorumHash != qc.quorumHash) {
        LogPrint(BCLog::LLMQ, /* Continued */
                 "%s -- height=%d, type=%d, quorumIndex=%d, quorumHash=%s, qc.quorumHash=%s signers=%s, "
                 "validMembers=%d, quorumPublicKey=%s non equal quorumHash.\n",
                 __func__, nHeight, std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString(),
                 qc.quorumHash.ToString(), qc.CountSigners(), qc.CountValidMembers(), qc.quorumPublicKey.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull()) {
            LogPrint(BCLog::LLMQ, /* Continued */
                     "%s -- height=%d, type=%d, quorumIndex=%d, quorumHash=%s, signers=%s, validMembers=%dqc "
                     "verifynull failed.\n",
                     __func__, nHeight, std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString(),
                     qc.CountSigners(), qc.CountValidMembers());
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid-null");
        }
        return true;
    }

    const auto stored_commitment = GetMinedCommitment(llmq_params.type, quorumHash);
    if (!stored_commitment.first.IsNull() &&
        !SerializedEqual(stored_commitment, std::make_pair(qc, blockHash))) {
        // Preserve the existing duplicate-commitment result while allowing an
        // exact block re-derivation to proceed through all validation below.
        // Note: a commitment retained by UndoBlock for another chainstate's
        // benefit would hit this path if this chain later re-mined the same
        // qc in a different block. That needs a disconnect of a block shared
        // with the other chainstate, which background validation (advancing
        // only toward the snapshot base along the snapshot chain) never does;
        // revisit if background reorgs ever become possible.
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
    }

    if (!IsMiningPhase(llmq_params, chainstate.m_chain, nHeight)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-height");
    }

    const auto* pQuorumBaseBlockIndex = chainstate.m_blockman.LookupBlockIndex(qc.quorumHash);
    if (pQuorumBaseBlockIndex == nullptr) {
        LogPrint(BCLog::LLMQ, "%s -- unexpectedly failed due to no known pindex for hash[%s]\n", __func__,
                 qc.quorumHash.ToString());
        return false;
    }

    // we don't validate signatures here; they already validated on previous step
    if (!qc.Verify({m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex}, /*checksigs=*/false)) {
        LogPrint(BCLog::LLMQ, /* Continued */
                 "%s -- height=%d, type=%d, quorumIndex=%d, quorumHash=%s, signers=%s, validMembers=%d, "
                 "quorumPublicKey=%s qc verify failed.\n",
                 __func__, nHeight, std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString(), qc.CountSigners(),
                 qc.CountValidMembers(), qc.quorumPublicKey.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid");
    }

    if (fJustCheck) {
        return true;
    }

    bool rotation_enabled = IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex);

    if (rotation_enabled) {
        LogPrint(BCLog::LLMQ, "%s -- height[%d] pQuorumBaseBlockIndex[%d] quorumIndex[%d] qversion[%d] Built\n",
                 __func__, nHeight, pQuorumBaseBlockIndex->nHeight, qc.quorumIndex, qc.nVersion);
    }

    // Store commitment in DB. A WriteDerived mismatch is local EvoDB
    // corruption, not a statement about the block: abort the node instead of
    // marking the block consensus-invalid and punishing the relaying peer.
    auto cacheKey = std::make_pair(llmq_params.type, quorumHash);
    if (!m_evoDb.WriteDerived(std::make_pair(DB_MINED_COMMITMENT, cacheKey), std::make_pair(qc, blockHash))) {
        return AbortNode(state, strprintf("CQuorumBlockProcessor::%s -- EvoDB quorum commitment mismatch for quorum %s",
                                          __func__, quorumHash.ToString()));
    }

    if (rotation_enabled) {
        m_evoDb.Write(BuildInversedHeightKeyIndexed(llmq_params.type, nHeight, int{qc.quorumIndex}), pQuorumBaseBlockIndex->nHeight);
    } else {
        m_evoDb.Write(BuildInversedHeightKey(llmq_params.type, nHeight), pQuorumBaseBlockIndex->nHeight);
    }

    // Only once this commitment's state change is complete, so the caches can never be
    // repopulated from a half-updated view. Callers all hold cs_main today, but the
    // invalidation should not depend on that.
    DropQcHashesCache();

    WITH_LOCK(minableCommitmentsCs, mapMinedCommitmentBlockCache.erase(qc.llmqType, qc.quorumHash));

    LogPrint(BCLog::LLMQ, "%s -- processed commitment from block. type=%d, quorumIndex=%d, quorumHash=%s\n", __func__,
             std23::to_underlying(qc.llmqType), qc.quorumIndex, quorumHash.ToString());

    return true;
}

void CQuorumBlockProcessor::DropQcHashesCache()
{
    LOCK(m_qc_hashes_cache_mutex);
    m_quorums_cached.clear();
    m_qc_hashes_cached.clear();
    m_qc_indexed_hashes_cached.clear();
    m_qc_hashes_lru.clear();
}

std::optional<std::pair<QcHashMap, QcIndexedHashMap>> CQuorumBlockProcessor::GetQcHashes(const CBlockIndex* pindexPrev) const
{
    auto quorums = GetMinedAndActiveCommitmentsUntilBlock(pindexPrev);

    LOCK(m_qc_hashes_cache_mutex);
    if (quorums == m_quorums_cached) {
        return std::make_pair(m_qc_hashes_cached, m_qc_indexed_hashes_cached);
    }

    // Quorums set is different, reset cached values
    m_quorums_cached.clear();
    m_qc_hashes_cached.clear();
    m_qc_indexed_hashes_cached.clear();
    if (!m_qc_hashes_lru.IsInitialized()) {
        m_qc_hashes_lru.Init(Params().GetConsensus());
    }

    for (const auto& [llmqType, vecBlockIndexes] : quorums) {
        const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
        assert(llmq_params_opt.has_value());
        bool rotation_enabled = IsQuorumRotationEnabled(llmq_params_opt.value(), pindexPrev);
        auto& vec_hashes = m_qc_hashes_cached[llmqType];
        vec_hashes.reserve(vecBlockIndexes.size());
        auto& map_indexed_hashes = m_qc_indexed_hashes_cached[llmqType];
        for (const auto& blockIndex : vecBlockIndexes) {
            uint256 block_hash{blockIndex->GetBlockHash()};

            std::pair<uint256, int> qc_hash;
            if (!m_qc_hashes_lru.get(llmqType, block_hash, qc_hash)) {
                auto [pqc, dummy_hash] = GetMinedCommitment(llmqType, block_hash);
                if (dummy_hash == uint256::ZERO) {
                    // this should never happen
                    return std::nullopt;
                }
                qc_hash.first = ::SerializeHash(pqc);
                qc_hash.second = rotation_enabled ? pqc.quorumIndex : 0;
                m_qc_hashes_lru.insert(llmqType, block_hash, qc_hash);
            }
            if (rotation_enabled) {
                map_indexed_hashes[qc_hash.second] = qc_hash.first;
            } else {
                vec_hashes.emplace_back(qc_hash.first);
            }
        }
    }
    std::swap(m_quorums_cached, quorums);
    return std::make_pair(m_qc_hashes_cached, m_qc_indexed_hashes_cached);
}

bool CQuorumBlockProcessor::UndoBlock(const Chainstate& chainstate, const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
{
    AssertLockHeld(::cs_main);

    PreComputeQuorumMembers(m_dmnman, m_qsnapman, m_chainman, pindex, /*reset_cache=*/true);

    std::multimap<Consensus::LLMQType, CFinalCommitment> qcs;
    if (BlockValidationState dummy; !GetCommitmentsFromBlock(block, pindex, qcs, dummy)) {
        return false;
    }

    for (auto& [_, qc2] : qcs) {
        auto& qc = qc2; // cannot capture structured binding into lambda
        if (qc.IsNull()) {
            continue;
        }

        if (!EraseMinedCommitmentIfUnreferenced(m_evoDb, chainstate, pindex, qc.llmqType, qc.quorumHash)) {
            LogPrint(BCLog::LLMQ, "%s -- retaining commitment for block %s used by another chainstate\n",
                     __func__, pindex->GetBlockHash().ToString());
        } else {
            const auto& llmq_params_opt = Params().GetLLMQ(qc.llmqType);
            assert(llmq_params_opt.has_value());

            if (IsQuorumRotationEnabled(llmq_params_opt.value(), pindex)) {
                m_evoDb.Erase(BuildInversedHeightKeyIndexed(qc.llmqType, pindex->nHeight, int{qc.quorumIndex}));
            } else {
                m_evoDb.Erase(BuildInversedHeightKey(qc.llmqType, pindex->nHeight));
            }
        }

        // Only once this commitment's state change is complete; see ProcessCommitment.
        DropQcHashesCache();

        WITH_LOCK(minableCommitmentsCs, mapMinedCommitmentBlockCache.erase(qc.llmqType, qc.quorumHash));

        // if a reorg happened, we should allow to mine this commitment later
        AddMineableCommitment(qc);
    }

    m_evoDb.Write(DB_BEST_BLOCK_UPGRADE, pindex->pprev->GetBlockHash());

    return true;
}

void CQuorumBlockProcessor::RemoveMineableCommitmentsForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
{
    AssertLockHeld(::cs_main);

    std::multimap<Consensus::LLMQType, CFinalCommitment> qcs;
    if (BlockValidationState dummy; !Assume(GetCommitmentsFromBlock(block, pindex, qcs, dummy))) {
        return;
    }

    LOCK(minableCommitmentsCs);
    for (const auto& [_, qc] : qcs) {
        if (qc.IsNull()) continue;
        minableCommitmentsByQuorum.erase(std::make_pair(qc.llmqType, qc.quorumHash));
        minableCommitments.erase(::SerializeHash(qc));
    }
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex, std::multimap<Consensus::LLMQType, CFinalCommitment>& ret, BlockValidationState& state)
{
    AssertLockHeld(::cs_main);

    const auto& consensus = Params().GetConsensus();

    ret.clear();

    for (const auto& tx : block.vtx) {
        if (tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            auto opt_qc = GetTxPayload<CFinalCommitmentTxPayload>(*tx);
            if (!opt_qc) {
                // should not happen as it was verified before processing the block
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s height=%d GetTxPayload fails\n", __func__, pindex->nHeight);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload");
            }
            auto& qc = *opt_qc;

            const auto& llmq_params_opt = Params().GetLLMQ(qc.commitment.llmqType);
            if (!llmq_params_opt.has_value()) {
                // should not happen as it was verified before processing the block
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-commitment-type");
            }

            // only allow one commitment per type and per block (This was changed with rotation)
            if (!IsQuorumRotationEnabled(llmq_params_opt.value(), pindex)) {
                if (ret.count(qc.commitment.llmqType) != 0) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
                }
            }

            ret.emplace(qc.commitment.llmqType, std::move(qc.commitment));
        }
    }

    if (!DeploymentActiveAt(*pindex, consensus, Consensus::DEPLOYMENT_DIP0003) && !ret.empty()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-premature");
    }

    return true;
}

size_t CQuorumBlockProcessor::GetNumCommitmentsRequired(const Consensus::LLMQParams& llmqParams, const CChain& chain, int nHeight) const
{
    AssertLockHeld(::cs_main);

    if (!IsMiningPhase(llmqParams, chain, nHeight)) return 0;

    // Note: This function can be called for new blocks
    assert(nHeight <= chain.Height() + 1);
    const auto* const pindex = chain.Height() < nHeight ? chain.Tip() : chain.Tip()->GetAncestor(nHeight);

    bool rotation_enabled = IsQuorumRotationEnabled(llmqParams, pindex);
    size_t quorums_num = rotation_enabled ? llmqParams.signingActiveQuorumCount : 1;
    size_t ret{0};

    for (const auto quorumIndex : util::irange(quorums_num)) {
        uint256 quorumHash = GetQuorumBlockHash(llmqParams, chain, nHeight, quorumIndex);
        if (!quorumHash.IsNull() && !HasMinedCommitment(llmqParams.type, quorumHash, chain)) ++ret;
    }

    return ret;
}

// WARNING: This method returns uint256() on the first block of the DKG interval (because the block hash is not known yet)
uint256 CQuorumBlockProcessor::GetQuorumBlockHash(const Consensus::LLMQParams& llmqParams, const CChain& active_chain, int nHeight, int quorumIndex)
{
    AssertLockHeld(::cs_main);

    int quorumStartHeight = nHeight - (nHeight % llmqParams.dkgInterval) + quorumIndex;

    const CBlockIndex* pindex = active_chain[quorumStartHeight];
    if (pindex == nullptr) {
        LogPrint(BCLog::LLMQ, "[GetQuorumBlockHash] llmqType[%d] h[%d] qi[%d] quorumStartHeight[%d] quorumHash[EMPTY]\n", std23::to_underlying(llmqParams.type), nHeight, quorumIndex, quorumStartHeight);
        return {};
    }

    return pindex->GetBlockHash();
}

bool CQuorumBlockProcessor::HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash) const
{
    LOCK(::cs_main);
    return HasMinedCommitment(llmqType, quorumHash, m_chainman.ActiveChain());
}

bool CQuorumBlockProcessor::HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash,
                                                const CChain& chain) const
{
    AssertLockHeld(::cs_main);

    uint256 mined_block_hash;
    bool cached;
    {
        LOCK(minableCommitmentsCs);
        cached = mapMinedCommitmentBlockCache.get(llmqType, quorumHash, mined_block_hash);
    }
    if (!cached) {
        mined_block_hash = GetMinedCommitment(llmqType, quorumHash).second;
        // Do not negatively cache. Snapshot activation seeds EvoDB directly,
        // outside ProcessCommitment's normal cache-invalidation path.
        if (!mined_block_hash.IsNull()) {
            LOCK(minableCommitmentsCs);
            mapMinedCommitmentBlockCache.insert(llmqType, quorumHash, mined_block_hash);
        }
    }

    const CBlockIndex* mined_block = m_chainman.m_blockman.LookupBlockIndex(mined_block_hash);
    return mined_block != nullptr && chain.Contains(mined_block);
}

std::pair<CFinalCommitment, uint256> CQuorumBlockProcessor::GetMinedCommitment(Consensus::LLMQType llmqType,
                                                                               const uint256& quorumHash) const
{
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmqType, quorumHash));
    std::pair<CFinalCommitment, uint256> ret;
    if (!m_evoDb.Read(key, ret)) {
        return {CFinalCommitment{}, uint256::ZERO};
    }
    return ret;
}

// The returned quorums are in reversed order, so the most recent one is at index 0
std::vector<const CBlockIndex*> CQuorumBlockProcessor::GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex, size_t maxCount) const
{
    AssertLockNotHeld(m_evoDb.cs);
    LOCK(m_evoDb.cs);

    auto dbIt = m_evoDb.GetCurTransaction().NewIteratorUniquePtr();

    auto firstKey = BuildInversedHeightKey(llmqType, pindex->nHeight);
    auto lastKey = BuildInversedHeightKey(llmqType, 0);

    dbIt->Seek(firstKey);

    std::vector<const CBlockIndex*> ret;
    ret.reserve(maxCount);

    while (dbIt->Valid() && ret.size() < maxCount) {
        decltype(firstKey) curKey;
        int quorumHeight;
        if (!dbIt->GetKey(curKey) || curKey >= lastKey) {
            break;
        }
        if (std::get<0>(curKey) != DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT || std::get<1>(curKey) != llmqType) {
            break;
        }

        if (uint32_t nMinedHeight = std::numeric_limits<uint32_t>::max() - be32toh_internal(std::get<2>(curKey));
                nMinedHeight > static_cast<uint32_t>(pindex->nHeight)) {
            break;
        }

        if (!dbIt->GetValue(quorumHeight)) {
            break;
        }

        const auto* pQuorumBaseBlockIndex = pindex->GetAncestor(quorumHeight);
        assert(pQuorumBaseBlockIndex);
        ret.emplace_back(pQuorumBaseBlockIndex);

        dbIt->Next();
    }

    return ret;
}

std::optional<const CBlockIndex*> CQuorumBlockProcessor::GetLastMinedCommitmentsByQuorumIndexUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, int quorumIndex, size_t cycle) const
{
    AssertLockNotHeld(m_evoDb.cs);
    LOCK(m_evoDb.cs);

    auto dbIt = m_evoDb.GetCurTransaction().NewIteratorUniquePtr();

    auto firstKey = BuildInversedHeightKeyIndexed(llmqType, pindex->nHeight, quorumIndex);
    auto lastKey = BuildInversedHeightKeyIndexed(llmqType, 0, quorumIndex);

    size_t currentCycle = 0;

    dbIt->Seek(firstKey);

    while (dbIt->Valid()) {
        decltype(firstKey) curKey;
        int quorumHeight;
        if (!dbIt->GetKey(curKey) || curKey >= lastKey) {
            return std::nullopt;
        }
        if (std::get<0>(curKey) != DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT_Q_INDEXED || std::get<1>(curKey) != llmqType) {
            return std::nullopt;
        }

        if (uint32_t nMinedHeight = std::numeric_limits<uint32_t>::max() - be32toh_internal(std::get<3>(curKey));
                nMinedHeight > static_cast<uint32_t>(pindex->nHeight)) {
            return std::nullopt;
        }

        if (!dbIt->GetValue(quorumHeight)) {
            return std::nullopt;
        }

        const auto* pQuorumBaseBlockIndex = pindex->GetAncestor(quorumHeight);
        assert(pQuorumBaseBlockIndex);

        if (currentCycle == cycle) {
            return std::make_optional(pQuorumBaseBlockIndex);
        }

        currentCycle++;

        dbIt->Next();
    }

    return std::nullopt;
}

std::vector<const CBlockIndex*> CQuorumBlockProcessor::GetLastMinedCommitmentsPerQuorumIndexUntilBlock(
    Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t cycle) const
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    std::vector<const CBlockIndex*> ret;

    for (const auto quorumIndex : util::irange(llmq_params_opt->signingActiveQuorumCount)) {
        std::optional<const CBlockIndex*> q = GetLastMinedCommitmentsByQuorumIndexUntilBlock(llmqType, pindex, quorumIndex, cycle);
        if (q.has_value()) {
            ret.emplace_back(q.value());
        }
    }

    return ret;
}

std::vector<const CBlockIndex*> CQuorumBlockProcessor::GetMinedCommitmentsIndexedUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount) const
{
    std::vector<const CBlockIndex*> ret;

    size_t cycle = 0;

    while (ret.size() < maxCount) {
        std::vector<const CBlockIndex*> cycleRet = GetLastMinedCommitmentsPerQuorumIndexUntilBlock(llmqType, pindex, cycle);

        if (cycleRet.empty()) {
            return ret;
        }

        size_t needToCopy = maxCount - ret.size();
        std::copy_n(cycleRet.begin(), std::min(needToCopy, cycleRet.size()), std::back_inserter(ret));
        cycle++;
    }

    return ret;
}

// The returned quorums are in reversed order, so the most recent one is at index 0
std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> CQuorumBlockProcessor::GetMinedAndActiveCommitmentsUntilBlock(gsl::not_null<const CBlockIndex*> pindex) const
{
    std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> ret;

    for (const auto& params : Params().GetConsensus().llmqs) {
        auto& commitments = ret[params.type];
        if (IsQuorumRotationEnabled(params, pindex)) {
            commitments = GetLastMinedCommitmentsPerQuorumIndexUntilBlock(params.type, pindex, 0);
        } else {
            commitments = GetMinedCommitmentsUntilBlock(params.type, pindex, params.signingActiveQuorumCount);
        }
    }

    return ret;
}

bool CQuorumBlockProcessor::HasMineableCommitment(const uint256& hash) const
{
    LOCK(minableCommitmentsCs);
    return minableCommitments.count(hash) != 0;
}

std::optional<CInv> CQuorumBlockProcessor::AddMineableCommitment(const CFinalCommitment& fqc)
{
    const uint256 commitmentHash = ::SerializeHash(fqc);

    const bool relay = [&]() {
        LOCK(minableCommitmentsCs);

        auto k = std::make_pair(fqc.llmqType, fqc.quorumHash);
        auto [itInserted, successfullyInserted] = minableCommitmentsByQuorum.try_emplace(k, commitmentHash);
        if (successfullyInserted) {
            minableCommitments.try_emplace(commitmentHash, fqc);
            return true;
        } else {
            auto& insertedQuorumHash = itInserted->second;
            const auto& oldFqc = minableCommitments.at(insertedQuorumHash);
            if (fqc.CountSigners() > oldFqc.CountSigners()) {
                // new commitment has more signers, so override the known one
                insertedQuorumHash = commitmentHash;
                minableCommitments.erase(insertedQuorumHash);
                minableCommitments.try_emplace(commitmentHash, fqc);
                return true;
            }
        }
        return false;
    }();

    return relay ? std::make_optional(CInv{MSG_QUORUM_FINAL_COMMITMENT, commitmentHash}) : std::nullopt;
}

bool CQuorumBlockProcessor::GetMineableCommitmentByHash(const uint256& commitmentHash, llmq::CFinalCommitment& ret) const
{
    LOCK(minableCommitmentsCs);
    auto it = minableCommitments.find(commitmentHash);
    if (it == minableCommitments.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

// Will return nullopt if no commitment should be mined
// Will return a null commitment if no mineable commitment is known and none was mined yet
std::optional<std::vector<CFinalCommitment>> CQuorumBlockProcessor::GetMineableCommitments(const Consensus::LLMQParams& llmqParams, int nHeight) const
{
    AssertLockHeld(::cs_main);

    std::vector<CFinalCommitment> ret;
    const auto& active_chain = m_chainman.ActiveChain();

    if (GetNumCommitmentsRequired(llmqParams, active_chain, nHeight) == 0) {
        // no commitment required
        return std::nullopt;
    }

    // Note: This function can be called for new blocks
    assert(nHeight <= active_chain.Height() + 1);
    const auto* const pindex = active_chain.Height() < nHeight ? active_chain.Tip() : active_chain.Tip()->GetAncestor(nHeight);

    bool rotation_enabled = IsQuorumRotationEnabled(llmqParams, pindex);
    bool basic_bls_enabled{DeploymentActiveAfter(pindex, m_chainman.GetConsensus(), Consensus::DEPLOYMENT_V19)};
    size_t quorums_num = rotation_enabled ? llmqParams.signingActiveQuorumCount : 1;

    std::stringstream ss;
    for (const auto quorumIndex : util::irange(quorums_num)) {
        CFinalCommitment cf;

        uint256 quorumHash = GetQuorumBlockHash(llmqParams, active_chain, nHeight, quorumIndex);
        if (quorumHash.IsNull()) {
            break;
        }

        if (HasMinedCommitment(llmqParams.type, quorumHash)) continue;

        LOCK(minableCommitmentsCs);

        auto k = std::make_pair(llmqParams.type, quorumHash);
        if (auto it = minableCommitmentsByQuorum.find(k); it == minableCommitmentsByQuorum.end()) {
            // null commitment required
            cf = CFinalCommitment(llmqParams, quorumHash);
            cf.quorumIndex = static_cast<int16_t>(quorumIndex);
            cf.nVersion = CFinalCommitment::GetVersion(rotation_enabled, basic_bls_enabled);
            ss << "{ created nversion[" << cf.nVersion << "] quorumIndex[" << cf.quorumIndex << "] }";
        } else {
            cf = minableCommitments.at(it->second);
            ss << "{ cached nversion[" << cf.nVersion << "] quorumIndex[" << cf.quorumIndex << "] }";
        }

        ret.push_back(std::move(cf));
    }

    LogPrint(BCLog::LLMQ, "GetMineableCommitments cf height[%d] content: %s\n", nHeight, ss.str());

    if (ret.empty()) {
        return std::nullopt;
    }
    return std::make_optional(ret);
}

bool CQuorumBlockProcessor::GetMineableCommitmentsTx(const Consensus::LLMQParams& llmqParams, int nHeight, std::vector<CTransactionRef>& ret) const
{
    AssertLockHeld(::cs_main);
    std::optional<std::vector<CFinalCommitment>> qcs = GetMineableCommitments(llmqParams, nHeight);
    if (!qcs.has_value()) {
        return false;
    }

    for (const auto& f : qcs.value()) {
        CFinalCommitmentTxPayload qc;
        qc.nHeight = nHeight;
        qc.commitment = f;
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_QUORUM_COMMITMENT;
        SetTxPayload(tx, qc);
        ret.push_back(MakeTransactionRef(tx));
    }

    return true;
}

} // namespace llmq
