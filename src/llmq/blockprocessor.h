// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_BLOCKPROCESSOR_H
#define BITCOIN_LLMQ_BLOCKPROCESSOR_H

#include <bls/bls.h>
#include <checkqueue.h>
#include <kernel/cs_main.h> // IWYU pragma: export
#include <llmq/cache.h>
#include <llmq/params.h>
#include <llmq/utils.h>
#include <msg_result.h>
#include <protocol.h>
#include <saltedhasher.h>
#include <sync.h>

#include <gsl/pointers.h>

#include <functional>
#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CBLSSignature;
class CChain;
class Chainstate;
class ChainstateManager;
class CDataStream;
class CDeterministicMNManager;
class CEvoDB;
class CNode;

namespace llmq
{
class CFinalCommitment;
class CQuorumSnapshotManager;

//! Serialized hashes of the commitments mined for the active quorums, by LLMQ type.
using QcHashMap = std::map<Consensus::LLMQType, std::vector<uint256>>;
//! As above, but keyed by quorumIndex, for rotation-enabled types.
using QcIndexedHashMap = std::map<Consensus::LLMQType, std::map<int16_t, uint256>>;

/** Erase a mined commitment unless another chainstate still contains its block. */
bool EraseMinedCommitmentIfUnreferenced(CEvoDB& evo_db, const Chainstate& chainstate,
                                        gsl::not_null<const CBlockIndex*> pindex,
                                        Consensus::LLMQType llmq_type, const uint256& quorum_hash)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

class CQuorumBlockProcessor
{
private:
    ChainstateManager& m_chainman;
    CDeterministicMNManager& m_dmnman;
    CEvoDB& m_evoDb;
    CQuorumSnapshotManager& m_qsnapman;

    CCheckQueue<utils::BlsCheck> m_bls_queue{4};

    mutable Mutex minableCommitmentsCs;
    std::map<std::pair<Consensus::LLMQType, uint256>, uint256> minableCommitmentsByQuorum GUARDED_BY(minableCommitmentsCs);
    std::map<uint256, CFinalCommitment> minableCommitments GUARDED_BY(minableCommitmentsCs);

    // Cache the block in which a commitment was mined. Membership in a
    // particular chain is checked on every call so reorgs need no cache flush.
    mutable PerLlmqTypeCache<uint256> mapMinedCommitmentBlockCache GUARDED_BY(minableCommitmentsCs);

    // Memoizes GetQcHashes(). The whole-result cache is keyed on the set of active
    // quorum base blocks, the LRU on those base-block hashes; neither key identifies
    // which CFinalCommitment was mined for a base, so both are dropped whenever mined
    // commitment state changes (see DropQcHashesCache). Owning them here keeps that
    // invalidation next to the writes it has to follow, and ties their lifetime to the
    // block index whose CBlockIndex* the outer cache stores.
    mutable Mutex m_qc_hashes_cache_mutex;
    mutable std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> m_quorums_cached GUARDED_BY(m_qc_hashes_cache_mutex);
    mutable PerLlmqTypeCache<std::pair<uint256, int>> m_qc_hashes_lru GUARDED_BY(m_qc_hashes_cache_mutex);
    mutable QcHashMap m_qc_hashes_cached GUARDED_BY(m_qc_hashes_cache_mutex);
    mutable QcIndexedHashMap m_qc_indexed_hashes_cached GUARDED_BY(m_qc_hashes_cache_mutex);

public:
    CQuorumBlockProcessor() = delete;
    CQuorumBlockProcessor(const CQuorumBlockProcessor&) = delete;
    CQuorumBlockProcessor& operator=(const CQuorumBlockProcessor&) = delete;
    explicit CQuorumBlockProcessor(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CEvoDB& evoDb,
                                   CQuorumSnapshotManager& qsnapman, int8_t bls_threads);
    ~CQuorumBlockProcessor();

