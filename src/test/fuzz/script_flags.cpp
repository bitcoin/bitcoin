// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/script.h>

#include <array>
#include <cassert>
#include <ios>
#include <utility>
#include <vector>

static SpanReader& operator>>(SpanReader& ds, script_verify_flags& f)
{
    script_verify_flags::value_type n{0};
    ds >> n;
    f = script_verify_flags::from_int(n);
    assert(n == f.as_int());
    return ds;
}

namespace {

class FuzzedSignatureChecker : public BaseSignatureChecker
{
public:
    FuzzedSignatureChecker(const CTransaction* /*tx*/, unsigned int /*in*/,
                           const CAmount& /*amount*/, const PrecomputedTransactionData& /*tx_data*/,
                           MissingDataBehavior /*mdb*/) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& pub_key,
                             const CScript& script_code, SigVersion sig_version) const override
    {
        return !sig.empty() && (sig[0] & 1);
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pub_key,
                               SigVersion sig_version, ScriptExecutionData& exec_data,
                               ScriptError* script_error = nullptr) const override
    {
        bool sig_ok = !sig.empty() && (sig[0] & 1);
        if (!sig_ok && script_error) {
            constexpr std::array<ScriptError, 3> schnorr_errs = {
                SCRIPT_ERR_SCHNORR_SIG,
                SCRIPT_ERR_SCHNORR_SIG_SIZE,
                SCRIPT_ERR_SCHNORR_SIG_HASHTYPE};
            size_t idx = (sig.size() > 1) ? (sig[1] % schnorr_errs.size()) : 0;
            *script_error = schnorr_errs[idx];
        }

        return sig_ok;
    }

    bool CheckLockTime(const CScriptNum& lock_time) const override
    {
        return (lock_time.GetInt64() & 1) != 0;
    }

    bool CheckSequence(const CScriptNum& sequence) const override
    {
        return (sequence.GetInt64() & 1) != 0;
    }

    bool CheckTaprootCommitment(const std::vector<unsigned char>& control,
                                const std::vector<unsigned char>& program,
                                const uint256& tapleaf_hash) const override
    {
        return !program.empty() && (program[0] & 1);
    }

    bool CheckWitnessScriptHash(std::span<const unsigned char> program,
                                const CScript& exec_script) const override
    {
        return !program.empty() && (program[0] & 1);
    }
};

template <typename SigChecker>
void CheckScriptFlags(FuzzBufferType buffer)
{
    if (buffer.size() > 100'000) return;
    SpanReader ds{buffer};
    try {
        const CTransaction tx(deserialize, TX_WITH_WITNESS, ds);

        script_verify_flags verify_flags;
        ds >> verify_flags;

        assert(verify_flags == script_verify_flags::from_int(verify_flags.as_int()));

        if (!IsValidFlagCombination(verify_flags)) return;

        script_verify_flags fuzzed_flags;
        ds >> fuzzed_flags;

        std::vector<CTxOut> spent_outputs;
        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            CTxOut prevout;
            ds >> prevout;
            if (!MoneyRange(prevout.nValue)) {
                // prevouts should be consensus-valid
                prevout.nValue = 1;
            }
            spent_outputs.push_back(prevout);
        }
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::move(spent_outputs));

        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            const CTxOut& prevout = txdata.m_spent_outputs.at(i);
            const SigChecker checker{&tx, i, prevout.nValue, txdata, MissingDataBehavior::ASSERT_FAIL};

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
} // namespace

/**
 * Both of the following harnesses test that all script verification flags only
 * tighten the interpreter rules (i.e. they represent soft-forks).
 */

FUZZ_TARGET(script_flags)
{
    CheckScriptFlags<TransactionSignatureChecker>(buffer);
}

// Signature validation is mocked out through FuzzedSignatureChecker
FUZZ_TARGET(script_flags_mocked)
{
    CheckScriptFlags<FuzzedSignatureChecker>(buffer);
}
