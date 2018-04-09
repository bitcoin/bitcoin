// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cbtx.h"
#include "specialtx.h"
#include "deterministicmns.h"
#include "simplifiedmns.h"

#include "validation.h"
#include "univalue.h"

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    if (!tx.IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-invalid");

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (cbTx.nVersion > CCbTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");

    if (pindexPrev) {
        if (pindexPrev->nHeight + 1 != cbTx.nHeight)
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-height");
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindex, CValidationState& state)
{
    AssertLockHeld(cs_main);

    if (block.vtx[0]->nType != TRANSACTION_COINBASE)
        return true;

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (pindex) {
        uint256 calculatedMerkleRoot;
        if (!CalcCbTxMerkleRootMNList(block, pindex->pprev, calculatedMerkleRoot, state)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }
        if (calculatedMerkleRoot != cbTx.merkleRootMNList) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }
    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state)
{
    AssertLockHeld(cs_main);
    LOCK(deterministicMNManager->cs);

    CDeterministicMNList tmpMNList;
    if (!deterministicMNManager->BuildNewListFromBlock(block, pindexPrev, state, tmpMNList)) {
        return false;
    }

    CSimplifiedMNList sml(tmpMNList);
    bool mutated = false;
    merkleRootRet = sml.CalcMerkleRoot(&mutated);
    return !mutated;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nHeight=%d, nVersion=%d, merkleRootMNList=%s)",
        nVersion, nHeight, merkleRootMNList.ToString());
}

void CCbTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", (int)nVersion));
    obj.push_back(Pair("height", (int)nHeight));
    obj.push_back(Pair("merkleRootMNList", merkleRootMNList.ToString()));
}
