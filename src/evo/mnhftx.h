// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_MNHFTX_H
#define BITCOIN_EVO_MNHFTX_H

#include <saltedhasher.h>
#include <sync.h>
#include <threadsafety.h>
#include <versionbits.h>

#include <bls/bls.h>
#include <unordered_lru_cache.h>

#include <gsl/pointers.h>
#include <univalue.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CChain;
class CEvoDB;
class CTransaction;
class TxValidationState;
struct RPCResult;
namespace llmq {
class CQuorumManager;
}
namespace node {
class BlockManager;
} // namespace node

// mnhf signal special transaction
class MNHFTx
{
public:
    uint8_t versionBit{0};
    uint256 quorumHash{0};
    CBLSSignature sig{};

    MNHFTx() = default;

    SERIALIZE_METHODS(MNHFTx, obj)
    {
        READWRITE(obj.versionBit, obj.quorumHash);
        READWRITE(CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), /*legacy=*/false));
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;
};

class MNHFTxPayload
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_MNHF_SIGNAL;
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint8_t nVersion{CURRENT_VERSION};
    MNHFTx signal;

public:
    /**
     * helper function to calculate Request ID used for signing
     */
    uint256 GetRequestId() const;

    /**
     * helper function to prepare special transaction for signing
     */
    CMutableTransaction PrepareTx() const;

    /**
     * Checks that don't depend on chain state or the BLS signature (payload version
     * and signal version bit bounds).
     */
    bool IsTriviallyValid(TxValidationState& state) const;

    SERIALIZE_METHODS(MNHFTxPayload, obj)
    {
        READWRITE(obj.nVersion, obj.signal);
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;
};

class CMNHFManager : public AbstractEHFManager
{
private:
    CEvoDB& m_evoDb;
    // TODO: store in database active EHF signals not for each block;
    // but quite opposite: keep only hash of block where signal is added.
    // TODO: implement migration to a new format
    const Consensus::Params& m_consensus_params;

    static constexpr size_t MNHFCacheSize = 1000;
    Mutex cs_cache;
    // versionBit <-> height
    Uint256LruHashMap<Signals> mnhfCache GUARDED_BY(cs_cache){MNHFCacheSize};

public:
    CMNHFManager() = delete;
    CMNHFManager(const CMNHFManager&) = delete;
    CMNHFManager& operator=(const CMNHFManager&) = delete;
    explicit CMNHFManager(CEvoDB& evoDb, const Consensus::Params& consensus_params);
    ~CMNHFManager() override;

    /**
     * Every new block should be processed when Tip() is updated by calling of CMNHFManager::ProcessBlock.
     * This function actually does only validate EHF transaction for this block and update internal caches/evodb state
     */
    std::optional<Signals> ProcessBlock(const CBlock& block, const CBlockIndex* const pindex, bool fJustCheck,
                                        BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * Every undo block should be processed when Tip() is updated by calling of CMNHFManager::UndoBlock
     * This function actually does nothing at the moment, because status of ancestor block is already known.
     * Although it should be still called to do some sanity checks
     */
    bool UndoBlock(const CBlock& block, const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    // Implements interface. Only reads state computed when a block was connected, and throws if
    // pindexPrev has none (e.g. it was never connected). A block whose connection failed after its
    // special transactions were processed can still be in the cache, so a caller that takes a block
    // from outside must check that it is valid first.
    Signals GetSignalsStage(const CBlockIndex* const pindexPrev) override EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * Helper that used in Unit Test to forcely setup EHF signal for specific block
     */
    void AddSignal(const CBlockIndex* const pindex, int bit) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    bool ForceSignalDBUpdate(const CBlockIndex* tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_cache);

    /**
     * Recompute missing state from block data, walking back from pindex to the first block whose
     * state is stored. Throws for a block that did not pass script validation. Only for startup,
     * inside an EvoDB transaction: pindex must be a block that was connected.
     */
    void RebuildSignals(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_cache);

private:
    void AddToCache(const Signals& signals, const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * Returns the signals stored for pindex, from the cache or evo db. Throws EvoDbMissingStateError
     * if there are none, e.g. because the block was never connected.
     * NOTE: that some signals could expired between blocks.
     */
    Signals GetForBlock(const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * This function access to in-memory cache or to evo db but does not calculate anything
     * NOTE: that some signals could expired between blocks.
     */
    std::optional<Signals> GetFromCache(const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
};

std::optional<uint8_t> extractEHFSignal(const CTransaction& tx);
bool CheckMNHFTx(const node::BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);
bool CheckMNHFTx(const node::BlockManager& blockman, const llmq::CQuorumManager& qman, const CChain& chain,
                 const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif // BITCOIN_EVO_MNHFTX_H
