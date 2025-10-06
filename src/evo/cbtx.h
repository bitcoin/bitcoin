// Copyright (c) 2017-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CBTX_H
#define BITCOIN_EVO_CBTX_H

#include <bls/bls.h>

#include <primitives/transaction.h>

#include <univalue.h>

#include <optional>
#include <string>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CDeterministicMNList;
class TxValidationState;
struct RPCResult;

namespace llmq {
class CQuorumBlockProcessor;
}// namespace llmq

// coinbase transaction
class CCbTx
{
public:
    enum class Version : uint16_t {
        INVALID = 0,
        MERKLE_ROOT_MNLIST = 1,
        MERKLE_ROOT_QUORUMS = 2,
        CLSIG_AND_BALANCE = 3,
        UNKNOWN,
    };

    static constexpr auto SPECIALTX_TYPE = TRANSACTION_COINBASE;
    Version nVersion{Version::MERKLE_ROOT_QUORUMS};
    int32_t nHeight{0};
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;
    uint32_t bestCLHeightDiff{0};
    CBLSSignature bestCLSignature;
    CAmount creditPoolBalance{0};

    SERIALIZE_METHODS(CCbTx, obj)
    {
        READWRITE(obj.nVersion, obj.nHeight, obj.merkleRootMNList);

        if (obj.nVersion >= Version::MERKLE_ROOT_QUORUMS) {
            READWRITE(obj.merkleRootQuorums);
            if (obj.nVersion >= Version::CLSIG_AND_BALANCE) {
                READWRITE(COMPACTSIZE(obj.bestCLHeightDiff));
                READWRITE(obj.bestCLSignature);
                READWRITE(obj.creditPoolBalance);
            }
        }

    }

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    std::string ToString() const;

    [[nodiscard]] UniValue ToJson() const;
};
template<> struct is_serializable_enum<CCbTx::Version> : std::true_type {};

bool CheckCbTx(const CCbTx& cbTx, const CBlockIndex* pindexPrev, TxValidationState& state);

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev,
                               const llmq::CQuorumBlockProcessor& quorum_block_processor, uint256& merkleRootRet,
                               BlockValidationState& state);

std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex);

#endif // BITCOIN_EVO_CBTX_H
