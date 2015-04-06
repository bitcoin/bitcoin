// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/consensus.h"

#include "chain.h"
#include "coins.h"
#include "consensus/validation.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "script/sigcache.h"
#include "tinyformat.h"
#include "utilmoneystr.h"
#include "version.h"

bool Consensus::CheckTx(const CTransaction& tx, CValidationState &state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        if (!Consensus::VerifyAmount(tx.vout[i].nValue))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-outofrange");
        nValueOut += tx.vout[i].nValue;
        if (!Consensus::VerifyAmount(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs
    std::set<COutPoint> vInOutPoints;
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        if (vInOutPoints.count(tx.vin[i].prevout))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        vInOutPoints.insert(tx.vin[i].prevout);
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            if (tx.vin[i].prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }
    return true;
}

bool Consensus::CheckBlockHeader(const CBlockHeader& block, int64_t nTime, CValidationState& state, const Consensus::Params& params, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, params))
        return state.DoS(50, false, REJECT_INVALID, "high-hash");

    // Check timestamp
    if (block.GetBlockTime() > nTime + 2 * 60 * 60)
        return state.Invalid(false, REJECT_INVALID, "time-too-new");

    return true;
}

bool Consensus::CheckBlock(const CBlock& block, int64_t nTime, CValidationState& state, const Consensus::Params& params, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!Consensus::CheckBlockHeader(block, nTime, state, params, fCheckPOW))
        return false;

    // Check the merkle root.
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = block.BuildMerkleTree(&mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.DoS(100, false, REJECT_INVALID, "bad-txnmrklroot", true);

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-duplicate", true);
    }

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.

    // Size limits
    if (block.vtx.empty() || block.vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-length");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-multiple");

    // Check transactions
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        if (!Consensus::CheckTx(block.vtx[i], state))
            return false;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        nSigOps += Consensus::GetLegacySigOpCount(block.vtx[i]);
    if (nSigOps > MAX_BLOCK_SIGOPS)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-sigops", true);

    return true;
}

CAmount Consensus::GetValueOut(const CTransaction& tx)
{
    CAmount nValueOut = 0;
    for (std::vector<CTxOut>::const_iterator it(tx.vout.begin()); it != tx.vout.end(); ++it)
    {
        nValueOut += it->nValue;
        if (!Consensus::VerifyAmount(it->nValue) || !Consensus::VerifyAmount(nValueOut))
            throw std::runtime_error("CTransaction::GetValueOut(): value out of range");
    }
    return nValueOut;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewEfficient& inputs, int nSpendHeight)
{
        // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
        // for an attacker to attempt to split the network.
        if (!inputs.HaveInputs(tx))
            return state.Invalid(false, REJECT_INVALID, "bad-txns-inputs-unavailable");

        CAmount nValueIn = 0;
        CAmount nFees = 0;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins *coins = inputs.AccessCoins(prevout.hash);
            assert(coins);

            // If prev is coinbase, check that it's matured
            if (coins->IsCoinBase())
                if (nSpendHeight - coins->nHeight < COINBASE_MATURITY)
                    return state.Invalid(false, REJECT_INVALID, strprintf("bad-txns-premature-spend-of-coinbase (depth %d)", nSpendHeight - coins->nHeight));

            // Check for negative or overflow input values
            nValueIn += coins->vout[prevout.n].nValue;
            if (!Consensus::VerifyAmount(coins->vout[prevout.n].nValue) || !Consensus::VerifyAmount(nValueIn))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }

        CAmount nValueOut = Consensus::GetValueOut(tx);
        if (nValueIn < nValueOut)
            return state.DoS(100, false, REJECT_INVALID, strprintf("bad-txns-in-belowout (%s < %s)", FormatMoney(nValueIn), FormatMoney(nValueOut)));

        // Tally transaction fees
        CAmount nTxFee = nValueIn - nValueOut;
        if (nTxFee < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-negative");

        nFees += nTxFee;
        if (!Consensus::VerifyAmount(nFees))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-outofrange");

    return true;
}

bool Consensus::CheckTxInputsScripts(const CTransaction& tx, CValidationState& state, const CCoinsViewEfficient& inputs, bool cacheStore, unsigned int flags)
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

bool Consensus::VerifyTx(const CTransaction& tx, CValidationState &state, int nBlockHeight, int64_t nBlockTime, const CCoinsViewEfficient& inputs, int nSpendHeight, bool cacheStore, unsigned int flags)
{
    if (!CheckTx(tx, state))
        return false;
    if (!IsFinalTx(tx, nBlockHeight, nBlockTime))
        return state.DoS(0, false, REJECT_NONSTANDARD, "non-final");
    if (!CheckTxInputs(tx, state, inputs, nSpendHeight))
        return false;
    if (GetSigOpCount(tx, inputs) > MAX_BLOCK_SIGOPS)
        return state.DoS(0, false, REJECT_NONSTANDARD, "bad-txns-too-many-sigops");
    if (!CheckTxInputsScripts(tx, state, inputs, cacheStore, flags))
        return false;        
    return true;
}

