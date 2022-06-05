// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// TODO: Don't know whether Taproot/Tapscript tests should be exercising
// `libconsensus` the way the tests in `script_tests` do

#include <core_io.h>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <span.h>
#include <test/util/boost_test_boosts.h>
#include <test/util/pretty_data.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <test/util/vector.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <limits>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using namespace test::util::literals;

namespace {

typedef std::vector<unsigned char> valtype;

/**
 * Value/Name pair used in data-driven tests
 */
template <typename V>
struct vn_pair {
    vn_pair(V value, std::string_view name) : m_value(value), m_name(name) {}

    const V m_value;
    const std::string_view m_name;
};

/**
 * Sequence of value/name pairs used in data-driven tests
 */
template <typename V>
using vn_sequence = std::vector<vn_pair<V>>;

/**
 * Invokes undefined behavior.  See `std::unreachable` in C++23.
 */
[[noreturn]] inline void declare_unreachable()
{
#ifdef _MSC_VER
    __assume(false);
#else
    // Assume all other compilers than MSVC implement this GCC builtin.
    __builtin_unreachable();
#endif
}

/**
 * Representation changer to fill an integral type with a known pattern.
 *
 * Pattern is successive byte values given a starting point.  Endianness doesn't
 * matter.
 */
union FillWithPattern {
    uint256 u256{0};
    uint64_t u64raw[sizeof(uint256) / sizeof(uint64_t)];
    uint32_t u32[sizeof(uint256) / sizeof(uint32_t)];
    int32_t i32[sizeof(uint256) / sizeof(int32_t)];
    uint8_t u8[sizeof(uint256)];

    constexpr FillWithPattern(uint8_t start)
    {
        for (auto it = std::begin(u8); it != std::end(u8); ++it) {
            *it = start++;
        }
    }

    uint64_t u64() const
    {
        // It is desirable to force high bit off
        return u64raw[0] & static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    }
};

/**
 * The two possible actions for our mock signature checker
 */
enum class CHECKER_VALIDATION { ALWAYS_SUCCEEDS,
                                ALWAYS_FAILS };

/**
 * For these tests don't need _real_ signature/pubkey validation.  That is
 * tested elsewhere.  So we just _mock_ the signature checker and force it
 * to answer valid/invalid as we wish.
 */
class SignatureCheckerMock : public BaseSignatureChecker
{
    //! What kind of mock checker is this?
    CHECKER_VALIDATION m_kind = CHECKER_VALIDATION::ALWAYS_FAILS;

    //! True _iff_ CheckSchnorrSignature was actually called
    mutable bool m_was_called = false;

public:
    //! Whether this mock always validates, or always fails, the signature/pubkey check.
    explicit SignatureCheckerMock(CHECKER_VALIDATION kind) : m_kind(kind) {}

    //! Mocks the actual checking of the validity of the Schnorr signature by always succeeding or always failing
    bool CheckSchnorrSignature(Span<const unsigned char> sig,
                               Span<const unsigned char> pubkey,
                               SigVersion sigversion,
                               ScriptExecutionData& execdata,
                               ScriptError* serror = nullptr) const override
    {
        m_was_called = true;
        switch (m_kind) {
        case CHECKER_VALIDATION::ALWAYS_SUCCEEDS:
            if (serror) *serror = SCRIPT_ERR_OK;
            return true;

        case CHECKER_VALIDATION::ALWAYS_FAILS:
            if (serror) *serror = SCRIPT_ERR_SCHNORR_SIG;
            return false;
        }
        declare_unreachable();
    }

    bool CheckerWasCalled() const
    {
        return m_was_called;
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(script_tapscript_tests, BasicTestingSetup)

/**
 * Testing EvalScript OP_CHECKSIGADD branch and EvalChecksigTapscript, both in
 * interpreter.cpp, against the BIP342 "Rules for signature opcodes".
 */
BOOST_AUTO_TEST_CASE(eval_checksigadd_basic_checks)
{
    const valtype SIG_64BYTES(64, 0); // N.B.: Must be () not {}!
    const valtype SIG_65BYTES(65, 0);
    const valtype SIG_EMPTY{};

    const valtype PUBKEY_32BYTES(32, 0);
    const valtype PUBKEY_15BYTES(15, 0);
    const valtype PUBKEY_EMPTY{};

    constexpr int64_t TEST_NUM = 10;

    constexpr int64_t START_VALIDATION_WEIGHT{90};
    constexpr int64_t BIP342_SIGOPS_LIMIT{50};
    constexpr int64_t END_VALIDATION_WEIGHT{START_VALIDATION_WEIGHT - BIP342_SIGOPS_LIMIT};

    /**
     * A fluent API for running these tests.
     *
     * (Easiest way to understand this class is to look at the actual tests
     * that follow in this function.)
     */
    struct Context {
        explicit Context(std::string_view descr) : m_test_description(descr)
        {
            m_execdata.m_validation_weight_left_init = true;
            m_execdata.m_validation_weight_left = START_VALIDATION_WEIGHT;
        }

        std::string m_test_description;
        SigVersion m_sig_version = SigVersion::TAPSCRIPT;
        uint32_t m_flags = 0;
        CScript m_script;
        ScriptError m_err = SCRIPT_ERR_OK;
        std::vector<valtype> m_stack;
        ScriptExecutionData m_execdata;
        CHECKER_VALIDATION m_kind;
        bool m_sigchecker_was_called = false;
        int64_t m_caller_line = 0;
        bool m_result = false;

        Context& SetVersion(SigVersion v)
        {
            m_sig_version = v;
            return *this;
        }

        Context& SetChecker(CHECKER_VALIDATION kind)
        {
            m_kind = kind;
            return *this;
        }

        Context& SetRemainingWeight(int64_t w)
        {
            m_execdata.m_validation_weight_left = w;
            return *this;
        }

        Context& AddFlags(uint32_t f)
        {
            m_flags |= f;
            return *this;
        }

        CScript& SetScript()
        {
            return m_script;
        }

        Context& DoTest(int64_t line)
        {
            SignatureCheckerMock checker_mock(m_kind);
            m_caller_line = line;
            m_result = EvalScript(m_stack, m_script,
                                  SCRIPT_VERIFY_TAPROOT | m_flags,
                                  checker_mock,
                                  m_sig_version,
                                  m_execdata,
                                  &m_err);
            m_sigchecker_was_called = checker_mock.CheckerWasCalled();
            return *this;
        }

        Context& CheckCallSucceeded()
        {
            BOOST_CHECK_MESSAGE(m_result,
                                Descr()
                                    << ": EvalScript succeeded, as expected");
            BOOST_CHECK_MESSAGE(m_err == SCRIPT_ERR_OK,
                                Descr()
                                    << ": Error code expected OK, actual was "
                                    << ScriptErrorString(m_err));
            return *this;
        }

        Context& CheckCallFailed(ScriptError expected)
        {
            BOOST_CHECK_MESSAGE(!m_result,
                                Descr()
                                    << ": EvalScript failed, as expected");
            BOOST_CHECK_MESSAGE(m_err == expected,
                                Descr()
                                    << ": Error code expected " << ScriptErrorString(expected)
                                    << ", actual was " << ScriptErrorString(m_err));
            return *this;
        }

        Context& CheckSignatureWasValidated()
        {
            BOOST_CHECK_MESSAGE(m_sigchecker_was_called,
                                Descr() << ": CheckSchnorrSignature was called, as expected");
            return *this;
        }

        Context& CheckSignatureWasNotValidated()
        {
            BOOST_CHECK_MESSAGE(!m_sigchecker_was_called,
                                Descr() << ": CheckSchnorrSignature was not called, as expected");
            return *this;
        }

        Context& CheckRemainingValidationWeight(int64_t expected)
        {
            BOOST_CHECK_MESSAGE(m_execdata.m_validation_weight_left == expected,
                                Descr()
                                    << ": Remaining validation weight expected "
                                    << expected << ", actual was "
                                    << m_execdata.m_validation_weight_left);
            return *this;
        }

        Context& CheckStackDepth(std::size_t expected)
        {
            BOOST_CHECK_MESSAGE(m_stack.size() == expected,
                                Descr()
                                    << ": Stack depth expected " << expected
                                    << ", actual was " << m_stack.size());
            return *this;
        }

        Context& CheckTOS(int64_t expected)
        {
            BOOST_CHECK_MESSAGE(!m_stack.empty(),
                                Descr()
                                    << ": Stack expected at least one item, actually was empty");
            const int64_t actual = CScriptNum(m_stack.at(0), false).GetInt64();
            BOOST_CHECK_MESSAGE(expected == actual,
                                Descr()
                                    << ": Top-of-stack expected " << expected
                                    << ", actual was " << actual);
            return *this;
        }

    private:
        std::string Descr()
        {
            std::string descr;
            descr.reserve(m_test_description.size() + 20);
            descr += m_test_description;
            descr += " (@";
            descr += as_string(m_caller_line);
            descr += ")";
            return descr;
        }
    };

    {
        Context ctx("SigVersion must not be BASE");
        ctx.SetVersion(SigVersion::BASE).SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_BAD_OPCODE)
            .CheckStackDepth(3);
    }

    {
        Context ctx("SigVersion must not be WITNESS_V0");
        ctx.SetVersion(SigVersion::WITNESS_V0).SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_BAD_OPCODE)
            .CheckStackDepth(3);
    }

