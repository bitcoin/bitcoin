// Copyright (c) 2017-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CBTX_H
#define BITCOIN_EVO_CBTX_H

#include <bls/bls.h>
#include <primitives/transaction.h>
#include <univalue.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class TxValidationState;
class CSimplifiedMNList;

namespace llmq {
class CChainLocksHandler;
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

    std::string ToString() const;

    [[nodiscard]] UniValue ToJson() const;
};
template<> struct is_serializable_enum<CCbTx::Version> : std::true_type {};

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);

bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex,
                          const llmq::CQuorumBlockProcessor& quorum_block_processor, CSimplifiedMNList&& sml,
                          BlockValidationState& state);
bool CalcCbTxMerkleRootMNList(uint256& merkleRootRet, CSimplifiedMNList&& sml, BlockValidationState& state);
bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev,
                               const llmq::CQuorumBlockProcessor& quorum_block_processor, uint256& merkleRootRet,
                               BlockValidationState& state);

bool CheckCbTxBestChainlock(const CBlock& block, const CBlockIndex* pindexPrev,
                            const llmq::CChainLocksHandler& chainlock_handler, BlockValidationState& state);
bool CalcCbTxBestChainlock(const llmq::CChainLocksHandler& chainlock_handler, const CBlockIndex* pindexPrev,
                           uint32_t& bestCLHeightDiff, CBLSSignature& bestCLSignature);

std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex);

#endif // BITCOIN_EVO_CBTX_H
