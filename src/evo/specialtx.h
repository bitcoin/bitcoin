// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_SPECIALTX_H
#define SYSCOIN_EVO_SPECIALTX_H

#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>
#include <sync.h>
extern RecursiveMutex cs_main;
class CBlock;
class CBlockIndex;
class TxValidationState;
class BlockValidationState;
class CCoinsViewCache;
class BlockManager;
bool CheckSpecialTx(BlockManager &blockman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool ProcessSpecialTxsInBlock(BlockManager &blockman, const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view, bool fJustCheck, bool fCheckCbTxMerleRoots) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

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
    scriptData << OP_RETURN << std::vector<unsigned char>(ds.begin(), ds.end());
    // if opreturn exists update payload
	if (GetSyscoinData(CTransaction(tx), vchData, nOut))
        tx.vout[nOut].scriptPubKey = scriptData;
    // otherwise add a new output with opreturn
    else
        tx.vout.push_back(CTxOut(0, scriptData));
}
uint256 CalcTxInputsHash(const CTransaction& tx);
#endif //SYSCOIN_EVO_SPECIALTX_H
