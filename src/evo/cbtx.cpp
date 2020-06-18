// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cbtx.h"
#include "deterministicmns.h"
#include "simplifiedmns.h"
#include "specialtx.h"

#include "chainparams.h"
#include "consensus/merkle.h"
#include "univalue.h"
#include "validation.h"

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_COINBASE) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-type");
    }

    if (!tx.IsCoinBase()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-invalid");
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
    }


    if (pindexPrev) {
        if (cbTx.nVersion < 2) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
        }
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, CValidationState& state)
{
    if (block.vtx[0]->nVersion != SYSCOIN_TX_VERSION_MN_COINBASE) {
        return true;
    }

    static int64_t nTimePayload = 0;
    static int64_t nTimeMerkleMNL = 0;

    int64_t nTime1 = GetTimeMicros();

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    int64_t nTime2 = GetTimeMicros(); nTimePayload += nTime2 - nTime1;
    LogPrint(BCLog::BENCH, "          - GetTxPayload: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimePayload * 0.000001);

    if (pindex) {
        uint256 calculatedMerkleRoot;
        if (!CalcCbTxMerkleRootMNList(block, pindex->pprev, calculatedMerkleRoot, state)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }
        if (calculatedMerkleRoot != cbTx.merkleRootMNList) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }

        int64_t nTime3 = GetTimeMicros(); nTimeMerkleMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCH, "          - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMerkleMNL * 0.000001);
    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state)
{
    LOCK(deterministicMNManager->cs);

    static int64_t nTimeDMN = 0;
    static int64_t nTimeSMNL = 0;
    static int64_t nTimeMerkle = 0;

    int64_t nTime1 = GetTimeMicros();

    CDeterministicMNList tmpMNList;
    if (!deterministicMNManager->BuildNewListFromBlock(block, pindexPrev, state, tmpMNList, false)) {
        return false;
    }

    int64_t nTime2 = GetTimeMicros(); nTimeDMN += nTime2 - nTime1;
    LogPrint(BCLog::BENCH, "            - BuildNewListFromBlock: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeDMN * 0.000001);

    CSimplifiedMNList sml(tmpMNList);

    int64_t nTime3 = GetTimeMicros(); nTimeSMNL += nTime3 - nTime2;
    LogPrint(BCLog::BENCH, "            - CSimplifiedMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeSMNL * 0.000001);

    static CSimplifiedMNList smlCached;
    static uint256 merkleRootCached;
    static bool mutatedCached{false};

    if (sml.mnList == smlCached.mnList) {
        merkleRootRet = merkleRootCached;
        return !mutatedCached;
    }

    bool mutated = false;
    merkleRootRet = sml.CalcMerkleRoot(&mutated);

    int64_t nTime4 = GetTimeMicros(); nTimeMerkle += nTime4 - nTime3;
    LogPrint(BCLog::BENCH, "            - CalcMerkleRoot: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkle * 0.000001);

    smlCached = std::move(sml);
    merkleRootCached = merkleRootRet;
    mutatedCached = mutated;

    return !mutated;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, merkleRootMNList=%s)",
        nVersion, merkleRootMNList.ToString());
}
