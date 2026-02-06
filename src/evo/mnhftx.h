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
class CEvoDB;
class CTransaction;
class ChainstateManager;
class TxValidationState;
struct RPCResult;
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
    // TODO: move its functionallity of ProcessBlock, UndoBlock to specialtxman;
    // it will help to drop dependency on m_chainman, m_qman here (and validation.h)
    // Secondly, store in database active EHF signals not for each block;
    // but quite opposite: keep only hash of block where signal is added.
    // TODO: implement migration to a new format
    const ChainstateManager& m_chainman;
    const llmq::CQuorumManager& m_qman;

    static constexpr size_t MNHFCacheSize = 1000;
    Mutex cs_cache;
    // versionBit <-> height
    Uint256LruHashMap<Signals> mnhfCache GUARDED_BY(cs_cache){MNHFCacheSize};

public:
    CMNHFManager() = delete;
    CMNHFManager(const CMNHFManager&) = delete;
    CMNHFManager& operator=(const CMNHFManager&) = delete;
    explicit CMNHFManager(CEvoDB& evoDb, const ChainstateManager& chainman, const llmq::CQuorumManager& qman);
    ~CMNHFManager();

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

    // Implements interface
    Signals GetSignalsStage(const CBlockIndex* const pindexPrev) override EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * Helper that used in Unit Test to forcely setup EHF signal for specific block
     */
    void AddSignal(const CBlockIndex* const pindex, int bit) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    bool ForceSignalDBUpdate() EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_cache);

private:
    void AddToCache(const Signals& signals, const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    /**
     * This function returns list of signals available on previous block.
     * if the signals for previous block is not available in cache it would read blocks from disk
     * until state won't be recovered.
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
bool CheckMNHFTx(const ChainstateManager& chainman, const llmq::CQuorumManager& qman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);

#endif // BITCOIN_EVO_MNHFTX_H