    {
        Context ctx("Minimum stack height 3 for OP_CHECKSIGADD");
        ctx.SetScript()
            << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_INVALID_STACK_OPERATION)
            .CheckStackDepth(2);
    }

    {
        Context ctx("`n` (2nd arg) size > 4 must fail");
        // This is probably meant to be a check on the _encoding_ - that it is
        // minimal, but it can also be a check on the _value_.  BIP342 doesn't
        // say which.  Could be both...
        ctx.SetScript()
            << SIG_EMPTY << CScriptNum(10000000000LL) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            // (IMO this is an _unsatisfactory_ error code to return for a required
            // BIP342 check, but see the `catch` clause in `EvalScript`)
            .CheckCallFailed(SCRIPT_ERR_UNKNOWN_ERROR)
            .CheckStackDepth(3);
    }

    {
        Context ctx("Empty sig + empty pubkey");
        ctx.SetScript()
            << SIG_EMPTY << CScriptNum(TEST_NUM) << PUBKEY_EMPTY << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_PUBKEYTYPE)
            .CheckStackDepth(3);
    }

    {
        Context ctx("Sig + empty pubkey");
        ctx.SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_EMPTY << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_PUBKEYTYPE)
            .CheckStackDepth(3);
    }

    {
        Context ctx("Insufficient validation weight remaining");
        ctx.SetRemainingWeight(BIP342_SIGOPS_LIMIT - 1)
                .SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT)
            .CheckStackDepth(3);
    }

    {
        Context ctx("Empty sig + 32byte pubkey skips validation");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
                .SetScript()
            << SIG_EMPTY << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureWasNotValidated()
            .CheckRemainingValidationWeight(START_VALIDATION_WEIGHT)
            .CheckStackDepth(1)
            .CheckTOS(TEST_NUM);
    }

    {
        Context ctx("Empty sig + non32byte pubkey skips validation");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
                .SetScript()
            << SIG_EMPTY << CScriptNum(TEST_NUM) << PUBKEY_15BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureWasNotValidated()
            .CheckRemainingValidationWeight(START_VALIDATION_WEIGHT)
            .CheckStackDepth(1)
            .CheckTOS(TEST_NUM);
    }

    {
        Context ctx("non32byte pubkey ('unknown pubkey type') _with_ discourage flag fails");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
                .AddFlags(SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE)
                .SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_15BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE)
            .CheckSignatureWasNotValidated()
            .CheckStackDepth(3);
    }

    {
        Context ctx("32byte pubkey + sig with validation failure forced");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_FAILS)
                .SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_SCHNORR_SIG)
            .CheckSignatureWasValidated()
            .CheckStackDepth(3);
    }

    {
        Context ctx("32byte pubkey + sig with validation success forced");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
                .SetScript()
            << SIG_64BYTES << CScriptNum(TEST_NUM) << PUBKEY_32BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureWasValidated()
            .CheckRemainingValidationWeight(END_VALIDATION_WEIGHT)
            .CheckStackDepth(1)
            .CheckTOS(TEST_NUM + 1);
    }

    {
        Context ctx("non32byte pubkey + empty sig with validation success forced");
        ctx.SetChecker(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
                .SetScript()
            << SIG_EMPTY << CScriptNum(TEST_NUM) << PUBKEY_15BYTES << OP_CHECKSIGADD;
        ctx.DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureWasNotValidated()
            .CheckRemainingValidationWeight(START_VALIDATION_WEIGHT)
            .CheckStackDepth(1)
            .CheckTOS(TEST_NUM);
    }
}

BOOST_AUTO_TEST_CASE(signature_hash_schnorr_failure_cases)
{
    // As defined by BIP-341 Signature Validation Rules
    // Here we pick an acceptable SigVersion
    const SigVersion sigversion = SigVersion::TAPROOT;

    CMutableTransaction tx_to_m;
    tx_to_m.vin.push_back(CTxIn());
    const uint32_t in_pos{0};

    PrecomputedTransactionData cache;
    cache.m_bip341_taproot_ready = true;
    cache.m_spent_outputs_ready = true;

    ScriptExecutionData execdata;
    execdata.m_annex_init = true;
    execdata.m_annex_present = false;
    execdata.m_annex_hash = uint256::ZERO;
    execdata.m_tapleaf_hash_init = false;
    execdata.m_codeseparator_pos_init = true;

    uint256 hash_out{0};

    {
        // Check all invalid hash_type codes rejected
        const std::set<uint8_t> allowable_hash_types{0x00, 0x01, 0x02, 0x03, 0x81, 0x82, 0x83};
        for (unsigned ht = 0; ht <= 255; ht++) {
            const uint8_t hash_type = static_cast<uint8_t>(ht);
            if (allowable_hash_types.find(hash_type) != allowable_hash_types.end()) continue;

            BOOST_CHECK_MESSAGE(!SignatureHashSchnorr(hash_out, execdata, tx_to_m, in_pos,
                                                      hash_type, sigversion, cache,
                                                      MissingDataBehavior::FAIL),
                                "hash_type = " << Hex(hash_type) << " expected to fail");
        }
    }

    {
        // Check that if hash_type == SIGHASH_SINGLE then missing a "corresponding
        // output" fails.
        CMutableTransaction tx_to_m;
        tx_to_m.vin.push_back(CTxIn());
        tx_to_m.vin.push_back(CTxIn());
        tx_to_m.vin.push_back(CTxIn());

        uint8_t in_pos = 1;
        BOOST_CHECK_MESSAGE(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, SIGHASH_SINGLE, sigversion, cache,
                                                  MissingDataBehavior::FAIL),
                            "SIGHASH_SINGLE with in_pos(1) > #tx_to==0 is expected to fail");

        tx_to_m.vout.push_back(CTxOut());
        in_pos = 2;
        BOOST_CHECK_MESSAGE(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, SIGHASH_SINGLE, sigversion, cache,
                                                  MissingDataBehavior::FAIL),
                            "SIGHASH_SINGLE with in_pos(2) > #tx_to==1 is expected to fail");
    }
}

