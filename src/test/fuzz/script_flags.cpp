// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <streams.h>
#include <test/util/script.h>
#include <version.h>

#include <test/fuzz/fuzz.h>

FUZZ_TARGET(script_flags)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int nVersion;
        ds >> nVersion;
        ds.SetVersion(nVersion);
    } catch (const std::ios_base::failure&) {
        return;
    }

    try {
        const CTransaction tx(deserialize, ds);
        const PrecomputedTransactionData txdata(tx);

        unsigned int verify_flags;
        ds >> verify_flags;

        if (!IsValidFlagCombination(verify_flags)) return;

        unsigned int fuzzed_flags;
        ds >> fuzzed_flags;

        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            CTxOut prevout;
            ds >> prevout;
            if (!MoneyRange(prevout.nValue)) {
                // prevouts should be consensus-valid
                prevout.nValue = 1;
            }

            const TransactionSignatureChecker checker{&tx, i, prevout.nValue, txdata};

            ScriptError serror;
            const bool ret = VerifyScript(tx.vin.at(i).scriptSig, prevout.scriptPubKey, verify_flags, checker, &serror);
            assert(ret == (serror == SCRIPT_ERR_OK));

            // Verify that removing flags from a passing test or adding flags to a failing test does not change the result
            if (ret) {
                verify_flags &= ~fuzzed_flags;
            } else {
                verify_flags |= fuzzed_flags;
            }
            if (!IsValidFlagCombination(verify_flags)) return;

            ScriptError serror_fuzzed;
            const bool ret_fuzzed = VerifyScript(tx.vin.at(i).scriptSig, prevout.scriptPubKey, verify_flags, checker, &serror_fuzzed);
            assert(ret_fuzzed == (serror_fuzzed == SCRIPT_ERR_OK));

            assert(ret_fuzzed == ret);
        }
    } catch (const std::ios_base::failure&) {
        return;
    }
}
