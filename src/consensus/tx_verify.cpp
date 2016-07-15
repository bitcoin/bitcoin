// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"

#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "validation.h"

bool Consensus::CheckTxCoinbase(const CTransaction& tx, CValidationState& state, const int64_t flags, const int64_t nHeight)
{
    // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
    if (flags & TX_COINBASE_VERIFY_BIP34) {
        const CScript coinbaseSigScript = tx.vin[0].scriptSig;
        CScript expect = CScript() << nHeight;
        if (coinbaseSigScript.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), coinbaseSigScript.begin()))
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-height", false, "block height mismatch in coinbase");
    }

    return true;
}

bool Consensus::VerifyTx(const CTransaction& tx, CValidationState& state, const int64_t flags, const int64_t nHeight)
{
    if (tx.IsCoinBase())
        return CheckTxCoinbase(tx, state, flags, nHeight);

    return true;
}