BOOST_AUTO_TEST_CASE(signature_hash_schnorr_all_success_paths)
{
    // Our approach here will be to follow BIP-341's signature algorithm (with
    // the BIP-342 extension) doing two things at once:
    //   1) We'll set up the input arguments to `SignatureHashSchnorr` function
    //      being tested, _and_
    //   2) we'll _compute the hash of those fields ourselves_ exaxctly as
    //      it is described in BIP-341 and BIP-342.
    // Then we can compare the two.  We'll do this in a data-driven way for each
    // of the different scenarios that the algorithm supports.
    //
    // In this way this test achieves 100% _path_ coverage of `SignatureHashSchnorr`
    // (not just 100% _branch_ coverage).
    // - Sadly, this isn't shown in the `lcov` reports.  There are still a few
    //   red `-` marks left.  This is because:
    //   1. `lcov` wasn't designed to handle death tests.
    //   2. ??? Some other unknown reasons, possibly due to the instrumentation,
    //      possibly due to `lcov` limitations.  You can see by the test output
    //      (`-log_level=all`) or within a debugger that in fact _all_ branches
    //      are taken when executing all the tests in this file.

    // Here we define, and then generate, all combinations of the alternatives
    // for the parameters that vary the signature combination algorithm

    const vn_sequence<SigVersion> SigVersion_alternatives{
        {SigVersion::TAPROOT, "TAPROOT"sv},
        {SigVersion::TAPSCRIPT, "TAPSCRIPT"sv}};

    const vn_sequence<uint32_t> hash_type_output_alternatives{
        {SIGHASH_DEFAULT, "SIGHASH_DEFAULT"sv},
        {SIGHASH_ALL, "SIGHASH_ALL"sv},
        {SIGHASH_NONE, "SIGHASH_NONE"sv},
        {SIGHASH_SINGLE, "SIGHASH_SINGLE"sv}};

    const vn_sequence<uint32_t> hash_type_input_alternatives{
        {0, "N/A"sv},
        {SIGHASH_ANYONECANPAY, "SIGHASH_ANYONECANPAY"sv}};

    const vn_sequence<uint8_t> annex_alternatives{
        {0, "no annex"sv},
        {1, "annex present"sv}};

    const vn_sequence<bool> output_hash_alternatives{
        {false, "output hash missing"sv},
        {true, "output hash provided"sv}};

    for (const auto& sigversion_alternative : SigVersion_alternatives)
        for (const auto& hash_type_output_alternative : hash_type_output_alternatives)
            for (const auto& hash_type_input_alternative : hash_type_input_alternatives)
                for (const auto& annex_alternative : annex_alternatives)
                    for (const auto& output_hash_alternative : output_hash_alternatives) {
                        // Exclude the invalid combination of SIGHASH_DEFAULT with SIGHASH_ANYONECANPAY
                        if (hash_type_output_alternative.m_value == SIGHASH_DEFAULT && hash_type_input_alternative.m_value == SIGHASH_ANYONECANPAY) continue;

                        // We're going to want to know which scenario it is if a check actually
                        // fails ...
                        std::string scenario_description;
                        {
                            std::ostringstream oss;
                            oss << sigversion_alternative.m_name << ", "
                                << hash_type_output_alternative.m_name << ", "
                                << hash_type_input_alternative.m_name << ", "
                                << annex_alternative.m_name << ", "
                                << output_hash_alternative.m_name;
                            scenario_description = oss.str();
                        }

                        // Set up the scenario we're running now - these 4 variables define the scenario
                        const SigVersion sigversion{sigversion_alternative.m_value};
                        const uint8_t hash_type{static_cast<uint8_t>(hash_type_output_alternative.m_value | hash_type_input_alternative.m_value)};
                        const uint8_t annex_present{annex_alternative.m_value};
                        const bool have_output_hash{output_hash_alternative.m_value};

                        // Compute some helper values that depend on scenario
                        const uint8_t ext_flag{sigversion == SigVersion::TAPSCRIPT};
                        const uint8_t hash_input_type{static_cast<uint8_t>(hash_type & SIGHASH_INPUT_MASK)};
                        const uint8_t hash_output_type{static_cast<uint8_t>((hash_type == SIGHASH_DEFAULT) ? SIGHASH_ALL : (hash_type & SIGHASH_OUTPUT_MASK))};
                        const uint8_t spend_type = (ext_flag * 2) + annex_present;

                        // Fixed values (by algorithm)
                        const uint8_t epoch{0x00};
                        const uint8_t key_version{0};

                        // Mocked values fixed for purposes of this unit test.  This is a long
                        // list of crufty things but that's because `SignatureHashSchnorr`, the
                        // function being tested, takes as arguments not just the transaction
                        // being signed (plus control data) but also some _precomputed values_
                        // in two different structs: `PrecomputedTransactionData`, and
                        // `ScriptExecutionData`.  On the one hand this is nice because a lot
                        // of complexity of the signature algorithm doesn't have to be duplicated
                        // here in this test: we can just use mocked values.  On the other hand,
                        // there's a lot of icky setup to do to get all the values in the right
                        // places both for our "by the book" implementation and to be set up to
                        // call `SignatureHashSchnorr`.
                        //
                        // Try to make things simpler by at least using the same names for the
                        // setup variables as for the fields in the parameter structs.

                        const uint32_t in_pos{1};
                        const int32_t tx_version{FillWithPattern(0x01).i32[0]};
                        const uint32_t tx_lock_time{FillWithPattern(0x05).u32[0]};
                        const uint256 prevouts_single_hash{FillWithPattern(0x10).u256};
                        const uint256 spent_amounts_single_hash{FillWithPattern(0x18).u256};
                        const uint256 spent_scripts_single_hash{FillWithPattern(0x20).u256};
                        const uint256 sequences_single_hash{FillWithPattern(0x28).u256};
                        const uint256 outputs_single_hash{FillWithPattern(0x30).u256};
                        const uint256 output_hash{FillWithPattern(0x40).u256};
                        const uint256 annex_hash{FillWithPattern(0x48).u256};
                        const uint256 tapleaf_hash{FillWithPattern(0x50).u256};
                        const uint32_t codeseparator_pos{FillWithPattern(0x58).u32[0]};
                        const COutPoint tx_input_at_pos_prevout{FillWithPattern(0x60).u256,
                                                                FillWithPattern(0x68).u32[0]};
                        const uint32_t tx_input_at_pos_nsequence{FillWithPattern(0x70).u32[0]};
                        CTxOut spent_output_at_pos;
                        spent_output_at_pos.nValue = FillWithPattern(0x80).u64();
                        spent_output_at_pos.scriptPubKey /*random script, not even valid*/
                            << OP_DUP << OP_HASH160 << OP_EQUALVERIFY << OP_CHECKSIG;
                        CTxOut tx_output_at_pos;
                        tx_output_at_pos.nValue = FillWithPattern(0x90).u64();
                        tx_output_at_pos.scriptPubKey /*random script, not even valid*/
                            << OP_CHECKSIG << OP_EQUALVERIFY << OP_HASH160 << OP_DUP;

                        // Now set up the arguments that are going to be passed to
                        // `SignatureHashSchnorr`

                        CMutableTransaction tx_to;
                        tx_to.nVersion = tx_version;
                        tx_to.nLockTime = tx_lock_time;
                        for (uint32_t i = 0; i < in_pos + 2; i++) {
                            tx_to.vin.push_back(CTxIn());
                            tx_to.vout.push_back(CTxOut());
                        }
                        tx_to.vin[in_pos].prevout = tx_input_at_pos_prevout;
                        tx_to.vin[in_pos].nSequence = tx_input_at_pos_nsequence;
                        tx_to.vout[in_pos] = tx_output_at_pos;

                        PrecomputedTransactionData cache;
                        cache.m_bip341_taproot_ready = true;
                        cache.m_prevouts_single_hash = prevouts_single_hash;
                        cache.m_spent_amounts_single_hash = spent_amounts_single_hash;
                        cache.m_spent_scripts_single_hash = spent_scripts_single_hash;
                        cache.m_sequences_single_hash = sequences_single_hash;
                        cache.m_spent_outputs_ready = true;
                        for (uint32_t i = 0; i < in_pos + 2; i++) {
                            cache.m_spent_outputs.push_back(CTxOut());
                        }
                        cache.m_spent_outputs[in_pos] = spent_output_at_pos;
                        cache.m_outputs_single_hash = outputs_single_hash;

                        ScriptExecutionData execdata;
                        execdata.m_annex_init = true;
                        execdata.m_annex_present = !!annex_present;
                        execdata.m_annex_hash = annex_hash;
                        execdata.m_output_hash.reset();
                        if (have_output_hash) {
                            execdata.m_output_hash = output_hash;
                        }
                        if (sigversion == SigVersion::TAPSCRIPT) {
                            execdata.m_tapleaf_hash_init = true;
                            execdata.m_tapleaf_hash = tapleaf_hash;
                            execdata.m_codeseparator_pos_init = true;
                            execdata.m_codeseparator_pos = codeseparator_pos;
                        }

                        // Now here is where we take all that data - _not_ the arguments to
                        // `SignatureHashSchnorr` but all the scenario parameters, the helpers,
                        // the values fixed by the algorithm, and our mocked values, and actually
                        // follow the BIP-341/BIP-342 signature calculation algorithm right from
                        // the spec ...

                        // Start with a tagged hasher with the correct tag
                        CHashWriter hasher = TaggedHash("TapSighash");

                        // First byte to hash is always the "epoch", 0x00 (BIP-341, footnote 20)
                        hasher << epoch;

                        // Next: hash_type (1 byte)
                        hasher << hash_type;

                        // Next: transaction version (4 bytes)
                        hasher << tx_version;

                        // Next: transaction lock time (4 bytes)
                        hasher << tx_lock_time;

                        // Next if _not_ SIGHASH_ANYONECANPAY:
                        // a) SHA256 of the serialization of all input outpoints (32 bytes)
                        // b) SHA256 of the serialization of all spent output amounts (32 bytes)
                        // c) SHA256 of the serialization of all spent outputs' _scriptPubKeys_
                        //    serialized as script (32 bytes)
                        // d) SHA256 of the serialization of all input `nSequence` (32 bytes)
                        if (hash_input_type != SIGHASH_ANYONECANPAY) {
                            hasher << prevouts_single_hash;
                            hasher << spent_amounts_single_hash;
                            hasher << spent_scripts_single_hash;
                            hasher << sequences_single_hash;
                        }

                        // Next if _not_ SIGHASH_NONE _and not_ SIGHASH_SINGLE:
                        // SHA256 of the serialization of all outputs in CTxOut format (32 bytes)
                        if (hash_output_type != SIGHASH_NONE && hash_output_type != SIGHASH_SINGLE) {
                            hasher << outputs_single_hash;
                        }

                        // Now, data about input/prevout being spent

                        // The "spend_type" (1 byte) which is a function of ext_flag (above) and
                        // whether there is an annex present (here: no)
                        hasher << spend_type;

                        // Here, if we are _not_ SIGHASH_ANYONECANPAY, we just add the index of
                        // the input in the transaction input vector (4 bytes). There must be a
                        // input transaction at this index but _in this scenario_ it doesn't have
                        // to have any data (it is never inspected).  Same for output transactions.
                        //
                        // On the other hand, if we _are_ SIGHASH_ANYONECANPAY, then we add the
                        // `COutPoint` of this input (36 bytes), the value of the previous
                        // output spent by this input (8 bytes), the `ScriptPubKey` of the
                        // previous output spent by this input (35 bytes), and the `nSequence`
                        // of this input.  These values are all precomputed and made available
                        // to `SignatureHashSchnorr` in the `PrecomputedTransactionData` struct.
                        if (hash_input_type == SIGHASH_ANYONECANPAY) {
                            hasher << tx_input_at_pos_prevout;
                            hasher << spent_output_at_pos.nValue;
                            hasher << spent_output_at_pos.scriptPubKey;
                            hasher << tx_input_at_pos_nsequence;
                        } else {
                            hasher << in_pos;
                        }

                        // Now, if there is an "annex", add its hash (32 byte).  This is
                        // precomputed and we don't actually have to have an actual annex to
                        // pass in to `SignatureHashSchnorr`, nor do we have to hash it.
                        if (annex_present) {
                            hasher << annex_hash;
                        }

                        // Here, iff the hash type is `SIGHASH_SINGLE`, add the hash of the
                        // corresponding transaction output (32 bytes).  The wrinkle here is that
                        // (for some reason) _sometimes_ this hash is precomputed, and _sometimes_
                        // it is _not_.  So `SignatureHashSchnorr` will either use it if it is
                        // provided or compute it from the corresponding output itself. (For our
                        // purposes in this test the output need not be valid - it just must be
                        // present.)
                        if (hash_output_type == SIGHASH_SINGLE) {
                            if (!have_output_hash) {
                                CHashWriter hasher2(SER_GETHASH, 0);
                                hasher2 << tx_output_at_pos;
                                hasher << hasher2.GetSHA256();
                            } else {
                                hasher << output_hash;
                            }
                        }

                        // This is the TAPSCRIPT extension from BIP-342.  If the version is
                        // TAPSCRIPT then add the tapleaf hash (32 bytes), the key_version (1
                        // byte, fixed value of 0x00), and the "opcode position of the last
                        // executed OP_CODESEPARATOR before the currently executed signature
                        // opcode" (4 bytes).  The tapleaf hash and the code separator position
                        // are both precomputed values.
                        if (sigversion == SigVersion::TAPSCRIPT) {
                            hasher << tapleaf_hash;
                            hasher << key_version;
                            hasher << codeseparator_pos;
                        }

                        // That's all that goes into the hasher for this signature
                        const uint256 expected_hash_out = hasher.GetSHA256();

                        // Now, _finally_, we test the actual implemented algorithm under test:
                        uint256 actual_hash_out{0};
                        BOOST_TEST(SignatureHashSchnorr(actual_hash_out,
                                                        execdata, tx_to, in_pos,
                                                        hash_type, sigversion, cache,
                                                        MissingDataBehavior::FAIL),
                                   "Scenario: " << scenario_description);
                        BOOST_TEST(expected_hash_out == actual_hash_out,
                                   "Scenario: " << scenario_description
                                                << " - expected " << expected_hash_out.ToString()
                                                << " == actual " << actual_hash_out.ToString());
                    }
}

