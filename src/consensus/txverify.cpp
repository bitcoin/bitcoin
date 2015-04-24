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
#include "util.h"
#include "utilmoneystr.h"
#include "version.h"

#include <boost/foreach.hpp>

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        if (!txin.IsFinal())
            return false;
    return true;
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut &prevout = inputs.GetOutputFor(tx.vin[i]);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

bool CheckTransaction(const CTransaction& tx, CValidationState &state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, error("CheckTransaction(): vin empty"),
                         REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, error("CheckTransaction(): vout empty"),
                         REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, error("CheckTransaction(): size limits failed"),
                         REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, error("CheckTransaction(): txout.nValue negative"),
                             REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, error("CheckTransaction(): txout.nValue too high"),
                             REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, error("CheckTransaction(): txout total out of range"),
                             REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs
    std::set<COutPoint> vInOutPoints;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (vInOutPoints.count(txin.prevout))
            return state.DoS(100, error("CheckTransaction(): duplicate inputs"),
                             REJECT_INVALID, "bad-txns-inputs-duplicate");
        vInOutPoints.insert(txin.prevout);
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, error("CheckTransaction(): coinbase script size"),
                             REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, error("CheckTransaction(): prevout is null"),
                                 REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight)
{
        // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
        // for an attacker to attempt to split the network.
        if (!inputs.HaveInputs(tx))
            return state.Invalid(error("CheckInputs(): %s inputs unavailable", tx.GetHash().ToString()));

        CAmount nValueIn = 0;
        CAmount nFees = 0;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins *coins = inputs.AccessCoins(prevout.hash);
            assert(coins);

            // If prev is coinbase, check that it's matured
            if (coins->IsCoinBase()) {
                if (nSpendHeight - coins->nHeight < COINBASE_MATURITY)
                    return state.Invalid(
                        error("CheckInputs(): tried to spend coinbase at depth %d", nSpendHeight - coins->nHeight),
                        REJECT_INVALID, "bad-txns-premature-spend-of-coinbase");
            }

            // Check for negative or overflow input values
            nValueIn += coins->vout[prevout.n].nValue;
            if (!MoneyRange(coins->vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return state.DoS(100, error("CheckInputs(): txin values out of range"),
                                 REJECT_INVALID, "bad-txns-inputvalues-outofrange");

        }

        if (nValueIn < tx.GetValueOut())
            return state.DoS(100, error("CheckInputs(): %s value in (%s) < value out (%s)",
                                        tx.GetHash().ToString(), FormatMoney(nValueIn), FormatMoney(tx.GetValueOut())),
                             REJECT_INVALID, "bad-txns-in-belowout");

        // Tally transaction fees
        CAmount nTxFee = nValueIn - tx.GetValueOut();
        if (nTxFee < 0)
            return state.DoS(100, error("CheckInputs(): %s nTxFee < 0", tx.GetHash().ToString()),
                             REJECT_INVALID, "bad-txns-fee-negative");
        nFees += nTxFee;
        if (!MoneyRange(nFees))
            return state.DoS(100, error("CheckInputs(): nFees out of range"),
                             REJECT_INVALID, "bad-txns-fee-outofrange");
    return true;
}
