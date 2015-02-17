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
        if (tx.vout[i].nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (tx.vout[i].nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += tx.vout[i].nValue;
        if (!MoneyRange(nValueOut))
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

bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned nRequired, unsigned nToCheck)
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
