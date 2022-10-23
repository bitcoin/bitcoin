// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/range_proof.h>

#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(range_proof_tests, MclTestingSetup)

using MsgPair = std::pair<std::string, std::vector<unsigned char>>;

struct TestCase
{
    std::string name;
    Scalars values;
    bool is_batched;  // prove function is called once for with all values
    bool should_complete_recovery;
    size_t num_amounts;
    bool verify_result;
    MsgPair msg;
};

static G1Point GenNonce()
{
    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });
    return nonce;
}

static TokenId GenTokenId()
{
    TokenId token_id(uint256(123));
    return token_id;
}

static MsgPair GenMsgPair(std::string s = "spagetti meatballs")
{
    std::vector<unsigned char> message { s.begin(), s.end() };
    return std::pair(s, message);
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMsgPair();
    auto token_id = GenTokenId();

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto p = rp.Prove(vs, nonce, msg.second, token_id);

    auto is_valid = rp.Verify(std::vector<Proof> { p }, token_id);
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(test_range_proof_recovery_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMsgPair();
    auto token_id = GenTokenId();

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto proof = rp.Prove(vs, nonce, msg.second, token_id);

    size_t index = 0;
    auto req = AmountRecoveryRequest::of(proof, index, nonce);
    auto reqs = std::vector<AmountRecoveryRequest> { req };
    auto result = rp.RecoverAmounts(reqs, token_id);

    BOOST_CHECK(result.is_completed);
    auto xs = result.amounts;
    BOOST_CHECK(xs.size() == 1);
    BOOST_CHECK(xs[0].gamma == nonce.GetHashWithSalt(100));
    BOOST_CHECK(xs[0].amount == 1);
    BOOST_CHECK(xs[0].message == msg.first);
}

static std::vector<TestCase> BuildTestCases()
{
    RangeProof rp;

    Scalar one(1);
    Scalar two(2);
    Scalar lower_bound(0);
    Scalar upper_bound = (one << 64) - one;  // int64_t max
    // [LB, LB+1, UB-1, UB]
    Scalars valid_inputs;
    valid_inputs.Add(lower_bound);
    valid_inputs.Add(lower_bound + one);
    valid_inputs.Add(upper_bound - one);
    valid_inputs.Add(upper_bound);

    // [-1, UB+1, UB+2, UB*2]
    Scalars invalid_inputs;
    invalid_inputs.Add(one.Negate());
    invalid_inputs.Add(upper_bound + one);
    invalid_inputs.Add(upper_bound + one + one);
    invalid_inputs.Add(upper_bound << 1);

    std::vector<TestCase> test_cases;

    // test single valid value
    for (auto value: valid_inputs.m_vec) {
        Scalars values;
        values.Add(value);

        TestCase x;
        x.name = (boost::format("valid input value %1%") % value.GetString()).str().c_str();
        x.values = values;
        x.is_batched = false;
        x.should_complete_recovery = true;
        x.num_amounts = 1;
        x.msg = GenMsgPair();
        x.verify_result = true;
        test_cases.push_back(x);
    }

    // test single invalid value
    for (auto value: invalid_inputs.m_vec) {
        Scalars values;
        values.Add(value);

        TestCase x;
        x.name = (boost::format("invalid input value %1%") % value.GetString()).str().c_str();
        x.values = values;
        x.is_batched = false;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.msg = GenMsgPair();
        x.verify_result = false;
        test_cases.push_back(x);
    }

    // test batched valid values
    {
        TestCase x;
        x.name = "batched valid values";
        x.values = valid_inputs;
        x.is_batched = true;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.msg = GenMsgPair();
        x.verify_result = true;
        test_cases.push_back(x);
    }

    // test batched invalid values
    {
        TestCase x;
        x.name = "batched invalid values";
        x.values = invalid_inputs;
        x.is_batched = true;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.msg = GenMsgPair();
        x.verify_result = false;
        test_cases.push_back(x);
    }

    // test with messages of various length
    {
        Scalars values;
        values.Add(Scalar(1));

        std::vector msg_sizes { 1ul, 23ul, 24ul, Config::m_max_message_size };
        for (auto msg_size: msg_sizes) {
            TestCase x;
            x.name = (boost::format("with message of length %1%") % msg_size).str().c_str();
            x.values = values;
            x.is_batched = true;
            x.should_complete_recovery = true;
            x.num_amounts = 1;
            x.msg = GenMsgPair(std::string(msg_size, 'x'));
            x.verify_result = true;
            test_cases.push_back(x);
        }
    }

    // test # of input values from 1 to max
    {
        for (size_t n=1; n<=Config::m_max_input_values; ++n) {
            Scalars values;
            for (size_t i=0; i<n; ++i) {
                values.Add(Scalar(i + 1));
            }
            TestCase x;
            x.name = (boost::format("%1% valid input values") % n).str().c_str();
            x.values = values;
            x.is_batched = true;
            x.should_complete_recovery = true;
            x.num_amounts = n == 1 ? 1 : 0;  // recovery should be performed only when n=1
            x.msg = GenMsgPair();
            x.verify_result = true;
            test_cases.push_back(x);
        }
    }

    // test valid and invalid values mixed
    {
        Scalars values;
        for (auto& s: valid_inputs.m_vec) values.Add(s);
        for (auto& s: invalid_inputs.m_vec) values.Add(s);

        TestCase x;
        x.name = "mix of valid and invalid values";
        x.values = values;
        x.is_batched = true;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.msg = GenMsgPair();
        x.verify_result = false;
        test_cases.push_back(x);
    }

    return test_cases;
}

static void RunTestCase(
    RangeProof& rp,
    TestCase& test_case
) {
    printf(" Test case '%s'\n", test_case.name.c_str());

    auto token_id = GenTokenId();
    auto nonce = GenNonce();

    std::vector<Proof> proofs;

    // calculate proofs
    if (test_case.is_batched) {
        auto proof = rp.Prove(test_case.values, nonce, test_case.msg.second, token_id);
        proofs.push_back(proof);
    } else {
        for (auto value: test_case.values.m_vec) {
            Scalars single_value_vec;
            single_value_vec.Add(value);
            auto proof = rp.Prove(single_value_vec, nonce, test_case.msg.second, token_id);
            proofs.push_back(proof);
        }
    }

    // verify proofs
    auto verify_result = rp.Verify(proofs, token_id);
    BOOST_CHECK(verify_result == test_case.verify_result);

    // recover value, gamma and message
    std::vector<AmountRecoveryRequest> reqs;

    for (size_t i=0; i<proofs.size(); ++i) {
        reqs.push_back(AmountRecoveryRequest::of(proofs[i], i, nonce));
    }
    auto recovery_result = rp.RecoverAmounts(reqs, token_id);
    BOOST_CHECK(recovery_result.is_completed == test_case.should_complete_recovery);

    if (recovery_result.is_completed) {
        auto amounts = recovery_result.amounts;
        BOOST_CHECK(amounts.size() == test_case.num_amounts);

        for (size_t i=0; i<amounts.size(); ++i) {
            auto x = amounts[i];
            auto gamma = nonce.GetHashWithSalt(100 + i);

            BOOST_CHECK(((uint64_t) x.amount) == test_case.values[i].GetUint64());
            BOOST_CHECK(x.gamma == gamma);

            std::vector<unsigned char> x_msg(x.message.begin(), x.message.end());
            BOOST_CHECK(x_msg == test_case.msg.second);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_recovery)
{
    auto test_cases = BuildTestCases();
    RangeProof rp;
    printf("====> Started running range proof prove/verify/recovery tests\n");
    for (auto test_case: test_cases) {
        RunTestCase(rp, test_case);
    }
    printf("====> Done\n");
}

BOOST_AUTO_TEST_CASE(test_range_proof_message_size)
{
    RangeProof rp;

    Scalars values;
    values.Add(Scalar(1));
    G1Point nonce = G1Point::GetBasePoint();
    TokenId token_id;

    {
        // empty msg
        std::vector<unsigned char> msg;
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // msg of valid size
        std::string s(Config::m_max_message_size, 'x');
        std::vector<unsigned char> msg(s.begin(), s.end());
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // msg of exceeded size
        std::string s(Config::m_max_message_size + 1, 'x');
        std::vector<unsigned char> msg(s.begin(), s.end());
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_number_of_input_values)
{
    RangeProof rp;
    G1Point nonce = G1Point::GetBasePoint();
    std::vector<unsigned char> msg;
    TokenId token_id;

    {
        // should throw if there is no input value
        Scalars values;
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
    {
        // should not throw if number of input values is within the valid range
        Scalars values;
        values.Add(Scalar(1));
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // should throw if number of input values is outsize the valid range
        Scalars values;
        for (size_t i=0; i<Config::m_max_input_values + 1; ++i) {
            values.Add(Scalar(1));
        }
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_validate_proofs_by_sizes)
{
    auto gen_valid_proof_wo_value_commitments = []() {
        Proof p;
        p.num_rounds = 1;
        p.Ls.Add(G1Point::GetBasePoint());
        p.Rs.Add(G1Point::GetBasePoint());
        return p;
    };

    RangeProof rp;
    {
        // no proof should validate fine
        std::vector<Proof> proofs;
        BOOST_CHECK_NO_THROW(rp.ValidateProofsBySizes(proofs));
    }
    {
        // no value commitment
        auto p = gen_valid_proof_wo_value_commitments();
        std::vector<Proof> proofs { p };
        BOOST_CHECK_THROW(rp.ValidateProofsBySizes(proofs), std::runtime_error);
    }
    {
        // minimum number of value commitments
        auto p = gen_valid_proof_wo_value_commitments();
        p.Vs.Add(G1Point::GetBasePoint());
        std::vector<Proof> proofs { p };
        BOOST_CHECK_NO_THROW(rp.ValidateProofsBySizes(proofs));
    }
    {
        // maximum number of value commitments
        auto p = gen_valid_proof_wo_value_commitments();
        for (size_t i=0; i<Config::m_max_input_values; ++i) {
            p.Vs.Add(G1Point::GetBasePoint());
        }
        std::vector<Proof> proofs { p };
        BOOST_CHECK_NO_THROW(rp.ValidateProofsBySizes(proofs));
    }
    {
        // number of value commitments exceeding maximum
        auto p = gen_valid_proof_wo_value_commitments();
        for (size_t i=0; i<Config::m_max_input_values + 1; ++i) {
            p.Vs.Add(G1Point::GetBasePoint());
        }
        std::vector<Proof> proofs { p };
        BOOST_CHECK_THROW(rp.ValidateProofsBySizes(proofs), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()
