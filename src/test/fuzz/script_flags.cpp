// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/script.h>

#include <cassert>
#include <ios>
#include <utility>
#include <vector>

namespace {

inline uint64_t HashSig(Span<const unsigned char> sig)
{
    CSipHasher hasher{0xdead, 0xbeef};
    hasher.Write(sig);
    return hasher.Finalize();
}
// Reduce a CScriptNum to one bit
inline bool HashScriptNum(const CScriptNum& num)
{
    CSipHasher hasher{0xdead, 0xbeef};
    hasher.Write(num.getvch());
    return hasher.Finalize() & 1;
}

class FuzzedSignatureChecker : public BaseSignatureChecker
{
public:
    FuzzedSignatureChecker(const CTransaction* tx, unsigned int in,
                           const CAmount& amount, const PrecomputedTransactionData& tx_data,
                           MissingDataBehavior mdb) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& pub_key,
                             const CScript& script_code, SigVersion sig_version) const override
    {
        return !sig.empty() && (HashSig(sig) & 1);
    }
    bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pub_key,
                               SigVersion sig_version, ScriptExecutionData& exec_data,
                               ScriptError* script_error = nullptr) const override
    {
        uint64_t fuzz_hash{HashSig(sig)};
        bool sig_ok = fuzz_hash & 1;
        if (!sig_ok && script_error) {
            constexpr std::array<ScriptError, 3> schnorr_errs = {
                SCRIPT_ERR_SCHNORR_SIG,
                SCRIPT_ERR_SCHNORR_SIG_SIZE,
                SCRIPT_ERR_SCHNORR_SIG_HASHTYPE};
            *script_error = schnorr_errs[(fuzz_hash >> 1) % schnorr_errs.size()];
        }

        return sig_ok;
    }
    bool CheckLockTime(const CScriptNum& lock_time) const override
    {
        return HashScriptNum(lock_time);
    }
    bool CheckSequence(const CScriptNum& sequence) const override
    {
        return HashScriptNum(sequence);
    }

    bool CheckTaprootCommitment(const std::vector<unsigned char>& control,
                                const std::vector<unsigned char>& program,
                                const uint256& tapleaf_hash) const override
    {
        return !program.empty() && (program[0] & 1);
    }

    bool CheckWitnessScriptHash(Span<const unsigned char> program,
                                const CScript& exec_script) const override
    {
        return !program.empty() && (program[0] & 1);
    }
};

template <typename SigChecker>
void CheckScriptFlags(FuzzBufferType buffer)
{
    if (buffer.size() > 100'000) return;
    DataStream ds{buffer};
    try {
        const CTransaction tx(deserialize, TX_WITH_WITNESS, ds);

        unsigned int verify_flags;
        ds >> verify_flags;

        if (!IsValidFlagCombination(verify_flags)) return;

        unsigned int fuzzed_flags;
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