namespace {

// Valid Schnoor (pubkey, msg, signature) tuples (copied from `key_tests.cpp`)

struct SchnorrTriplet {
    SchnorrTriplet(std::string pubkey, std::string sighash, std::string sig)
        : m_pubkey(ParseHex(pubkey)), m_sighash(uint256(ParseHex(sighash))), m_sig(ParseHex(sig)) {}
    valtype m_pubkey;
    uint256 m_sighash;
    valtype m_sig;
};

static const std::vector<SchnorrTriplet> SCHNORR_TRIPLETS = {
    {"F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9", "0000000000000000000000000000000000000000000000000000000000000000", "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0"},
    {"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A"},
    {"DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8", "7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C", "5831AAEED7B44BB74E5EAB94BA9D4294C49BCF2A60728D8B4C200F50DD313C1BAB745879A5AD954A72C45A91C3A51D3C7ADEA98D82F8481E0E1E03674A6F3FB7"},
    {"25D1DFF95105F5253C4022F628A996AD3A0D95FBF21D468A1B33F8C160D8F517", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "7EB0509757E246F19449885651611CB965ECC1A187DD51B64FDA1EDC9637D5EC97582B9CB13DB3933705B32BA982AF5AF25FD78881EBB32771FC5922EFC66EA3"},
    {"D69C3509BB99E412E68B0FE8544E72837DFA30746D8BE2AA65975F29D22DC7B9", "4DF3C3F68FCC83B27E9D42C90431A72499F17875C81A599B566C9889B9696703", "00000000000000000000003B78CE563F89A0ED9414F5AA28AD0D96D6795F9C6376AFB1548AF603B3EB45C9F8207DEE1060CB71C04E80F593060B07D28308D7F4"},
};

} // namespace

BOOST_AUTO_TEST_CASE(internal_test_validate_schnorr_testdata)
{
    for (const auto& triplet : SCHNORR_TRIPLETS) {
        BOOST_TEST(XOnlyPubKey(triplet.m_pubkey).VerifySchnorr(triplet.m_sighash, triplet.m_sig));
    }
}

BOOST_AUTO_TEST_CASE(verify_schnorr_signature)
{
    // Defeat, for test purposes, the protected access of
    // `GenericTransactionSignatureChecker::VerifySchnorrSignature`
    struct UnprotectedTransactionSignatureChecker : public MutableTransactionSignatureChecker {
        using MutableTransactionSignatureChecker::MutableTransactionSignatureChecker;
        using MutableTransactionSignatureChecker::VerifySchnorrSignature;
    };
    UnprotectedTransactionSignatureChecker sut{nullptr, 0, {}, {}};

    // Positive tests: triplets which verify
    for (const auto& triplet : SCHNORR_TRIPLETS) {
        BOOST_TEST(sut.VerifySchnorrSignature(triplet.m_sig,
                                              XOnlyPubKey{triplet.m_pubkey},
                                              triplet.m_sighash));
    }

    // Negative tests: triplets which fail to verify (get these failing triplets
    // by modifying a valid triplet, one field at a time)
    auto diddle_front_byte = [](auto v) { v[0]++; return v; };
    auto& triplet = SCHNORR_TRIPLETS[0];
    BOOST_TEST(!sut.VerifySchnorrSignature(diddle_front_byte(triplet.m_sig),
                                           XOnlyPubKey{triplet.m_pubkey},
                                           triplet.m_sighash));
    BOOST_TEST(!sut.VerifySchnorrSignature(triplet.m_sig,
                                           XOnlyPubKey{diddle_front_byte(triplet.m_pubkey)},
                                           triplet.m_sighash));
    BOOST_TEST(!sut.VerifySchnorrSignature(triplet.m_sig,
                                           XOnlyPubKey{triplet.m_pubkey},
                                           uint256::ONE));
}

