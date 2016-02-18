// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"

#include "validation.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"

// TODO remove the following dependencies
#include "main.h"

bool Consensus::CheckTxPreInputs(const CTransaction& tx, CValidationState& state, const int nHeight, int64_t nLockTimeCutoff, int64_t& nSigOps)
{
    if (!IsFinalTx(tx, nHeight, nLockTimeCutoff))
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-nonfinal", false, "non-final transaction");

    if (!CheckTransaction(tx, state))
        return false;

    nSigOps += GetLegacySigOpCount(tx);
    if (nSigOps > MAX_BLOCK_SIGOPS)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-sigops", "too many sigops");

    return true;
}

bool Consensus::VerifyTx(const CTransaction& tx, CValidationState& state, const unsigned int flags, const int nHeight, const int64_t nMedianTime, const int64_t nBlockTime, int64_t& nSigOps)
{
    const int64_t nLockTimeCutoff = (flags & LOCKTIME_MEDIAN_TIME_PAST) ? nMedianTime : nBlockTime;
    if (!CheckTxPreInputs(tx, state, nHeight, nLockTimeCutoff, nSigOps))
        return false;

    return true;
}
