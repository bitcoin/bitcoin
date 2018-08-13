// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_SPECIALTX_H
#define CROWN_SPECIALTX_H

#include "streams.h"
#include "version.h"

#include "init.h"
#include "util.h"
#include "pubkey.h"
#include "coincontrol.h"
#include "rpcprotocol.h"
#include "legacysigner.h"


class CTransaction;
class CBlock;
class CBlockIndex;
class CValidationState;

bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state);
bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state);
bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex);

template <typename T>
inline bool GetTxPayload(const std::vector<unsigned char>& payload, T& obj)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);
    
    try
    {
        ds >> obj;
    }
    catch (std::exception& e)
    {
        return false;
    }

    if (!ds.empty())
        return false;

    return true;
}

template <typename T>
inline bool GetTxPayload(const CMutableTransaction& tx, T& obj)
{
    return GetTxPayload(tx.extraPayload, obj);
}

template <typename T>
inline bool GetTxPayload(const CTransaction& tx, T& obj)
{
    return GetTxPayload(tx.extraPayload, obj);
}

template <typename T>
void SetTxPayload(CMutableTransaction& tx, const T& payload)
{
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.extraPayload.assign(ds.begin(), ds.end());
}

uint256 CalcTxInputsHash(const CTransaction& tx);

#endif//CROWN_SPECIALTX_H
