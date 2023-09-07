// Copyright (c) 2017-2022 The Dash Core developers
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
class CCoinsViewCache;
class TxValidationState;

namespace llmq {
class CQuorumBlockProcessor;
class CChainLocksHandler;
}// namespace llmq

// Forward declaration from core_io to get rid of circular dependency
UniValue ValueFromAmount(const CAmount& amount);

// coinbase transaction
class CCbTx
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_COINBASE;
    static constexpr uint16_t CB_V19_VERSION = 2;
    static constexpr uint16_t CB_V20_VERSION = 3;

    uint16_t nVersion{CB_V19_VERSION};
    int32_t nHeight{0};
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;
    uint32_t bestCLHeightDiff;
    CBLSSignature bestCLSignature;
    CAmount creditPoolBalance{0};

    SERIALIZE_METHODS(CCbTx, obj)
    {
        READWRITE(obj.nVersion, obj.nHeight, obj.merkleRootMNList);

        if (obj.nVersion >= CB_V19_VERSION) {
            READWRITE(obj.merkleRootQuorums);
            if (obj.nVersion >= CB_V20_VERSION) {
                READWRITE(COMPACTSIZE(obj.bestCLHeightDiff));
                READWRITE(obj.bestCLSignature);
                READWRITE(obj.creditPoolBalance);
            }
        }

    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("height", nHeight);
        obj.pushKV("merkleRootMNList", merkleRootMNList.ToString());
        if (nVersion >= CB_V19_VERSION) {
            obj.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
            if (nVersion >= CB_V20_VERSION) {
                obj.pushKV("bestCLHeightDiff", static_cast<int>(bestCLHeightDiff));
                obj.pushKV("bestCLSignature", bestCLSignature.ToString());
                obj.pushKV("creditPoolBalance", ValueFromAmount(creditPoolBalance));
            }
        }
    }
};

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state);

bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, const llmq::CQuorumBlockProcessor& quorum_block_processor, BlockValidationState& state, const CCoinsViewCache& view);
bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state, const CCoinsViewCache& view);
bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, const llmq::CQuorumBlockProcessor& quorum_block_processor, uint256& merkleRootRet, BlockValidationState& state);

bool CheckCbTxBestChainlock(const CBlock& block, const CBlockIndex* pindexPrev, const llmq::CChainLocksHandler& chainlock_handler, BlockValidationState& state);
bool CalcCbTxBestChainlock(const llmq::CChainLocksHandler& chainlock_handler, const CBlockIndex* pindexPrev, uint32_t& bestCLHeightDiff, CBLSSignature& bestCLSignature);

std::optional<CCbTx> GetCoinbaseTx(const CBlockIndex* pindex);
std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex);
#endif // BITCOIN_EVO_CBTX_H