    //! Predicate answering "do we have any record of asking this peer for the inv?", consuming that
    //! record as a side effect. An answer that is merely late or was superseded still returns true;
    //! false means we have nothing to show we asked, which is either because we did not or because
    //! the answer came too long after we did (see GetDataResponse). Passed in rather than reached
    //! through PeerManagerInternal because net_processing already depends on this header; see
    //! ProcessMessage for how it is used.
    //!
    //! Must be invoked without ::cs_main held -- the implementation takes it. Thread-safety
    //! analysis cannot check this through the type-erased std::function, so keep any call site
    //! outside ProcessMessage's own LOCK(::cs_main) block.
    using ConsumeRequestFn = std::function<bool(const CInv&)>;

    [[nodiscard]] MessageProcessingResult ProcessMessage(const CNode& peer, std::string_view msg_type,
                                                         CDataStream& vRecv,
                                                         const ConsumeRequestFn& consume_request)
        EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);

    bool ProcessBlock(Chainstate& chainstate, const CBlock& block, gsl::not_null<const CBlockIndex*> pindex, BlockValidationState& state,
                      bool fJustCheck, bool fBLSChecks) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs, !m_qc_hashes_cache_mutex);
    bool UndoBlock(const Chainstate& chainstate, const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs, !m_qc_hashes_cache_mutex);
    //! Drop the commitments mined by a block that has been fully connected from the mineable pool.
    void RemoveMineableCommitmentsForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);

    //! it returns hash of commitment if it should be relay, otherwise nullopt
    std::optional<CInv> AddMineableCommitment(const CFinalCommitment& fqc) EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    bool HasMineableCommitment(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    bool GetMineableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret) const
        EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    std::optional<std::vector<CFinalCommitment>> GetMineableCommitments(const Consensus::LLMQParams& llmqParams,
                                                                        int nHeight) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
    bool GetMineableCommitmentsTx(const Consensus::LLMQParams& llmqParams, int nHeight, std::vector<CTransactionRef>& ret) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
    bool HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    bool HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, const CChain& chain) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
    std::pair<CFinalCommitment, uint256> GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash) const;

    /**
     * Serialized hashes of the commitments mined for the quorums active as of pindexPrev.
     *
     * Memoized; returns nullopt if a commitment recorded as mined could not be read back,
     * which should never happen.
     */
    std::optional<std::pair<QcHashMap, QcIndexedHashMap>> GetQcHashes(const CBlockIndex* pindexPrev) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_qc_hashes_cache_mutex);

    std::vector<const CBlockIndex*> GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex, size_t maxCount) const;
    std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> GetMinedAndActiveCommitmentsUntilBlock(gsl::not_null<const CBlockIndex*> pindex) const;

    std::vector<const CBlockIndex*> GetMinedCommitmentsIndexedUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount) const;
    std::vector<const CBlockIndex*> GetLastMinedCommitmentsPerQuorumIndexUntilBlock(Consensus::LLMQType llmqType,
                                                                                    const CBlockIndex* pindex,
                                                                                    size_t cycle) const;
    std::optional<const CBlockIndex*> GetLastMinedCommitmentsByQuorumIndexUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, int quorumIndex, size_t cycle) const;
private:
    //! Called from every site that writes or erases mined commitment state.
    void DropQcHashesCache() EXCLUSIVE_LOCKS_REQUIRED(!m_qc_hashes_cache_mutex);

    static bool GetCommitmentsFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex, std::multimap<Consensus::LLMQType, CFinalCommitment>& ret, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessCommitment(Chainstate& chainstate, int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state,
                           bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs, !m_qc_hashes_cache_mutex);
public:
    // Public for multi-chainstate accounting tests and callers which validate
    // against a chainstate other than the active one.
    size_t GetNumCommitmentsRequired(const Consensus::LLMQParams& llmqParams, const CChain& chain, int nHeight) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
private:
    static uint256 GetQuorumBlockHash(const Consensus::LLMQParams& llmqParams, const CChain& active_chain, int nHeight, int quorumIndex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_BLOCKPROCESSOR_H
