// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_ASSETLOCKTX_H
#define BITCOIN_EVO_ASSETLOCKTX_H

#include <bls/bls.h>
#include <consensus/amount.h>
#include <gsl/pointers.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <univalue.h>

#include <optional>

class CBlockIndex;
class CRangesSet;
class TxValidationState;
struct RPCResult;
namespace llmq {
class CQuorumManager;
} // namespace llmq
namespace node {
class BlockManager;
} // namespace node

class CAssetLockPayload
{
public:
    static constexpr uint8_t CURRENT_VERSION = 1;
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_ASSET_LOCK;

private:
    uint8_t nVersion{CURRENT_VERSION};
    std::vector<CTxOut> creditOutputs;

public:
    explicit CAssetLockPayload(const std::vector<CTxOut>& creditOutputs) :
        creditOutputs(creditOutputs)
    {}

    CAssetLockPayload() = default;

    SERIALIZE_METHODS(CAssetLockPayload, obj)
    {
        READWRITE(
            obj.nVersion,
            obj.creditOutputs
        );
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    // getters
    uint8_t getVersion() const
    {
        return nVersion;
    }

    const std::vector<CTxOut>& getCreditOutputs() const
    {
        return creditOutputs;
    }
};

class CAssetUnlockPayload
{
public:
    static constexpr uint8_t CURRENT_VERSION = 1;
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_ASSET_UNLOCK;

    static constexpr size_t MAXIMUM_WITHDRAWALS = 32;

private:
    uint8_t nVersion{CURRENT_VERSION};
    uint64_t index{0};
    uint32_t fee{0};
    uint32_t requestedHeight{0};
    uint256 quorumHash{0};
    CBLSSignature quorumSig{};

public:
    CAssetUnlockPayload(uint8_t nVersion, uint64_t index, uint32_t fee, uint32_t requestedHeight,
            uint256 quorumHash, CBLSSignature quorumSig) :
        nVersion(nVersion),
        index(index),
        fee(fee),
        requestedHeight(requestedHeight),
        quorumHash(quorumHash),
        quorumSig(quorumSig)
    {}

    CAssetUnlockPayload() = default;

    SERIALIZE_METHODS(CAssetUnlockPayload, obj)
    {
        READWRITE(
            obj.nVersion,
            obj.index,
            obj.fee,
            obj.requestedHeight,
            obj.quorumHash,
            obj.quorumSig
        );
    }

    std::string ToString() const;

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;

    bool VerifySig(const llmq::CQuorumManager& qman, const uint256& msgHash, gsl::not_null<const CBlockIndex*> pindexTip, TxValidationState& state) const;

    // getters
    uint8_t getVersion() const
    {
        return nVersion;
    }

    uint64_t getIndex() const
    {
        return index;
    }

    uint32_t getFee() const
    {
        return fee;
    }

    uint32_t getRequestedHeight() const
    {
        return requestedHeight;
    }

    const uint256& getQuorumHash() const
    {
        return quorumHash;
    }

    const CBLSSignature& getQuorumSig() const
    {
        return quorumSig;
    }

    // used by mempool to know when possible to drop a transaction as expired
    static constexpr int HEIGHT_DIFF_EXPIRING = 48;
    int getHeightToExpiry() const
    {
        return requestedHeight + HEIGHT_DIFF_EXPIRING;
    }
};

bool CheckAssetLockTx(const CTransaction& tx, TxValidationState& state);
bool CheckAssetUnlockTx(const node::BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, const std::optional<CRangesSet>& indexes, TxValidationState& state);
bool CheckAssetLockUnlockTx(const node::BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, const std::optional<CRangesSet>& indexes, TxValidationState& state);
bool GetAssetUnlockFee(const CTransaction& tx, CAmount& txfee, TxValidationState& state);

#endif // BITCOIN_EVO_ASSETLOCKTX_H
