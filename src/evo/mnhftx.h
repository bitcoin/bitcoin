// Copyright (c) 2021-2024 The Dash Core developers
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

#include <atomic>
#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CEvoDB;
class CTransaction;
class ChainstateManager;
class TxValidationState;
namespace llmq {
class CQuorumManager;
}

// mnhf signal special transaction
class MNHFTx
{
public:
    uint8_t versionBit{0};
    uint256 quorumHash{0};
    CBLSSignature sig{};

    MNHFTx() = default;
    bool Verify(const llmq::CQuorumManager& qman, const uint256& quorumHash, const uint256& requestId, const uint256& msgHash,
                TxValidationState& state) const;

    SERIALIZE_METHODS(MNHFTx, obj)
    {
        READWRITE(obj.versionBit, obj.quorumHash);
        READWRITE(CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), /* fLegacy= */ false));
    }

    std::string ToString() const;

    [[nodiscard]] UniValue ToJson() const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("versionBit", versionBit);
        obj.pushKV("quorumHash", quorumHash.ToString());
        obj.pushKV("sig", sig.ToString());
        return obj;
    }
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

    SERIALIZE_METHODS(MNHFTxPayload, obj)
    {
        READWRITE(obj.nVersion, obj.signal);
    }

    std::string ToString() const;

    [[nodiscard]] UniValue ToJson() const;
};

class CMNHFManager : public AbstractEHFManager
{
private:
    CEvoDB& m_evoDb;
    std::atomic<ChainstateManager*> m_chainman{nullptr};
    std::atomic<llmq::CQuorumManager*> m_qman{nullptr};

    static constexpr size_t MNHFCacheSize = 1000;
    Mutex cs_cache;
    // versionBit <-> height
    Uint256LruHashMap<Signals> mnhfCache GUARDED_BY(cs_cache){MNHFCacheSize};

public:
    explicit CMNHFManager(CEvoDB& evoDb);
    ~CMNHFManager();
    explicit CMNHFManager(const CMNHFManager&) = delete;

    /**
     * Every new block should be processed when Tip() is updated by calling of CMNHFManager::ProcessBlock.
     * This function actually does only validate EHF transaction for this block and update internal caches/evodb state
     *
     * @pre Caller must ensure that LLMQContext has been initialized and the llmq::CQuorumManager pointer has been
     *      set by calling ConnectManagers() for this CMNHFManager instance
     */
    std::optional<Signals> ProcessBlock(const CBlock& block, const CBlockIndex* const pindex, bool fJustCheck, BlockValidationState& state);

    /**
     * Every undo block should be processed when Tip() is updated by calling of CMNHFManager::UndoBlock
     * This function actually does nothing at the moment, because status of ancestor block is already known.
     * Although it should be still called to do some sanity checks
     *
     * @pre Caller must ensure that LLMQContext has been initialized and the llmq::CQuorumManager pointer has been
     *      set by calling ConnectManagers() for this CMNHFManager instance
     */
    bool UndoBlock(const CBlock& block, const CBlockIndex* const pindex);

    // Implements interface
    Signals GetSignalsStage(const CBlockIndex* const pindexPrev) override;

    /**
     * Helper that used in Unit Test to forcely setup EHF signal for specific block
     */
    void AddSignal(const CBlockIndex* const pindex, int bit) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * Set llmq::CQuorumManager pointer.
     *
     * Separated from constructor to allow LLMQContext to use CMNHFManager in read-only capacity.
     * Required to mutate state.
     */
    void ConnectManagers(gsl::not_null<ChainstateManager*> chainman, gsl::not_null<llmq::CQuorumManager*> qman);

    /**
     * Reset llmq::CQuorumManager pointer.
     *
     * @pre Must be called before LLMQContext (containing llmq::CQuorumManager) is destroyed.
     */
    void DisconnectManagers();

    bool ForceSignalDBUpdate() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

private:
    void AddToCache(const Signals& signals, const CBlockIndex* const pindex);

    /**
     * This function returns list of signals available on previous block.
     * if the signals for previous block is not available in cache it would read blocks from disk
     * until state won't be recovered.
     * NOTE: that some signals could expired between blocks.
     */
    Signals GetForBlock(const CBlockIndex* const pindex);

    /**
     * This function access to in-memory cache or to evo db but does not calculate anything
     * NOTE: that some signals could expired between blocks.
     */
    std::optional<Signals> GetFromCache(const CBlockIndex* const pindex);
};

std::optional<uint8_t> extractEHFSignal(const CTransaction& tx);
bool CheckMNHFTx(const ChainstateManager& chainman, const llmq::CQuorumManager& qman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);

#endif // BITCOIN_EVO_MNHFTX_H
