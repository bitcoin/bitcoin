// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "amount.h"
#include "core.h"
#include "main.h"
#include "sync.h"
#include "txmempool.h"
#include "util.h"

#include <cmath>
#include <string>

bool CNodePolicy::AcceptTxPoolPreInputs(CTxMemPool& pool, CValidationState& state, const CTransaction& tx)
{
    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    std::string reason;
    if (fRequireStandardTx && !IsStandardTx(tx, reason))
        return state.DoS(0,
                         error("%s : nonstandard transaction: %s", __func__, reason),
                         REJECT_NONSTANDARD, reason);

    // Check for conflicts with in-memory transactions
    if (pool.lookupConflicts(tx, NULL))
    {
        // Disable replacement feature for now
        return false;
    }

    return true;
}

bool CNodePolicy::AcceptTxWithInputs(CTxMemPool& pool, CValidationState& state, const CTransaction& tx, CCoinsViewCache& view)
{
    // Check for non-standard pay-to-script-hash in inputs
    if (fRequireStandardTx && !AreInputsStandard(tx, view))
        return error("%s : nonstandard transaction input", __func__);

    // Check that the transaction doesn't have an excessive number of
    // sigops, making it impossible to mine. Since the coinbase transaction
    // itself can contain sigops MAX_TX_SIGOPS is less than
    // MAX_BLOCK_SIGOPS; we still consider this an invalid rather than
    // merely non-standard transaction.
    unsigned int nSigOps = GetLegacySigOpCount(tx);
    nSigOps += GetP2SHSigOpCount(tx, view);
    if (nSigOps > MAX_TX_SIGOPS)
        return state.DoS(0,
                         error("%s : too many sigops %s, %d > %d",
                               __func__, tx.GetHash().ToString(), nSigOps, MAX_TX_SIGOPS),
                         REJECT_NONSTANDARD, "bad-txns-too-many-sigops");

    return true;
}

bool CNodePolicy::AcceptMemPoolEntry(CTxMemPool& pool, CValidationState& state, CTxMemPoolEntry& entry, CCoinsViewCache& view, bool& fRateLimit)
{
    const CTransaction& tx = entry.GetTx();

    CAmount nFees = entry.GetFee();
    unsigned int nSize = entry.GetTxSize();

    // Don't accept it if it can't get into a block
    CAmount txMinFee = GetMinRelayFee(tx, nSize, true);
    if (nFees < txMinFee)
        return state.DoS(0, error("%s : not enough fees %s, %d < %d",
                                  __func__, tx.GetHash().ToString(), nFees, txMinFee),
                         REJECT_INSUFFICIENTFEE, "insufficient fee");

    // Continuously rate-limit free (really, very-low-fee)transactions
    // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
    // be annoying or make others' transactions take longer to confirm.
    fRateLimit = (nFees < ::minRelayTxFee.GetFee(nSize));

    return true;
}

bool CNodePolicy::RateLimitTx(CTxMemPool& pool, CValidationState& state, CTxMemPoolEntry& entry, CCoinsViewCache& view)
{
    static CCriticalSection csFreeLimiter;
    static double dFreeCount;
    static int64_t nLastTime;
    int64_t nNow = GetTime();

    LOCK(csFreeLimiter);

    // Use an exponentially decaying ~10-minute window:
    dFreeCount *= pow(1.0 - 1.0/600.0, (double)(nNow - nLastTime));
    nLastTime = nNow;
    // -limitfreerelay unit is thousand-bytes-per-minute
    // At default rate it would take over a month to fill 1GB
    if (dFreeCount >= GetArg("-limitfreerelay", 15)*10*1000)
        return state.DoS(0, error("%s : free transaction rejected by rate limiter", __func__),
                         REJECT_INSUFFICIENTFEE, "insufficient priority");
    unsigned int nSize = entry.GetTxSize();
    LogPrint("mempool", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
    dFreeCount += nSize;

    return true;
}
