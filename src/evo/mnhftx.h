// Copyright (c) 2021-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_MNHFTX_H
#define BITCOIN_EVO_MNHFTX_H

#include <bls/bls.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <univalue.h>

#include <optional>
#include <saltedhasher.h>
#include <unordered_map>
#include <unordered_lru_cache.h>
#include <versionbits.h>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CEvoDB;
class TxValidationState;

// mnhf signal special transaction
class MNHFTx
{
public:
    uint8_t versionBit{0};
    uint256 quorumHash{0};
    CBLSSignature sig{};

    MNHFTx() = default;
    bool Verify(const uint256& quorumHash, const uint256& requestId, const uint256& msgHash, TxValidationState& state) const;

    SERIALIZE_METHODS(MNHFTx, obj)
    {
        READWRITE(obj.versionBit, obj.quorumHash);
        READWRITE(CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), /* fLegacy= */ false));
    }

    std::string ToString() const;

    [[nodiscard]] UniValue ToJson() const
    {
        UniValue obj;
        obj.clear();
        obj.setObject();
        obj.pushKV("versionBit", (int)versionBit);
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

    [[nodiscard]] UniValue ToJson() const
    {
        UniValue obj;
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("signal", signal.ToJson());
        return obj;
    }
};

class CMNHFManager : public AbstractEHFManager
{
private:
    CEvoDB& m_evoDb;

    static constexpr size_t MNHFCacheSize = 1000;
    Mutex cs_cache;
    // versionBit <-> height
    unordered_lru_cache<uint256, Signals, StaticSaltedHasher> mnhfCache GUARDED_BY(cs_cache) {MNHFCacheSize};

    // This cache is used only for v20 activation to avoid double lock through VersionBitsConditionChecker::SignalHeight
    VersionBitsCache v20_activation GUARDED_BY(cs_cache);
public:
    explicit CMNHFManager(CEvoDB& evoDb);
    ~CMNHFManager();
    explicit CMNHFManager(const CMNHFManager&) = delete;

    /**
     * Every new block should be processed when Tip() is updated by calling of CMNHFManager::ProcessBlock.
     * This function actually does only validate EHF transaction for this block and update internal caches/evodb state
     */
    std::optional<Signals> ProcessBlock(const CBlock& block, const CBlockIndex* const pindex, bool fJustCheck, BlockValidationState& state);

    /**
     * Every undo block should be processed when Tip() is updated by calling of CMNHFManager::UndoBlock
     * This function actually does nothing at the moment, because status of ancestor block is already know.
     * Although it should be still called to do some sanity checks
     */
    bool UndoBlock(const CBlock& block, const CBlockIndex* const pindex);


    // Implements interface
    Signals GetSignalsStage(const CBlockIndex* const pindexPrev) override;

    /**
     * Helper that used in Unit Test to forcely setup EHF signal for specific block
     */
    void AddSignal(const CBlockIndex* const pindex, int bit) LOCKS_EXCLUDED(cs_cache);
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
bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);

#endif // BITCOIN_EVO_MNHFTX_H