/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last nToCheck blocks, starting at pstart and going backwards.
 */
static bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned nRequired, unsigned nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }
    return (nFound >= nRequired);
}

bool Consensus::ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    // Check proof of work
    if (block.nBits != GetNextWorkRequired(pindexPrev, &block, params))
        return state.DoS(100, false, REJECT_INVALID, "bad-diffbits");

    // Check timestamp against prev
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return state.Invalid(false, REJECT_INVALID, "time-too-old");

    // Reject block.nVersion=n blocks when 95% (75% on testnet) of the network has upgraded, last version=3:
    for (unsigned int i = 2; i <= 3; i++)
        if (block.nVersion < 2 && IsSuperMajority(2, pindexPrev, params.nMajorityRejectBlockOutdated, params.nMajorityWindow))
            return state.Invalid(false, REJECT_OBSOLETE, strprintf("bad-version nVersion=%d", i-1));

    return true;
}

bool Consensus::VerifyBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& params, int64_t nTime, CBlockIndex* pindexPrev)
{
    if (!Consensus::CheckBlockHeader(block, nTime, state, params, true))
        return false;
    if (!Consensus::ContextualCheckBlockHeader(block, state, pindexPrev, params))
        return false;
    return true;
}

bool Consensus::IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        if (!tx.vin[i].IsFinal())
            return false;
    return true;
}

bool Consensus::ContextualCheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& params, const CBlockIndex* pindexPrev)
{
    const int nHeight = pindexPrev->nHeight + 1;
    // Check that all transactions are finalized
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (!Consensus::IsFinalTx(block.vtx[i], nHeight, block.GetBlockTime()))
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-nonfinal");

    // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
    // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
    if (block.nVersion >= 2 && IsSuperMajority(2, pindexPrev, params.nMajorityEnforceBlockUpgrade, params.nMajorityWindow))
    {
        CScript expect = CScript() << nHeight;
        if (block.vtx[0].vin[0].scriptSig.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), block.vtx[0].vin[0].scriptSig.begin())) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-height");
        }
    }
    return true;
}

bool Consensus::VerifyBlock(const CBlock& block, CValidationState& state, const Consensus::Params& params, int64_t nTime, const CBlockIndex* pindexPrev)
{
    if (!CheckBlock(block, nTime, state, params, true, true))
        return false;
    if (!ContextualCheckBlock(block, state, params, pindexPrev))
        return false;
    return true;    
}


unsigned Consensus::GetFlags(const CBlock& block, CBlockIndex* pindex, const Consensus::Params& params)
{
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = (pindex->GetBlockTime() >= nBIP16SwitchTime);
    unsigned int flags = fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE;

    if (block.nVersion >= 3 && IsSuperMajority(3, pindex->pprev, params.nMajorityEnforceBlockUpgrade, params.nMajorityWindow))
        flags |= SCRIPT_VERIFY_DERSIG;
    return flags;
}

bool Consensus::EnforceBIP30(const CBlock& block, CValidationState& state, const CBlockIndex* pindexPrev, const CCoinsViewEfficient& inputs)
{
    if (!(pindexPrev->nHeight==91842 && pindexPrev->GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
        (pindexPrev->nHeight==91880 && pindexPrev->GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")))
        for (unsigned int i = 1; i < block.vtx.size(); i++) {
            const CCoins* coins = inputs.AccessCoins(block.vtx[i].GetHash());
            if (coins && !coins->IsPruned())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-BIP30");
        }
    return true;
}

unsigned int Consensus::GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nSigOps += tx.vin[i].scriptSig.GetSigOpCount(false);

    for (unsigned int i = 0; i < tx.vout.size(); i++)
        nSigOps += tx.vout[i].scriptPubKey.GetSigOpCount(false);

    return nSigOps;
}

unsigned int Consensus::GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewEfficient& inputs)
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

unsigned int Consensus::GetSigOpCount(const CTransaction& tx, const CCoinsViewEfficient& inputs)
{
    return Consensus::GetLegacySigOpCount(tx) + Consensus::GetP2SHSigOpCount(tx, inputs);
}

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& params)
{
    int halvings = nHeight / params.nSubsidyHalvingInterval;
    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return 0;

    CAmount nSubsidy = 50 * COIN;
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;
    return nSubsidy;
}
