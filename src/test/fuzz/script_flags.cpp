// Copyright (c) 2009-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <streams.h>
#include <util/memory.h>
#include <version.h>

#include <test/fuzz/fuzz.h>

/** Flags that are not forbidden by an assert */
static bool IsValidFlagCombination(unsigned flags);

void initialize()
{
    static const ECCVerifyHandle verify_handle;
}

void test_one_input(const std::vector<uint8_t>& buffer)
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

        unsigned int verify_flags;
        ds >> verify_flags;

        if (!IsValidFlagCombination(verify_flags)) return;

        unsigned int fuzzed_flags;
        ds >> fuzzed_flags;

        std::vector<CTxOut> spent_outputs;
        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            CTxOut prevout;
            ds >> prevout;
            spent_outputs.push_back(prevout);
        }
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::move(spent_outputs));

        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            const CTxOut& prevout = txdata.m_spent_outputs.at(i);
            const TransactionSignatureChecker checker{&tx, i, prevout.nValue, txdata};

            ScriptError serror;
            const bool ret = VerifyScript(tx.vin.at(i).scriptSig, prevout.scriptPubKey, &tx.vin.at(i).scriptWitness, verify_flags, checker, &serror);
            assert(ret == (serror == SCRIPT_ERR_OK));

            // Verify that removing flags from a passing test or adding flags to a failing test does not change the result
            if (ret) {
                verify_flags &= ~fuzzed_flags;
            } else {
                verify_flags |= fuzzed_flags;
            }
            if (!IsValidFlagCombination(verify_flags)) return;

            ScriptError serror_fuzzed;
            const bool ret_fuzzed = VerifyScript(tx.vin.at(i).scriptSig, prevout.scriptPubKey, &tx.vin.at(i).scriptWitness, verify_flags, checker, &serror_fuzzed);
            assert(ret_fuzzed == (serror_fuzzed == SCRIPT_ERR_OK));

            assert(ret_fuzzed == ret);
        }
    } catch (const std::ios_base::failure&) {
        return;
    }
}

static bool IsValidFlagCombination(unsigned flags)
{
    if (flags & SCRIPT_VERIFY_CLEANSTACK && ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) return false;
    if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) return false;
    return true;
}
