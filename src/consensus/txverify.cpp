// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/consensus.h"

#include "coins.h"
#include "consensus/validation.h"
#include "primitives/transaction.h"
#include "script/sigcache.h"
#include "tinyformat.h"

unsigned int GetSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    return GetLegacySigOpCount(tx) + GetP2SHSigOpCount(tx, inputs);
}

bool Consensus::CheckTxInputsScripts(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, bool cacheStore, unsigned int flags)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const COutPoint& prevout = tx.vin[i].prevout;
        const CCoins* coins = inputs.AccessCoins(prevout.hash);
        assert(coins);

        const CScript& scriptPubKey = coins->vout[prevout.n].scriptPubKey;
        CachingTransactionSignatureChecker checker(&tx, i, cacheStore);
        ScriptError scriptError(SCRIPT_ERR_UNKNOWN_ERROR);
        if (!VerifyScript(scriptPubKey, tx.vin[i].scriptSig, flags, checker, &scriptError))
            return state.DoS(100, false, REJECT_INVALID, 
                             strprintf("script-verify-failed (in input %d: %s)", i, ScriptErrorString(scriptError)));
    }
    return true;
}

bool Consensus::VerifyTx(const CTransaction& tx, CValidationState &state, int nBlockHeight, int64_t nBlockTime, const CCoinsViewCache& inputs, int nSpendHeight, bool cacheStore, unsigned int flags)
{
    if (!CheckTx(tx, state))
        return false;
    if (!CheckFinalTx(tx, nBlockHeight, nBlockTime))
        return state.DoS(0, false, REJECT_NONSTANDARD, "non-final");
    if (!CheckTxInputs(tx, state, inputs, nSpendHeight))
        return false;
    if (GetSigOpCount(tx, inputs) > MAX_BLOCK_SIGOPS)
        return state.DoS(0, false, REJECT_NONSTANDARD, "bad-txns-too-many-sigops");
    if (!CheckTxInputsScripts(tx, state, inputs, cacheStore, flags))
        return false;        
    return true;
}
