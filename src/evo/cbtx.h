// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_CBTX_H
#define SYSCOIN_EVO_CBTX_H

#include <primitives/transaction.h>
#include <univalue.h>
#include <sync.h>
extern RecursiveMutex cs_main;
class CBlock;
class CBlockIndex;
class BlockValidationState;
class TxValidationState;
namespace llmq 
{
    class CFinalCommitmentTxPayload;
}
class CCoinsViewCache;

// coinbase transaction
class CCbTx
{
public:
    static const uint16_t CURRENT_VERSION = 2;

public:
    uint16_t nVersion{CURRENT_VERSION};
    int32_t nHeight{0};
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;

public:
    SERIALIZE_METHODS(CCbTx, obj) {
        READWRITE(obj.nVersion, obj.nHeight, obj.merkleRootMNList, obj.merkleRootQuorums);
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("height", (int)nHeight);
        obj.pushKV("merkleRootMNList", merkleRootMNList.ToString());
        obj.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
    }
};

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool CheckCbTx(const CCbTx &cbTx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state, CCoinsViewCache& view, const llmq::CFinalCommitmentTxPayload *qcIn = nullptr) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state, const llmq::CFinalCommitmentTxPayload *qcIn = nullptr) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif //SYSCOIN_EVO_CBTX_H
