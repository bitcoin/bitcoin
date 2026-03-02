// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_SPECIALTX_H
#define SYSCOIN_EVO_SPECIALTX_H

#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>
#include <kernel/cs_main.h>
class CBlock;
class CBlockIndex;
class uint256;
class TxValidationState;
class BlockValidationState;
class CCoinsViewCache;
class ChainstateManager;
class CDeterministicMNListNEVMAddressDiff;
namespace llmq {
class CBTCCheckpointSig;
}
namespace node {
class BlockManager;
}
bool CheckSpecialTx(node::BlockManager &blockman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck, bool check_sigs) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool ProcessSpecialTxsInBlock(ChainstateManager &chainman, const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CDeterministicMNListNEVMAddressDiff &diff, CCoinsViewCache& view, bool fJustCheck, bool check_sigs, bool ibd) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CDeterministicMNListNEVMAddressDiff& diffNEVM, bool bReverify, bool bReplay) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

// SYSCOIN: helpers for extracting BTCC and BTCPREV from coinbase Syscoin-data payload.
bool ExtractBTCCReceipt(const CBlock& block, llmq::CBTCCheckpointSig& receipt);
bool ExtractBTCPREVCommitment(const CBlock& block, uint256& btcPrevHash);

template <typename T>
inline bool GetTxPayload(const std::vector<unsigned char>& payload, T& obj)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ds >> obj;
    } catch (std::exception& e) {
        return false;
    }
    return true;
}
template <typename T>
inline bool GetTxPayload(const CMutableTransaction& tx, T& obj)
{
    return GetTxPayload(CTransaction(tx), obj);
}
template <typename T>
inline bool GetTxPayload(const CTransaction& tx, T& obj)
{
    std::vector<unsigned char> vchData;
	int nOut;
	if (!GetSyscoinData(tx, vchData, nOut))
		return false;
    return GetTxPayload(vchData, obj);
}
template <typename T>
void SetTxPayload(CMutableTransaction& tx, const T& payload)
{
    std::vector<unsigned char> vchData;
	int nOut;
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    CScript scriptData;
    const auto bytesVec = MakeUCharSpan(ds);
    scriptData << OP_RETURN << std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
    // if opreturn exists update payload
	if (GetSyscoinData(CTransaction(tx), vchData, nOut))
        tx.vout[nOut].scriptPubKey = scriptData;
    // otherwise add a new output with opreturn
    else
        tx.vout.push_back(CTxOut(0, scriptData));
}
uint256 CalcTxInputsHash(const CTransaction& tx);
#endif // SYSCOIN_EVO_SPECIALTX_H