BOOST_AUTO_TEST_CASE(check_schnorr_signature)
{
    // Provide, for test purposes, a subclass of `GenericTransactionsSignatureChecker`
    // that mocks `VerifySchnorrSignature` so we can more easily test
    // `CheckSchnorrSignature` without going to the trouble of having a valid
    // transaction (which is unnecessary for this _unit_ test.)
    struct MockVerifyingTransactionSignatureChecker : public MutableTransactionSignatureChecker {
        uint256 m_expected_sighash = []() {
            uint256 h{};
            // This is the known sighash of the Tx and input data we set up (precomputed)
            h.SetHex("f614d8ae6dcc49e2ca2ef1c03f93c7326189e5575d446e825e5a2700fb1cb83c");
            return h;
        }();

        using MutableTransactionSignatureChecker::MutableTransactionSignatureChecker;

        enum class if_as_expected_return { False,
                                           True };
        if_as_expected_return m_iae{if_as_expected_return::True};
        void SetExpectation(if_as_expected_return iaer) { m_iae = iaer; }

        bool VerifySchnorrSignature(Span<const unsigned char> sig,
                                    const XOnlyPubKey& pubkey,
                                    const uint256& sighash) const override
        {
            // Following line used only to determine the known canned `expected_sighash` above:
            // BOOST_TEST_MESSAGE("MockVerifySchnorrSignature: sighash == " << sighash.ToString());

            bool as_expected = sighash == m_expected_sighash;
            if (m_iae == if_as_expected_return::True)
                return as_expected;
            else
                return !as_expected;
        };
    };

    const auto triplet = SCHNORR_TRIPLETS[0];
    const CMutableTransaction txToIn{};
    ScriptExecutionData execdata{};

    {
        // Signature must be 64 or 65 bytes long
        for (size_t i = 0; i <= 99; i++) {
            valtype testsig(i, i);
            if (testsig.size() == 64 || testsig.size() == 65) continue;
            MockVerifyingTransactionSignatureChecker sut(&txToIn, 0, {}, MissingDataBehavior::FAIL);
            ScriptError serror{SCRIPT_ERR_OK};
            BOOST_TEST(!sut.CheckSchnorrSignature(testsig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
            BOOST_TEST(serror == SCRIPT_ERR_SCHNORR_SIG_SIZE);
        }
    }

    {
        // Iff signature is 65 bytes long last byte must **NOT** be SIGHASH_DEFAULT (0x00) per BIP-342
        {
            // Negative test: last byte _is_ SIGHASH_DEFAULT
            valtype testsig(65, 65);
            testsig.back() = SIGHASH_DEFAULT;

            MockVerifyingTransactionSignatureChecker sut(&txToIn, 0, {}, MissingDataBehavior::FAIL);
            ScriptError serror{SCRIPT_ERR_OK};
            BOOST_TEST(!sut.CheckSchnorrSignature(testsig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
            BOOST_TEST(serror == SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
        }
        {
            // Negative tests: last byte is _not_ SIGHASH_DEFAULT, but we early exit _without changing
            // serror_ because we don't provide a txDataIn (🡄 this requires knowledge of how
            // `CheckSchnorrSignature` is written).
            for (size_t i = 1; i <= 255; i++) {
                valtype testsig(65, i);

                MockVerifyingTransactionSignatureChecker sut(&txToIn, 0, {}, MissingDataBehavior::FAIL);
                ScriptError serror{SCRIPT_ERR_OK};
                BOOST_TEST(!sut.CheckSchnorrSignature(testsig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
                BOOST_TEST(serror == SCRIPT_ERR_OK);
            }
        }
    }

    {
        // Now check that, given the parameters, if `SignatureHashSchnorr fails there's an error exit.
        // Otherwise, if it succeeds, it proceeds to call `VerifySchnorrSignature` and depending on
        // _that_ result `SignatureHashSchnorr` either succeeds or fails.
        //
        // We do this using the mocked `VerifySchnorrSignature` so we only need to pass parameters
        // that work with `SignatureHashSchnorr`, they don't _also_ have to validate with
        // `VerifySchnorrSignature`.

        const uint32_t in_pos{0};
        CMutableTransaction txToIn{};
        txToIn.nVersion = 0;
        txToIn.nLockTime = 0;
        txToIn.vin.push_back(CTxIn());
        txToIn.vin[in_pos].prevout = COutPoint(uint256::ZERO, 0);
        txToIn.vin[in_pos].nSequence = 0;
        txToIn.vout.push_back(CTxOut());

        PrecomputedTransactionData txDataIn{};
        txDataIn.m_bip341_taproot_ready = true;
        txDataIn.m_prevouts_single_hash = uint256::ZERO;
        txDataIn.m_spent_amounts_single_hash = uint256::ZERO;
        txDataIn.m_spent_scripts_single_hash = uint256::ZERO;
        txDataIn.m_sequences_single_hash = uint256::ZERO;
        txDataIn.m_spent_outputs_ready = true;
        txDataIn.m_spent_outputs.push_back(CTxOut());
        txDataIn.m_spent_outputs[in_pos].nValue = 0;
        txDataIn.m_spent_outputs[in_pos].scriptPubKey << OP_DUP << OP_CHECKSIG;
        txDataIn.m_outputs_single_hash = uint256::ZERO;

        ScriptExecutionData execdata{};
        execdata.m_annex_init = true;
        execdata.m_annex_present = true;
        execdata.m_annex_hash = uint256::ZERO;
        execdata.m_output_hash.reset();

        {
            // Confirm that we can force `SignatureHashSchnorr` to fail (via an early exit)
            PrecomputedTransactionData txDataIn{};
            MockVerifyingTransactionSignatureChecker sut(&txToIn, in_pos, {}, txDataIn, MissingDataBehavior::FAIL);
            ScriptError serror{SCRIPT_ERR_OK};
            BOOST_TEST(!sut.CheckSchnorrSignature(triplet.m_sig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
            BOOST_TEST(serror == SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
        }

        {
            // Now `SignatureHashSchnorr` will return true but we'll fail `VerifySchnorrSignature`
            // and show it returns the correct error.
            MockVerifyingTransactionSignatureChecker sut(&txToIn, in_pos, {}, txDataIn, MissingDataBehavior::FAIL);
            sut.SetExpectation(MockVerifyingTransactionSignatureChecker::if_as_expected_return::False);
            ScriptError serror{SCRIPT_ERR_OK};
            BOOST_TEST(!sut.CheckSchnorrSignature(triplet.m_sig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
            BOOST_TEST(serror == SCRIPT_ERR_SCHNORR_SIG);
        }

        {
            // Finally, same as previous, except we'll force `VerifySchnorrSignature` to succeed and
            // show now that `CheckSchnorrSignature` finally succeeds.
            MockVerifyingTransactionSignatureChecker sut(&txToIn, in_pos, {}, txDataIn, MissingDataBehavior::FAIL);
            sut.SetExpectation(MockVerifyingTransactionSignatureChecker::if_as_expected_return::True);
            ScriptError serror{SCRIPT_ERR_OK};
            BOOST_TEST(sut.CheckSchnorrSignature(triplet.m_sig, triplet.m_pubkey, SigVersion::TAPROOT, execdata, &serror));
            BOOST_TEST(serror == SCRIPT_ERR_OK);
        }
    }
}

BOOST_AUTO_TEST_CASE(compute_tapleaf_hash)
{
    // Try two examples, reimplementing the BIP-341 specification
    {
        uint8_t leaf_version = 0;
        CScript cs{};
        auto expected = (TaggedHash("TapLeaf") << leaf_version << CScript()).GetSHA256();
        auto actual = ComputeTapleafHash(leaf_version, CScript());
        BOOST_TEST(expected == actual,
                   "leaf version 0, empty CScript - expected "
                       << expected.ToString() << " actual " << actual.ToString());
    }

    {
        uint8_t leaf_version = 0x4a;
        CScript cs{};
        cs << OP_CHECKLOCKTIMEVERIFY << OP_CHECKSIGADD; // just a random CScript
        auto expected = (TaggedHash("TapLeaf") << leaf_version << cs).GetSHA256();
        auto actual = ComputeTapleafHash(leaf_version, cs);
        BOOST_TEST(expected == actual,
                   "leaf version 0x4A, CScript w/ 2 opcodes - expected "
                       << expected.ToString() << " actual " << actual.ToString());
    }
}

BOOST_AUTO_TEST_CASE(compute_taproot_merkle_root)
{
    using namespace test::util::vector_ops;

    // Test by using a small enhancement to a `vector<unsigned char>` that makes
    // it easy to convert to/from strings so the tests are more easily readable,
    // and also adds directly the two necessary operations from BIP-340: byte
    // vector concatenation and byte vector select subrange.

    // Use an arbitrary tapleaf hash throughout
    const uint256 tapleaf_hash1 = ComputeTapleafHash(0x10, CScript{} << OP_CHECKMULTISIG);
    const uint256 tapleaf_hash2 = ComputeTapleafHash(0x20, CScript{} << OP_CHECKSEQUENCEVERIFY);

    //                         ".........|.........|.........|..."      33 bytes
    const auto control_base1 = "[point (#1) - 33 bytes of junk!!>"_bv;
    const auto control_base2 = "[point (#2) - 33 more bad bytes!>"_bv;
    assert(control_base1.size() == 33 && control_base2.size() == 33);

    // Nodes `node_low` and `node_high` are constructed to be _forced_ lower/higher
    // (respectively) than arbitrary hash.  This isn't exactly true, of course:
    // only the _first byte_ of these nodes are low or high.  If the first byte
    // of the "arbitrary" hash is `0x00` or `0xff` we've got a problem .. but
    // this isn't the case for this test data.

    //                                     ".........|.........|.........|.."       32 bytes
    const auto node_low = []() { auto r  = "(this is node to-be-diddled low)"_bv; r.front() = 0x00; return r; }();
    const auto node_high = []() { auto r = "(this is nod to-be-diddled high)"_bv; r.front() = 0xFF; return r; }();
    assert(node_low.size() == 32 && node_high.size() == 32);

    assert(node_low < from_base_blob(tapleaf_hash1) && from_base_blob(tapleaf_hash1) < node_high);
    assert(node_low < from_base_blob(tapleaf_hash2) && from_base_blob(tapleaf_hash2) < node_high);

    const CHashWriter hw_branch{TaggedHash("TapBranch")};

    {
        // Control block contains only the initial point, no nodes - always returns
        // the tapleaf hash, doesn't matter what the control block is
        uint256 expected1 = tapleaf_hash1;
        uint256 actual1a = ComputeTaprootMerkleRoot(control_base1, tapleaf_hash1);
        BOOST_TEST(expected1 == actual1a,
                   "expected " << HexStr(expected1) << ", actual " << HexStr(actual1a));
        uint256 actual1b = ComputeTaprootMerkleRoot(control_base2, tapleaf_hash1);
        BOOST_TEST(expected1 == actual1b,
                   "expected " << HexStr(expected1) << ", actual " << HexStr(actual1b));
        uint256 expected2 = tapleaf_hash2;
        uint256 actual2 = ComputeTaprootMerkleRoot(control_base2, tapleaf_hash2);
        BOOST_TEST(expected2 == actual2,
                   "expected " << HexStr(expected2) << ", actual " << HexStr(actual2));
    }

    {
        // Control block contains one node - check both lexicographic orders

        {
            uint256 expected = (CHashWriter{hw_branch} << Span{node_low} << tapleaf_hash1).GetSHA256();
            uint256 actual = ComputeTaprootMerkleRoot(Span{control_base1 || node_low}, tapleaf_hash1);
            BOOST_TEST(expected == actual,
                    "expected " << HexStr(expected) << ", actual " << HexStr(actual));
        }
        {
            uint256 expected = (CHashWriter{hw_branch} << tapleaf_hash1 << Span{node_high}).GetSHA256();
            uint256 actual = ComputeTaprootMerkleRoot(Span{control_base1 || node_high}, tapleaf_hash1);
            BOOST_TEST(expected == actual,
                    "expected " << HexStr(expected) << ", actual " << HexStr(actual));
        }
    }

    {
        // With a control block with more than one node (here: two nodes), each subsequent node
        // is hashed with the hash of the previous nodes in _lexicographic_ order.

        // Control block is going to be `point1 || node_high || node_{low,high}`
        uint256 intermediate_k = (CHashWriter{hw_branch} << tapleaf_hash1 << Span{node_high}).GetSHA256();

        // Verify that the intermediate hash is less than `node_high`
        assert(from_base_blob(intermediate_k) < node_high);

        {
            // 2nd node lexicographically _less than_ intermediate hash
            uint256 expected = (CHashWriter{hw_branch} << Span{node_low} << intermediate_k).GetSHA256();
            uint256 actual = ComputeTaprootMerkleRoot(control_base1 || node_high || node_low, tapleaf_hash1);
            BOOST_TEST(expected == actual,
                    "expected " << HexStr(expected) << ", actual " << HexStr(actual));
        }

        {
            // 2nd node lexicographically _greater than_ intermediate hash
            uint256 expected = (CHashWriter{hw_branch} << intermediate_k << Span{node_high}).GetSHA256();
            uint256 actual = ComputeTaprootMerkleRoot(control_base1 || node_high || node_high, tapleaf_hash1);
            BOOST_TEST(expected == actual,
                    "expected " << HexStr(expected) << ", actual " << HexStr(actual));
        }
    }
}

BOOST_AUTO_TEST_CASE(taproot_v1_verify_script)
{
    // Testing Taproot code paths in `VerifyWitnessProgram` and
    // `SigVersion::TAPSCRIPT` code paths in `ExecuteWitnessScript`.

    // Both `VerifyWitnessProgram` and `ExecuteWitnessScript` are `static`
    // inside of `interpreter.cpp` and thus inaccessible to a unit test.
    // The way to get to them is indirectly via `VerifyScript`.

    // Tests all success and failure paths mentioned in BIP-341 and
    // BIP-342.

    // This is a _unit test_ not a _functional test_ and the unit being
    // tested here does _not_ include actually verifying the signature.
    // That is tested elsewhere (e.g., by tests `verify_schnorr_signature`
    // and `check_schnorr_signature` in this file).  _This_ test _mocks_
    // the Schnorr signature verfication.  Thus the test data need not
    // actually have valid signatures (and is thus easier to prepare).

    /**
     * A fluent API for running these tests.  Swiped and adapted from
     * `script_tests.cpp`.
     *
     * (Easiest way to understand this class is to look at the actual tests
     * that follow in this function.)
     */

    struct Context {
        // raw key data from `key_tests.cpp` @305
        valtype m_sec{ParseHex("0000000000000000000000000000000000000000000000000000000000000003")};
        valtype m_pub{ParseHex("F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9")};
        valtype m_sig{ParseHex("E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0")};

        CKey m_sec_key;
        XOnlyPubKey m_pub_key;

    private:
        void SetupKeys()
        {
            BOOST_TEST(m_sec.size() == 32);
            BOOST_TEST(m_pub.size() == 32);
            BOOST_TEST(m_sig.size() == 64);

            m_sec_key.Set(m_sec.begin(), m_sec.end(), true /*compressed*/);
            m_pub_key = XOnlyPubKey(m_sec_key.GetPubKey());

            BOOST_TEST(m_pub_key.IsFullyValid());
        }

    public:
        explicit Context(std::string_view descr) : m_test_description(descr)
        {
            SetupKeys();

            // For Taproot v1 force SegWit version 1
            m_scriptPubKey << OP_1;
        }

        const std::string m_test_description;

        bool m_p2sh_wrapped = false;
        CScript m_scriptPubKey;
        unsigned int m_hash_type = SIGHASH_DEFAULT;
        valtype m_annex;
        std::vector<valtype> m_initial_witness_stack;
        valtype m_witness_signature;
        bool m_witness_init = false;
        CScriptWitness m_witness;
        CScript m_tapscript;
        bool m_control_block_init = false;
        valtype m_control_block;
        unsigned int m_leaf_version = 0;
        unsigned int m_pubkey_parity = 0;
        valtype m_taproot_internal_key;
        valtype m_control_block_field;
        unsigned int m_flags = 0;
        CHECKER_VALIDATION m_checker_validation = CHECKER_VALIDATION::ALWAYS_FAILS;

        int64_t m_caller_line = 0;
        bool m_result = false;
        ScriptError m_serror = SCRIPT_ERR_OK;
        bool m_checker_was_called = false;

        //
        // N.B.: Some methods herein temporarily marked [[maybe_unused]]
        // until the Tapscript tests get written
        //

        Context& SetValidPublicKey()
        {
            m_scriptPubKey << m_pub;
            return *this;
        }

        [[maybe_unused]] Context& SetPublicKey(valtype key)
        {
            BOOST_TEST(key.size() == 32);
            m_scriptPubKey << key;
            return *this;
        }

        Context& SetNBytePublicKey(size_t n)
        {
            valtype pub(n, 0xAB);
            m_scriptPubKey << pub;
            return *this;
        }

        Context& SetSignatureAnnex(const valtype& annex_without_suffix)
        {
            m_annex.push_back(0x50); // by definition of annex
            m_annex.insert(m_annex.end(), annex_without_suffix.begin(), annex_without_suffix.end());
            return *this;
        }

        // Used to directly set a witness (presumably, invalid for Taproot key path spending)
        Context& SetWitness(const std::vector<valtype>& witness)
        {
            m_witness_init = true;
            m_witness.stack = witness;
            return *this;
        }

        [[maybe_unused]] Context& PushToWitnessStack(const valtype& v)
        {
            m_initial_witness_stack.push_back(v);
            return *this;
        }

        Context& SetValidSignatureInWitness(unsigned char hash_type = SIGHASH_DEFAULT)
        {
            m_hash_type = hash_type;
            m_witness_signature = m_sig;
            if (hash_type) m_witness_signature.push_back(hash_type);

            BOOST_TEST((hash_type ? m_witness_signature.size() == 65 : m_witness_signature.size() == 64));
            return *this;
        }

        // Used to directly set the signature in the witness (presumably, invalid for Taproot key path spending)
        Context& SetSignatureInWitness(const valtype& sig)
        {
            m_hash_type = SIGHASH_DEFAULT;
            m_witness_signature = sig;
            return *this;
        }

        [[maybe_unused]] CScript& SetTapscript()
        {
            return m_tapscript;
        }

        [[maybe_unused]] Context& SetTapscriptLeafVersion()
        {
            m_leaf_version = 0xC0;
            return *this;
        }

        [[maybe_unused]] Context& SetLeafVersion(unsigned int lv, unsigned int pubkey_parity)
        {
            BOOST_TEST(lv < 256);
            BOOST_TEST(pubkey_parity < 2);

            m_leaf_version = lv;
            m_pubkey_parity = pubkey_parity;
            return *this;
        }

        [[maybe_unused]] Context& SetTaprootInternalKey(const valtype& p)
        {
            BOOST_TEST(p.size() == 32);
            m_taproot_internal_key = p;
            return *this;
        }

        [[maybe_unused]] Context& AddControlBlockField(const valtype& f)
        {
            BOOST_TEST(f.size() = 32);
            m_control_block_field.insert(m_control_block_field.end(), f.begin(), f.end());
            return *this;
        }

        [[maybe_unused]] Context& SetControlBlock(const valtype& cb)
        {
            m_control_block_init = true;
            m_control_block = cb;
            return *this;
        }

        // Used to directly set the flags (presumably, not the usual Taproot key path spending flags)
        Context& SetVerifyFlags(unsigned int flags)
        {
            m_flags = flags;
            return *this;
        }

        Context& SetSchnorrSignatureValidation(CHECKER_VALIDATION checker_validation)
        {
            m_checker_validation = checker_validation;
            return *this;
        }

        Context& SetP2SHWrapped()
        {
            m_p2sh_wrapped = true;
            return *this;
        }

        Context& DoTest(int64_t line)
        {
            m_caller_line = line;

            BOOST_TEST_MESSAGE(Descr() << "doing test");

            // Build control block
            bool have_control_block = m_control_block_init;
            valtype control_block(m_control_block);
            if (!have_control_block) {
                BOOST_TEST_MESSAGE("maybe building control block");
                if (m_leaf_version) {
                    BOOST_TEST_MESSAGE("have !=0 leaf version, definitely building control block");
                    have_control_block = true;
                    control_block.push_back(static_cast<unsigned char>(m_leaf_version | m_pubkey_parity));
                    control_block.insert(control_block.end(), m_taproot_internal_key.begin(), m_taproot_internal_key.end());
                    control_block.insert(control_block.end(), m_control_block_field.begin(), m_control_block_field.end());
                    BOOST_TEST_MESSAGE("control block size " << control_block.size());
                }
            }

            // build the witness if necessary
            if (!m_witness_init) {
                if (have_control_block) {
                    // Taproot script path spend
                    for (const auto& elem : m_initial_witness_stack)
                        m_witness.stack.push_back(elem);
                    m_witness.stack.push_back(valtype(m_tapscript.begin(), m_tapscript.end()));
                    m_witness.stack.push_back(control_block);
                } else {
                    // Taproot key path spend
                    if (!m_witness_signature.empty()) m_witness.stack.push_back(m_witness_signature);
                }
                if (!m_annex.empty()) m_witness.stack.push_back(m_annex);
            }

            if (!m_flags) {
                m_flags = SCRIPT_VERIFY_SIGPUSHONLY | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_TAPROOT;
            }

            SignatureCheckerMock checker_mock(m_checker_validation);
            CScript script_sig; // must be empty for actual Taproot
            if (m_p2sh_wrapped) {
                // But BIP-341 allows all SegWit v1 P2SH-wrapped outputs to pass
                valtype fake_hash(20, 0x00);
                script_sig << OP_0 << fake_hash;
            }

            m_result = VerifyScript(script_sig,
                                    m_scriptPubKey,
                                    &m_witness,
                                    m_flags,
                                    checker_mock,
                                    &m_serror);

            m_checker_was_called = checker_mock.CheckerWasCalled();

            return *this;
        }

        Context& CheckCallSucceeded()
        {
            BOOST_CHECK_MESSAGE(m_result,
                                Descr() << ": VerifyScript succeeded, as expected");
            BOOST_CHECK_MESSAGE(m_serror == SCRIPT_ERR_OK,
                                Descr() << ": error code expected OK, actual was "
                                        << ScriptErrorString(m_serror));
            return *this;
        }

        Context& CheckCallFailed(ScriptError expected)
        {
            BOOST_CHECK_MESSAGE(!m_result,
                                Descr() << ": VerifyScript failed, as expected");
            BOOST_CHECK_MESSAGE(m_serror == expected,
                                Descr() << ": Error code expected " << ScriptErrorString(expected)
                                        << ", actual was " << ScriptErrorString(m_serror));
            return *this;
        }

        Context& CheckSignatureCheckerWasCalled()
        {
            BOOST_CHECK_MESSAGE(m_checker_was_called,
                                Descr() << ": Schnoor signature checker was called, as expected");
            return *this;
        }

        Context& CheckSignatureCheckerWasNotCalled()
        {
            BOOST_CHECK_MESSAGE(!m_checker_was_called,
                                Descr() << ": Schnoor signature checker was not called, as expected");
            return *this;
        }

    private:
        std::string Descr()
        {
            std::string descr;
            descr.reserve(m_test_description.size() + 20);
            descr += m_test_description;
            descr += " (@";
            descr += as_string(m_caller_line);
            descr += ")";
            return descr;
        }
    };

    {
        Context ctx("Valid Taproot v1 key path spend, hash_type == default, verifies");
        ctx.SetValidPublicKey()
            .SetValidSignatureInWitness(SIGHASH_DEFAULT)
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
            .DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureCheckerWasCalled();
    }

    {
        Context ctx("Valid Taproot v1 key path spend, hash_type == default, with annex, verifies");
        ctx.SetValidPublicKey()
            .SetValidSignatureInWitness(SIGHASH_DEFAULT)
            .SetSignatureAnnex({0x01, 0x02, 0x03, 0x04, 0xFC, 0xFD, 0xFE, 0xFF})
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_SUCCEEDS)
            .DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureCheckerWasCalled();
    }

    {
        // Taproot v1 but witness program (scriptPubKey push) is NOT 32 bytes exactly: anything goes.
        // Witness programs size ∈ [2,40], per BIP-141
        {
            // Verifies even with arbitrary bad signature
            for (size_t n = 2; n <= 40; ++n){
                if (n == 32) continue;
                Context ctx("Taproot v1 with non-32-byte witness program verifies (bad signature)");
                ctx.SetNBytePublicKey(n)
                    .SetSignatureInWitness({1, 2, 3, 4, 5})
                    .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
                    .DoTest(__LINE__)
                    .CheckCallSucceeded()
                    .CheckSignatureCheckerWasNotCalled();
            }
        }

        {
            // Verifies even if witness blows out stack
            const valtype dummy_stack_element{0x00, 0x01, 0x02, 0x03, 0x04};
            std::vector<valtype> ginormous_witness(1100, dummy_stack_element);
            for (size_t n = 2; n <= 40; ++n) {
                if (n == 32) continue;
                Context ctx("Taproot v1 with non-32-byte witness program verifies (bad witness stack height)");
                ctx.SetNBytePublicKey(n)
                    .SetWitness(ginormous_witness)
                    .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
                    .DoTest(__LINE__)
                    .CheckCallSucceeded()
                    .CheckSignatureCheckerWasNotCalled();
            }
        }

        {
            // Verifies even if witness stack elements are too big
            const valtype dummy_stack_element(1000, 0x00);
            std::vector<valtype> ginormous_witness(1, dummy_stack_element);
            for (size_t n = 2; n <= 40; ++n) {
                if (n == 32) continue;
                Context ctx("Taproot v1 with non-32-byte witness program verifies (bad witness stack element size)");
                ctx.SetNBytePublicKey(n)
                    .SetWitness(ginormous_witness)
                    .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
                    .DoTest(__LINE__)
                    .CheckCallSucceeded()
                    .CheckSignatureCheckerWasNotCalled();
            }
        }
    }

    {
        Context ctx("Taproot v1 P2SH-wrapped verifies");
        ctx.SetValidPublicKey()
            .SetValidSignatureInWitness(SIGHASH_DEFAULT)
            .SetP2SHWrapped()
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
            .SetVerifyFlags(SCRIPT_VERIFY_SIGPUSHONLY | SCRIPT_VERIFY_TAPROOT) // can't verify WITNESS here because that implies verify !P2SH
            .DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureCheckerWasNotCalled();
    }

    {
        Context ctx("Taproot v1 with empty witness stack fails");
        ctx.SetValidPublicKey()
            .SetWitness({})
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
            .DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY)
            .CheckSignatureCheckerWasNotCalled();
    }

    {
        Context ctx("Taproot v1 key path spend where signature verification fails");
        ctx.SetValidPublicKey()
            .SetValidSignatureInWitness(SIGHASH_DEFAULT)
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
            .DoTest(__LINE__)
            .CheckCallFailed(SCRIPT_ERR_SCHNORR_SIG)
            .CheckSignatureCheckerWasCalled();
    }

    // BIP-341 calls for checking that the `hash_type` is valid, also that if the
    // `hash_type == SIGHASH_SINGLE` that there's a corresponding output: these
    // checks are tested in test `signature_hash_schnorr_failure_cases`.

    // Additional checks for code coverage (white box tests)
    {
        Context ctx("Taproot v1 key path spend with empty witness stack but no VERIFY_TAPROOT flag succeeds");
        ctx.SetValidPublicKey()
            .SetWitness({})
            .SetSchnorrSignatureValidation(CHECKER_VALIDATION::ALWAYS_FAILS)
            .SetVerifyFlags(SCRIPT_VERIFY_SIGPUSHONLY | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)
            .DoTest(__LINE__)
            .CheckCallSucceeded()
            .CheckSignatureCheckerWasNotCalled();
    }
}

///////////////////////////////////////////////
// 🡆🡆🡆 DEATH TESTS ONLY PAST THIS POINT 🡄🡄🡄
///////////////////////////////////////////////
//
// See `src/test/util/boost_test_boosts.h` for explanation of the
// `BOOST_CHECK_SIGABRT` macro.  Note that for each such assertion below a
// message will be issued to the log along the lines of "... Assertion ...
// failed.".  For these tests that is an _expected_ result.  The tests succeed
// iff those asserts fail (and print that message, in failing).

#if defined(OK_TO_TEST_ASSERT_FUNCTION)

BOOST_AUTO_TEST_CASE(signature_hash_schnorr_assert_cases)
{
    const SigVersion sigversion = SigVersion::TAPROOT;
    const uint8_t hash_type{SIGHASH_SINGLE};

    // Here we pass the assert
    CMutableTransaction tx_to_m;
    tx_to_m.vin.push_back(CTxIn());
    tx_to_m.vout.push_back(CTxOut());
    uint32_t in_pos{0};

    PrecomputedTransactionData cache;
    cache.m_bip341_taproot_ready = true;
    cache.m_spent_outputs_ready = true;
    cache.m_spent_outputs.push_back(CTxOut());

    ScriptExecutionData execdata;
    execdata.m_annex_init = true;
    execdata.m_annex_present = false;
    execdata.m_annex_hash = uint256::ZERO;

    uint256 hash_out{0};

    // (Deliberate variable shadowing follows for ease in writing separate tests
    // with mainly the same setup.)
    {
        // Check that an invalid SigVersion asserts.
        const SigVersion sigversion = SigVersion::BASE;
        BOOST_CHECK_SIGABRT(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, hash_type, sigversion, cache,
                                                  MissingDataBehavior::FAIL));
    }

    {
        // Check that in_pos must be valid w.r.t. #inputs
        const uint32_t in_pos{2};
        BOOST_CHECK_SIGABRT(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, hash_type, sigversion, cache,
                                                  MissingDataBehavior::FAIL));
    }

    {
        // Check that annex_init must be true
        ScriptExecutionData execdata;
        execdata.m_annex_init = false;
        BOOST_CHECK_SIGABRT(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, hash_type, sigversion, cache,
                                                  MissingDataBehavior::FAIL));
    }

    {
        // Check that tapleaf_hash_init and codeseparator_pos_init must be true
        // if version == TAPSCRIPT
        const SigVersion sigversion = SigVersion::TAPSCRIPT;
        ScriptExecutionData execdata;
        execdata.m_annex_init = true;
        execdata.m_annex_present = false;
        execdata.m_annex_hash = uint256::ZERO;
        execdata.m_tapleaf_hash_init = false;
        execdata.m_codeseparator_pos_init = true;
        BOOST_CHECK_SIGABRT(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, hash_type, sigversion, cache,
                                                  MissingDataBehavior::FAIL));
        execdata.m_tapleaf_hash_init = true;
        execdata.m_codeseparator_pos_init = false;
        BOOST_CHECK_SIGABRT(!SignatureHashSchnorr(hash_out, execdata, tx_to_m,
                                                  in_pos, hash_type, sigversion, cache,
                                                  MissingDataBehavior::FAIL));
    }
}

BOOST_AUTO_TEST_CASE(handle_missing_data)
{
    // `HandleMissingData` is a static free function inside of `interpreter.cpp`.
    // Easiest way to get to it is via `SignatureHashSchnorr<CMutableTransaction>`
    // which takes an explicit `MissingDataBehavior` value which is what is
    // needed to exercise `HandleMissingData`.

    // N.B.: This is somewhat fragile.  We are just finding a path through
    // `SignatureHashSchnorr` that definitely gets to `HandleMissingData`. If
    // the code in `SignatureHashSchnorr` changes for whatever reason the
    // setup code below may no longer pick out that path.

    // Here we pick an acceptable SigVersion and hash type
    const SigVersion sigversion = SigVersion::TAPROOT;
    const uint8_t hash_type{SIGHASH_DEFAULT};

    // Here we pass the assert
    CMutableTransaction tx_to_m;
    tx_to_m.vin.push_back(CTxIn());
    const CTransaction tx_to(tx_to_m);
    const uint32_t in_pos{0};

    // Here we take the `then` clause of the `if`
    PrecomputedTransactionData cache;
    cache.m_bip341_taproot_ready = false;
    cache.m_spent_outputs_ready = false;

    uint256 hash_out{0};
    ScriptExecutionData execdata;

    // `MissingDataBehavior::FAIL` simply returns false
    BOOST_CHECK(!SignatureHashSchnorr(hash_out, execdata, tx_to, in_pos,
                                      hash_type, sigversion, cache,
                                      MissingDataBehavior::FAIL));
    // Any other value for `MissingDataBehavior` triggers an `assert` function
    // which (on Linux) signals SIGABRT.
    BOOST_CHECK_SIGABRT(SignatureHashSchnorr(hash_out, execdata, tx_to, in_pos,
                                             hash_type, sigversion, cache,
                                             MissingDataBehavior::ASSERT_FAIL));
    BOOST_CHECK_SIGABRT(SignatureHashSchnorr(hash_out, execdata, tx_to, in_pos,
                                             hash_type, sigversion, cache,
                                             static_cast<MissingDataBehavior>(25)));
}

#endif

BOOST_AUTO_TEST_SUITE_END()
